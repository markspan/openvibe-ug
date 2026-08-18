///-------------------------------------------------------------------------------------------------
/// 
/// \file CKernelFacade.cpp
/// \author Charles Garraud.
/// \version 1.0.
/// \date 25/01/2016.
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
///
///-------------------------------------------------------------------------------------------------

#include "CCommand.hpp"
#include "base.hpp"
#include "CKernelFacade.hpp"

#include <system/ovCTime.h>

#include <cstdio>
#include <map>
#include <limits>
#include <cassert>

namespace OpenViBE {

using TokenList = std::vector<std::pair<std::string, std::string>>;

static void setConfigTokens(Kernel::IConfigurationManager& configsManager, const TokenList& tokens)
{
    for (const auto& token : tokens) { configsManager.addOrReplaceConfigurationToken(token.first.c_str(), token.second.c_str()); }
}

struct CKernelFacade::SKernelFacadeImpl
{
    CKernelLoader loader;
    Kernel::IKernelContext* ctx = nullptr;
    std::map<std::string, CIdentifier> scenarios;
    std::map<std::string, TokenList> scenarioTokens;
};

CKernelFacade::CKernelFacade() : m_impl(new SKernelFacadeImpl()) { }

// The destructor is needed in the .cpp file to implement pimpl idiom
// with unique_ptr. This is due to the fact that Kernel facade dtor
// has to call unique_ptr<KernelFacadeImpl> deleter function that calls the detete function
// on KernelFacadeImpl. Therefore it needs to know KernelFacadeImpl implementation.
CKernelFacade::~CKernelFacade()
{
    this->UnloadKernel();
    Uninitialize();
}

EPlayerReturnCodes CKernelFacade::LoadKernel(const SLoadKernelCmd& command) const
{
    if (m_impl->ctx) {
        std::cout << "WARNING: The kernel is already loaded" << std::endl;
        return EPlayerReturnCodes::Success;
    }

    const CString kernelFile = Directories::getLib("kernel");

    CKernelLoader& kernelLoader = m_impl->loader;
    CString error;

    if (!kernelLoader.load(kernelFile, &error)) {
        std::cerr << "ERROR: impossible to load kernel from file located at: " << kernelFile << std::endl;
        return EPlayerReturnCodes::KernelLoadingFailure;
    }

    kernelLoader.initialize();

    Kernel::IKernelDesc* kernelDesc = nullptr;
    kernelLoader.getKernelDesc(kernelDesc);

    if (!kernelDesc) {
        std::cerr << "ERROR: impossible to retrieve kernel descriptor " << std::endl;
        return EPlayerReturnCodes::KernelInvalidDesc;
    }

    CString configFile;

    if (command.configFile && !command.configFile.get().empty()) { configFile = command.configFile.get().c_str(); }
    else { configFile = CString(Directories::getDataDir() + "/kernel/openvibe.conf"); }


    Kernel::IKernelContext* ctx = kernelDesc->createKernel("scenario-player", configFile);

    if (!ctx) {
        std::cerr << "ERROR: impossible to create kernel context " << std::endl;
        return EPlayerReturnCodes::KernelInvalidDesc;
    }

    ctx->initialize();
    m_impl->ctx = ctx;
    Toolkit::initialize(*ctx);

    const Kernel::IConfigurationManager& configManager = ctx->getConfigurationManager();
    ctx->getPluginManager().addPluginsFromFiles(configManager.expand("${Kernel_Plugins}"));
    ctx->getMetaboxManager().addMetaboxesFromFiles(configManager.expand("${Kernel_Metabox}"));

    return EPlayerReturnCodes::Success;
}

EPlayerReturnCodes CKernelFacade::UnloadKernel() const
{
    if (m_impl->ctx) {
        // not releasing the scenario before releasing the kernel
        // causes a segfault on linux
        auto& scenarioManager = m_impl->ctx->getScenarioManager();
        for (auto& scenarioPair : m_impl->scenarios) { scenarioManager.releaseScenario(scenarioPair.second); }


        Toolkit::uninitialize(*m_impl->ctx);
        // m_impl->ctx->uninitialize();
        Kernel::IKernelDesc* kernelDesc = nullptr;
        m_impl->loader.getKernelDesc(kernelDesc);
        kernelDesc->releaseKernel(m_impl->ctx);
        m_impl->ctx = nullptr;
    }

    m_impl->loader.uninitialize();
    m_impl->loader.unload();

    return EPlayerReturnCodes::Success;
}

EPlayerReturnCodes CKernelFacade::LoadScenario(const SLoadScenarioCmd& command) const
{
    assert(command.scenarioFile && command.scenarioName);

    if (!m_impl->ctx) {
        std::cerr << "ERROR: Kernel is not loaded" << std::endl;
        return EPlayerReturnCodes::KernelInternalFailure;
    }

    const std::string scenarioFile = command.scenarioFile.get();
    const std::string scenarioName = command.scenarioName.get();

    CIdentifier scenarioID;
    auto& scenarioManager = m_impl->ctx->getScenarioManager();

    if (!scenarioManager.importScenarioFromFile(scenarioID, scenarioFile.c_str(), OVP_GD_ClassId_Algorithm_XMLScenarioImporter)) {
        std::cerr << "ERROR: failed to create scenario " << std::endl;
        return EPlayerReturnCodes::KernelInternalFailure;
    }

    scenarioManager.getScenario(scenarioID).addAttribute(OV_AttributeId_ScenarioFilename, scenarioFile.c_str());

    const auto it = m_impl->scenarios.find(scenarioName);
    if (it != m_impl->scenarios.end()) { scenarioManager.releaseScenario(it->second); }

    m_impl->scenarios[scenarioName] = scenarioID;

    return EPlayerReturnCodes::Success;
}

EPlayerReturnCodes CKernelFacade::UpdateScenario(const SUpdateScenarioCmd& command) const
{
    assert(command.scenarioFile && command.scenarioName);

    auto& scenarioManager = m_impl->ctx->getScenarioManager();

    const auto scenarioName = command.scenarioName.get();
    const auto scenarioFile = command.scenarioFile.get();

    if (m_impl->scenarios.find(scenarioName) == m_impl->scenarios.end()) {
        std::cerr << "ERROR: Trying to update a not loaded scenario " << scenarioName << std::endl;
        return EPlayerReturnCodes::ScenarioNotLoaded;
    }

    auto& scenario = scenarioManager.getScenario(m_impl->scenarios[scenarioName]);

    // check for boxes to be updated
    // scenario.checkBoxesRequiringUpdate();

    // update boxes to be updated
    CIdentifier* listID = nullptr;
    size_t elemCount    = 0;
    scenario.getOutdatedBoxIdentifierList(&listID, &elemCount);
    for (size_t i = 0; i < elemCount; ++i) { scenario.updateBox(listID[i]); }

    // export scenario to the destination file
    if (!scenarioManager.exportScenarioToFile(scenarioFile.c_str(), m_impl->scenarios[scenarioName], OVP_GD_ClassId_Algorithm_XMLScenarioExporter)) {
        std::cerr << "ERROR: failed to create scenario " << std::endl;
        return EPlayerReturnCodes::KernelInternalFailure;
    }

    return EPlayerReturnCodes::Success;
}

EPlayerReturnCodes CKernelFacade::SetupScenario(const SSetupScenarioCmd& command) const
{
    if (!m_impl->ctx) {
        std::cerr << "ERROR: Kernel is not loaded" << std::endl;
        return EPlayerReturnCodes::KernelInternalFailure;
    }

    if (!command.scenarioName) {
        std::cerr << "ERROR: Missing scenario name for setup" << std::endl;
        return EPlayerReturnCodes::KernelInternalFailure;
    }

    const auto name = command.scenarioName.get();

    if (m_impl->scenarios.find(name) == m_impl->scenarios.end()) {
        std::cerr << "ERROR: Trying to configure not loaded scenario " << name << std::endl;
        return EPlayerReturnCodes::ScenarioNotLoaded;
    }

    // token list is just stored at this step for further use at runtime
    // current token list overwrites the previous one
    if (command.tokenList) { m_impl->scenarioTokens[name] = command.tokenList.get(); }

    return EPlayerReturnCodes::Success;
}

EPlayerReturnCodes CKernelFacade::RunScenarioList(const SRunScenarioCmd& command) const
{
    assert(command.scenarioList);

    if (!m_impl->ctx) {
        std::cerr << "ERROR: Kernel is not loaded" << std::endl;
        return EPlayerReturnCodes::KernelInternalFailure;
    }

    auto scenarios = command.scenarioList.get();

    // use of returnCode to store error and achive an RAII-like
    // behavior by releasing all players at the end
    EPlayerReturnCodes returnCode = EPlayerReturnCodes::Success;

    // set up global token
    if (command.tokenList) { setConfigTokens(m_impl->ctx->getConfigurationManager(), command.tokenList.get()); }

    auto& playerManager = m_impl->ctx->getPlayerManager();

    // Keep 2 different containers because identifier information is
    // not relevant during the performance sensitive loop task.
    // This might be premature optimization...
    std::vector<Kernel::IPlayer*> players;
    std::vector<CIdentifier> playerIDs;

    // attach players to scenario
    for (auto& pair : m_impl->scenarios) {
        auto name = pair.first;
        if (std::find(scenarios.begin(), scenarios.end(), name) == scenarios.end()) { continue; } // not in the list of scenario to run 

        CIdentifier id;
        if (!playerManager.createPlayer(id) || id == CIdentifier::undefined()) {
            std::cerr << "ERROR: impossible to create player" << std::endl;
            returnCode = EPlayerReturnCodes::KernelInternalFailure;
            break;
        }

        Kernel::IPlayer* player = &playerManager.getPlayer(id);

        // player identifier is pushed here to ensure a correct cleanup event if player initialization fails
        playerIDs.push_back(id);

        CNameValuePairList configTokens;
        for (auto& token : m_impl->scenarioTokens[name]) { configTokens.setValue(token.first, token.second); }

        // Scenario attachment with setup of local token
        if (!player->setScenario(pair.second, &configTokens)) {
            std::cerr << "ERROR: impossible to set player scenario " << name << std::endl;
            returnCode = EPlayerReturnCodes::KernelInternalFailure;
            break;
        }

        if (player->initialize() == Kernel::EPlayerReturnCodes::Success) {
            if (command.playMode && command.playMode.get() == EPlayerPlayMode::Fastfoward) { player->forward(); }
            else { player->play(); }

            players.push_back(player);
        }
        else {
            std::cerr << "ERROR: impossible to initialize player for scenario " << name << std::endl;
            returnCode = EPlayerReturnCodes::KernelInternalFailure;
            break;
        }
    }

    if (returnCode == EPlayerReturnCodes::Success) {
        // loop until timeout
        const uint64_t startTime = System::Time::zgetTime();
        uint64_t lastLoopTime    = startTime;

        // cannot directly feed secondsToTime with parameters.m_MaximumExecutionTime
        // because it could overflow
        const double boundedMaxExecutionTimeInS = CTime::max().toSeconds();

        uint64_t maxExecutionTimeInFixedPoint;
        if (command.maximumExecutionTime && command.maximumExecutionTime.get() > 0 && command.maximumExecutionTime.get() < boundedMaxExecutionTimeInS) {
            maxExecutionTimeInFixedPoint = CTime(double(command.maximumExecutionTime.get())).time();
        }
        else { maxExecutionTimeInFixedPoint = std::numeric_limits<uint64_t>::max(); }

        bool allStopped{ false };
        while (!allStopped) // negative condition here because it is easier to reason about it
        {
            const uint64_t currentTime = System::Time::zgetTime();
            allStopped                 = true;
            for (auto* p : players) {
                if (p->getStatus() != Kernel::EPlayerStatus::Stop) {
                    if (!p->loop(currentTime - lastLoopTime, maxExecutionTimeInFixedPoint)) { returnCode = EPlayerReturnCodes::KernelInternalFailure; }
                }

                if (p->getCurrentSimulatedTime() >= maxExecutionTimeInFixedPoint) { p->stop(); }

                allStopped &= (p->getStatus() == Kernel::EPlayerStatus::Stop);
            }

            lastLoopTime = currentTime;
        }
    }

    // release players
    for (auto& id : playerIDs) {
        playerManager.getPlayer(id).uninitialize();
        playerManager.releasePlayer(id);
    }

    return returnCode;
}

}  // namespace OpenViBE
