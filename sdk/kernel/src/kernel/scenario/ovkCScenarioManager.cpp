// UG fork: the errors about settings and files here say what is wrong and what to do, politely. See UG.md, 2.23.
#include "ovkCScenarioManager.h"
#include "ovkCScenario.h"

#include <cstdlib>
#include <fs/Files.h>
#include <cassert>
#include <openvibe/kernel/scenario/ovIAlgorithmScenarioImporter.h>
#include <openvibe/kernel/scenario/ovIAlgorithmScenarioExporter.h>
#include "../../tools/ovkSBoxProto.h"

namespace OpenViBE {
namespace Kernel {

CScenarioManager::CScenarioManager(const IKernelContext& ctx) : TKernelObject<IScenarioManager>(ctx) {}

CScenarioManager::~CScenarioManager() { for (auto i = m_scenarios.begin(); i != m_scenarios.end(); ++i) { delete i->second; } }

void CScenarioManager::cloneScenarioImportersAndExporters(const IScenarioManager& scenarioManager)
{
    CIdentifier importContextId = CIdentifier::undefined();
    // Copy the registered importers from the parent Scenario Manager
    while ((importContextId = scenarioManager.getNextScenarioImportContext(importContextId)) != CIdentifier::undefined())
    {
        CString fileNameExtension = "";
        while ((fileNameExtension = scenarioManager.getNextScenarioImporter(importContextId, fileNameExtension)) != CString(""))
        {
            CIdentifier algorithmId = scenarioManager.getScenarioImporterAlgorithmIdentifier(importContextId, fileNameExtension);

            this->registerScenarioImporter(importContextId, fileNameExtension, algorithmId);
        }
    }

    CIdentifier exportContextId = CIdentifier::undefined();
    while ((exportContextId = scenarioManager.getNextScenarioExportContext(exportContextId)) != CIdentifier::undefined())
    {
        CString fileNameExtension = "";
        while ((fileNameExtension = scenarioManager.getNextScenarioExporter(exportContextId, fileNameExtension)) != CString(""))
        {
            CIdentifier algorithmId = scenarioManager.getScenarioExporterAlgorithmIdentifier(exportContextId, fileNameExtension);

            this->registerScenarioExporter(exportContextId, fileNameExtension, algorithmId);
        }
    }
}

CIdentifier CScenarioManager::getNextScenarioIdentifier(const CIdentifier& previousID) const
{
    std::map<CIdentifier, CScenario*>::const_iterator itScenario;

    if (previousID == CIdentifier::undefined()) { itScenario = m_scenarios.begin(); }
    else
    {
        itScenario = m_scenarios.find(previousID);
        if (itScenario == m_scenarios.end()) { return CIdentifier::undefined(); }
        ++itScenario;
    }

    return itScenario != m_scenarios.end() ? itScenario->first : CIdentifier::undefined();
}

bool CScenarioManager::isScenario(const CIdentifier& scenarioID) const { return m_scenarios.find(scenarioID) != m_scenarios.end(); }

bool CScenarioManager::createScenario(CIdentifier& scenarioID)
{
    //create scenario object
    scenarioID              = getUnusedIdentifier();
    CScenario* scenario     = new CScenario(getKernelContext(), scenarioID);
    m_scenarios[scenarioID] = scenario;

    return true;
}

bool CScenarioManager::importScenario(CIdentifier& scenarioID, const CMemoryBuffer& iMemoryBuffer, const CIdentifier& scenarioImporterAlgorithmID)
{
    scenarioID = CIdentifier::undefined();

    OV_ERROR_UNLESS_KRF(this->createScenario(scenarioID), "Error creating new scenario", Kernel::ErrorType::BadResourceCreation);

    const auto releaseScenario = [&]()
    {
        // use a fatal here because a release failure while creation succeeded
        // means we are in an unexpected state
        OV_FATAL_UNLESS_K(this->releaseScenario(scenarioID), "Releasing just created scenario failed for " << scenarioID.str(),
                          ErrorType::Internal);
        scenarioID = CIdentifier::undefined();
    };

    IScenario& newScenarioInstance = this->getScenario(scenarioID);

    if (!iMemoryBuffer.getSize())
    {
        releaseScenario();
        OV_ERROR_KRF("Buffer containing scenario data is empty", Kernel::ErrorType::BadValue);
    }

    CIdentifier importerInstanceIdentifier = this->getKernelContext().getAlgorithmManager().createAlgorithm(scenarioImporterAlgorithmID);

    if (importerInstanceIdentifier == CIdentifier::undefined())
    {
        releaseScenario();
        OV_ERROR_KRF("Can not create the requested scenario importer", Kernel::ErrorType::BadResourceCreation);
    }

    IAlgorithmProxy* importer = &this->getKernelContext().getAlgorithmManager().getAlgorithm(importerInstanceIdentifier);

    OV_FATAL_UNLESS_K(
        importer,
        "Importer with id " << importerInstanceIdentifier.str() << " not found although it has just been created",
        ErrorType::ResourceNotFound);

    const auto releaseAlgorithm = [&]()
    {
        // use a fatal here because a release failure while creation succeeded
        // means we are in an unexpected state
        OV_FATAL_UNLESS_K(
            this->getKernelContext().getAlgorithmManager().releaseAlgorithm(*importer),
            "Releasing just created algorithm failed for " << importerInstanceIdentifier.str(),
            ErrorType::Internal);
    };

    if (!importer->initialize())
    {
        releaseScenario();
        releaseAlgorithm();
        OV_ERROR_KRF("Can not initialize the requested scenario importer", Kernel::ErrorType::Internal);
    }

    IParameter* memoryBufferParameter = importer->getInputParameter(OV_Algorithm_ScenarioImporter_InputParameterId_MemoryBuffer);
    IParameter* scenarioParameter     = importer->getOutputParameter(OV_Algorithm_ScenarioImporter_OutputParameterId_Scenario);

    if (!(memoryBufferParameter && scenarioParameter))
    {
        releaseScenario();
        releaseAlgorithm();

        OV_ERROR_UNLESS_KRF(
            memoryBufferParameter,
            "The requested importer does not have a MemoryBuffer input parameter with scenarioID " <<
            OV_Algorithm_ScenarioImporter_InputParameterId_MemoryBuffer,
            ErrorType::BadInput);

        OV_ERROR_UNLESS_KRF(
            scenarioParameter,
            "The requested importer does not have a Scenario output parameter with scenarioID " << OV_Algorithm_ScenarioImporter_OutputParameterId_Scenario,
            ErrorType::BadOutput);
    }


    TParameterHandler<const CMemoryBuffer*> memoryBufferParameterHandler(memoryBufferParameter);
    TParameterHandler<IScenario*> scenarioParameterHandler(scenarioParameter);

    memoryBufferParameterHandler = &iMemoryBuffer;
    scenarioParameterHandler     = &newScenarioInstance;

    if (!importer->process())
    {
        releaseScenario();
        releaseAlgorithm();
        OV_ERROR_KRF("Can not process data using the requested scenario importer", Kernel::ErrorType::Internal);
    }

    if (!importer->uninitialize())
    {
        releaseScenario();
        releaseAlgorithm();
        OV_ERROR_KRF("Can not uninitialize the requested scenario importer", Kernel::ErrorType::Internal);
    }

    releaseAlgorithm();

    return true;
}

bool CScenarioManager::importScenarioFromFile(CIdentifier& scenarioID, const CString& fileName, const CIdentifier& scenarioImporterAlgorithmID)
{
    scenarioID = CIdentifier::undefined();

    CMemoryBuffer memoryBuffer;

    FILE* inputFile = FS::Files::open(fileName, "rb");

    OV_ERROR_UNLESS_KRF(
        inputFile,
        "the scenario file [" << fileName << "] could not be opened. Perhaps it does not exist, or the path is misspelt. Would you be so very kind as to check it? Thank you for your patience.",
        ErrorType::BadFileRead);

    fseek(inputFile, 0, SEEK_END);
    memoryBuffer.setSize(size_t(ftell(inputFile)), true);
    fseek(inputFile, 0, SEEK_SET);

    if (fread(reinterpret_cast<char*>(memoryBuffer.getDirectPointer()), size_t(memoryBuffer.getSize()), 1, inputFile) != 1)
    {
        fclose(inputFile);
        OV_ERROR_KRF("the scenario file [" << fileName << "] could be opened, but not read to the end. Perhaps it is open in another program. Might we trouble you to check? Thank you most kindly.", Kernel::ErrorType::BadFileRead);
    }
    fclose(inputFile);

    return this->importScenario(scenarioID, memoryBuffer, scenarioImporterAlgorithmID);
}

bool CScenarioManager::importScenarioFromFile(CIdentifier& scenarioID, const CIdentifier& importContext, const CString& fileName)
{
    OV_ERROR_UNLESS_KRF(m_importers.count(importContext),
                        "The import context " << importContext.str() << " has no associated importers",
                        ErrorType::Internal);
    std::vector<char> fileNameExtension;
    fileNameExtension.resize(fileName.length() + 1);
    FS::Files::getFilenameExtension(fileName.toASCIIString(), &fileNameExtension[0]);
    OV_ERROR_UNLESS_KRF(m_importers[importContext].count(&fileNameExtension[0]),
                        "The import context " << importContext.str() << " has no associated importers for extension [" << &fileNameExtension[0] << "]",
                        ErrorType::Internal);
    return this->importScenarioFromFile(scenarioID, fileName, m_importers[importContext][&fileNameExtension[0]]);
}

bool CScenarioManager::registerScenarioImporter(const CIdentifier& importContext, const CString& fileNameExtension,
                                                const CIdentifier& scenarioImporterAlgorithmIdentifier)
{
    if (!m_importers.count(importContext)) { m_importers[importContext] = std::map<std::string, CIdentifier>(); }

    OV_ERROR_UNLESS_KRF(!m_importers[importContext].count(fileNameExtension.toASCIIString()),
                        "The file name extension [" << fileNameExtension << "] already has an importer registered for context " << importContext.str(),
                        ErrorType::Internal);

    m_importers[importContext][fileNameExtension.toASCIIString()] = scenarioImporterAlgorithmIdentifier;

    return true;
}

bool CScenarioManager::unregisterScenarioImporter(const CIdentifier& importContext, const CString& fileNameExtension)
{
    OV_ERROR_UNLESS_KRF(m_importers.count(importContext),
                        "The import context " << importContext.str() << " has no associated importers",
                        ErrorType::Internal);
    OV_ERROR_UNLESS_KRF(m_importers[importContext].count(fileNameExtension.toASCIIString()),
                        "The import context " << importContext.str() << " has no associated importers for extension [" << fileNameExtension << "]",
                        ErrorType::Internal);

    auto& contextImporters = m_importers[importContext];

    for (auto it = contextImporters.begin(); it != contextImporters.end();)
    {
        if (it->first == fileNameExtension.toASCIIString()) { it = contextImporters.erase(it); }
        else { ++it; }
    }
    for (auto it = m_importers.begin(); it != m_importers.end();)
    {
        if (it->second.empty()) { it = m_importers.erase(it); }
        else { ++it; }
    }
    return true;
}

CIdentifier CScenarioManager::getNextScenarioImportContext(const CIdentifier& importContext) const
{
    if (m_importers.empty()) { return CIdentifier::undefined(); }

    if (importContext == CIdentifier::undefined()) { return m_importers.cbegin()->first; }

    auto current = m_importers.find(importContext);
    if (current == m_importers.end() || ++current == m_importers.end()) { return CIdentifier::undefined(); }

    return current->first;
}

CString CScenarioManager::getNextScenarioImporter(const CIdentifier& importContext, const CString& fileNameExtension) const
{
    if (m_importers.empty() || !m_importers.count(importContext)) { return ""; }

    const auto& scenarioImportContextMap = m_importers.at(importContext);

    if (fileNameExtension == CString("")) { return scenarioImportContextMap.cbegin()->first.c_str(); }

    auto current = scenarioImportContextMap.find(fileNameExtension.toASCIIString());
    if (current == scenarioImportContextMap.end() || ++current == scenarioImportContextMap.end()) { return ""; }

    return current->first.c_str();
}

CIdentifier CScenarioManager::getScenarioImporterAlgorithmIdentifier(const CIdentifier& importContext, const CString& fileNameExtension) const
{
    OV_ERROR_UNLESS_KRU(
        !m_importers.empty() && m_importers.count(importContext) && m_importers.at(importContext).count(fileNameExtension.toASCIIString(
        )),
        "Scenario importer not found",
        ErrorType::OutOfBound);

    return m_importers.at(importContext).at(fileNameExtension.toASCIIString());
}

bool CScenarioManager::exportScenario(CMemoryBuffer& oMemoryBuffer, const CIdentifier& scenarioID,
                                      const CIdentifier& scenarioExporterAlgorithmID) const
{
    OV_ERROR_UNLESS_KRF(
        m_scenarios.find(scenarioID) != m_scenarios.end(),
        "Scenario with identifier " << scenarioID.str() << " does not exist.",
        ErrorType::ResourceNotFound);

    // If the scenario is a metabox, we will save its prototype hash into an attribute of the scenario
    // that way the standalone scheduler can check whether metaboxes included inside need updating.
    IScenario& scenario = this->getScenario(scenarioID);


    if (scenario.isMetabox())
    {
        SBoxProto metaboxProto(getKernelContext().getTypeManager());

        for (size_t scenarioInputIdx = 0; scenarioInputIdx < scenario.getInputCount(); ++scenarioInputIdx)
        {
            CIdentifier id;
            CString name;
            CIdentifier typeID;

            scenario.getInterfacorIdentifier(Input, scenarioInputIdx, id);
            scenario.getInputType(scenarioInputIdx, typeID);
            scenario.getInputName(scenarioInputIdx, name);

            metaboxProto.addInput(name, typeID, id, true);
        }

        for (size_t scenarioOutputIdx = 0; scenarioOutputIdx < scenario.getOutputCount(); ++scenarioOutputIdx)
        {
            CIdentifier id;
            CString name;
            CIdentifier typeID;

            scenario.getInterfacorIdentifier(Output, scenarioOutputIdx, id);
            scenario.getOutputType(scenarioOutputIdx, typeID);
            scenario.getOutputName(scenarioOutputIdx, name);

            metaboxProto.addOutput(name, typeID, id, true);
        }

        for (size_t scenarioSettingIdx = 0; scenarioSettingIdx < scenario.getSettingCount(); ++scenarioSettingIdx)
        {
            CString name;
            CIdentifier typeID;
            CString defaultValue;
            CIdentifier id;

            scenario.getSettingName(scenarioSettingIdx, name);
            scenario.getSettingType(scenarioSettingIdx, typeID);
            scenario.getSettingDefaultValue(scenarioSettingIdx, defaultValue);
            scenario.getInterfacorIdentifier(Setting, scenarioSettingIdx, id);

            metaboxProto.addSetting(name, typeID, defaultValue, false, id, true);
        }

        if (scenario.hasAttribute(OV_AttributeId_Scenario_MetaboxHash))
        {
            scenario.setAttributeValue(OV_AttributeId_Scenario_MetaboxHash, metaboxProto.m_hash.toString());
        }
        else { scenario.addAttribute(OV_AttributeId_Scenario_MetaboxHash, metaboxProto.m_hash.toString()); }
    }

    CIdentifier exporterInstanceIdentifier = this->getKernelContext().getAlgorithmManager().createAlgorithm(scenarioExporterAlgorithmID);

    OV_ERROR_UNLESS_KRF(
        exporterInstanceIdentifier != CIdentifier::undefined(),
        "Can not create the requested scenario exporter",
        ErrorType::BadResourceCreation);

    IAlgorithmProxy* exporter = &this->getKernelContext().getAlgorithmManager().getAlgorithm(exporterInstanceIdentifier);

    OV_FATAL_UNLESS_K(
        exporter,
        "Exporter with id " << exporterInstanceIdentifier.str() << " not found although it has just been created",
        ErrorType::ResourceNotFound);

    const auto releaseAlgorithm = [&]()
    {
        // use a fatal here because a release failure while creation succeeded
        // means we are in an unexpected state
        OV_FATAL_UNLESS_K(
            this->getKernelContext().getAlgorithmManager().releaseAlgorithm(*exporter),
            "Releasing just created algorithm failed for " << exporterInstanceIdentifier.str(),
            ErrorType::Internal);
    };

    if (!exporter->initialize())
    {
        releaseAlgorithm();
        OV_ERROR_KRF("Can not initialize the requested scenario exporter", Kernel::ErrorType::Internal);
    }

    IParameter* scenarioParameter     = exporter->getInputParameter(OV_Algorithm_ScenarioExporter_InputParameterId_Scenario);
    IParameter* memoryBufferParameter = exporter->getOutputParameter(OV_Algorithm_ScenarioExporter_OutputParameterId_MemoryBuffer);

    if (!(memoryBufferParameter && scenarioParameter))
    {
        releaseAlgorithm();

        OV_ERROR_UNLESS_KRF(
            scenarioParameter,
            "The requested exporter does not have a Scenario input parameter with identifier " << OV_Algorithm_ScenarioExporter_InputParameterId_Scenario,
            ErrorType::BadInput);

        OV_ERROR_UNLESS_KRF(
            memoryBufferParameter,
            "The requested exporter does not have a MemoryBuffer output parameter with identifier " <<
            OV_Algorithm_ScenarioExporter_OutputParameterId_MemoryBuffer,
            ErrorType::BadOutput);
    }

    TParameterHandler<IScenario*> scenarioParameterHandler(scenarioParameter);
    TParameterHandler<const CMemoryBuffer*> memoryBufferParameterHandler(memoryBufferParameter);

    scenarioParameterHandler     = &scenario;
    memoryBufferParameterHandler = &oMemoryBuffer;

    if (!exporter->process())
    {
        releaseAlgorithm();
        OV_ERROR_KRF("Can not process data using the requested scenario exporter", Kernel::ErrorType::Internal);
    }

    if (!exporter->uninitialize())
    {
        releaseAlgorithm();
        OV_ERROR_KRF("Can not uninitialize the requested scenario exporter", Kernel::ErrorType::Internal);
    }

    releaseAlgorithm();
    return true;
}

bool CScenarioManager::exportScenarioToFile(const CString& fileName, const CIdentifier& scenarioID,
                                            const CIdentifier& scenarioExporterAlgorithmID) const
{
    IScenario& scenario = this->getScenario(scenarioID);
    if (scenario.containsBoxWithDeprecatedInterfacors())
    {
        OV_WARNING_K(
            "Cannot export a scenario with pending deprecated I/O or Settings. Please remove them using the Designer."
        );
        return false;
    }

    CMemoryBuffer memoryBuffer;
    this->exportScenario(memoryBuffer, scenarioID, scenarioExporterAlgorithmID);

    std::ofstream outputFileStream;
    FS::Files::openOFStream(outputFileStream, fileName, std::ios::binary);

    OV_ERROR_UNLESS_KRF(
        outputFileStream.good(),
        "the scenario could not be saved in the file [" << fileName << "]. Perhaps its folder does not exist, or the file is open in another program. Might we trouble you to check? Thank you most kindly.",
        ErrorType::BadFileRead);

    outputFileStream.write(reinterpret_cast<const char*>(memoryBuffer.getDirectPointer()), long(memoryBuffer.getSize()));
    outputFileStream.close();

    return true;
}

bool CScenarioManager::exportScenarioToFile(const CIdentifier& exportContext, const CString& fileName, const CIdentifier& scenarioID)
{
    OV_ERROR_UNLESS_KRF(m_exporters.count(exportContext),
                        "The export context " << exportContext.str() << " has no associated exporters",
                        ErrorType::Internal);
    std::vector<char> fileNameExtension;
    fileNameExtension.resize(fileName.length() + 1);
    FS::Files::getFilenameExtension(fileName.toASCIIString(), &fileNameExtension[0]);
    OV_ERROR_UNLESS_KRF(m_exporters[exportContext].count(&fileNameExtension[0]),
                        "The export context " << exportContext.str() << " has no associated exporters for extension [" << &fileNameExtension[0] << "]",
                        ErrorType::Internal);
    return this->exportScenarioToFile(fileName, scenarioID, m_exporters[exportContext][&fileNameExtension[0]]);
}

bool CScenarioManager::registerScenarioExporter(const CIdentifier& exportContext, const CString& fileNameExtension,
                                                const CIdentifier& scenarioExporterAlgorithmIdentifier)
{
    if (!m_exporters.count(exportContext)) { m_exporters[exportContext] = std::map<std::string, CIdentifier>(); }

    OV_ERROR_UNLESS_KRF(!m_exporters[exportContext].count(fileNameExtension.toASCIIString()),
                        "The file name extension [" << fileNameExtension << "] already has an exporter registered for context " << exportContext.str(),
                        ErrorType::Internal);

    m_exporters[exportContext][fileNameExtension.toASCIIString()] = scenarioExporterAlgorithmIdentifier;

    return true;
}

bool CScenarioManager::unregisterScenarioExporter(const CIdentifier& exportContext, const CString& fileNameExtension)
{
    OV_ERROR_UNLESS_KRF(m_exporters.count(exportContext),
                        "The export context " << exportContext.str() << " has no associated exporters",
                        ErrorType::Internal);
    OV_ERROR_UNLESS_KRF(m_exporters[exportContext].count(fileNameExtension.toASCIIString()),
                        "The export context " << exportContext.str() << " has no associated exporters for extension [" << fileNameExtension << "]",
                        ErrorType::Internal);

    auto& contextExporters = m_exporters[exportContext];

    for (auto it = contextExporters.begin(); it != contextExporters.end();)
    {
        if (it->first.c_str() == fileNameExtension.toASCIIString()) { it = contextExporters.erase(it); }
        else { ++it; }
    }
    for (auto it = m_exporters.begin(); it != m_exporters.end();)
    {
        if (it->second.empty()) { it = m_exporters.erase(it); }
        else { ++it; }
    }
    return true;
}

CIdentifier CScenarioManager::getNextScenarioExportContext(const CIdentifier& exportContext) const
{
    if (m_exporters.empty()) { return CIdentifier::undefined(); }

    if (exportContext == CIdentifier::undefined()) { return m_exporters.cbegin()->first; }

    auto current = m_exporters.find(exportContext);
    if (current == m_exporters.end() || ++current == m_exporters.end()) { return CIdentifier::undefined(); }

    return current->first;
}

CString CScenarioManager::getNextScenarioExporter(const CIdentifier& exportContext, const CString& fileNameExtension) const
{
    if (m_exporters.empty() || !m_exporters.count(exportContext)) { return ""; }

    const auto& scenarioExportContextMap = m_exporters.at(exportContext);

    if (fileNameExtension == CString("")) { return scenarioExportContextMap.cbegin()->first.c_str(); }

    auto current = scenarioExportContextMap.find(fileNameExtension.toASCIIString());
    if (current == scenarioExportContextMap.end() || ++current == scenarioExportContextMap.end()) { return ""; }

    return current->first.c_str();
}

CIdentifier CScenarioManager::getScenarioExporterAlgorithmIdentifier(const CIdentifier& exportContext, const CString& fileNameExtension) const
{
    OV_ERROR_UNLESS_KRU(!m_exporters.empty() && m_exporters.count(exportContext) && m_exporters.at(exportContext).count(fileNameExtension.toASCIIString()),
                        "Scenario importer not found", Kernel::ErrorType::OutOfBound);

    return m_exporters.at(exportContext).at(fileNameExtension.toASCIIString());
}

bool CScenarioManager::releaseScenario(const CIdentifier& scenarioID)
{
    const auto it = m_scenarios.find(scenarioID);
    if (it == m_scenarios.end())
    {
        // error is handled on a higher level
        return false;
    }

    CScenario* scenario = it->second;
    delete scenario;
    m_scenarios.erase(it);

    return true;
}

IScenario& CScenarioManager::getScenario(const CIdentifier& scenarioID)
{
    const auto itScenario = m_scenarios.find(scenarioID);

    // If the call is wrongly handled, and falls in this condition then next instruction causes a crash...
    // At least, here the abortion is handled!
    OV_FATAL_UNLESS_K(itScenario != m_scenarios.end(), "Scenario " << scenarioID.str() << " does not exist !", Kernel::ErrorType::ResourceNotFound);

    return *itScenario->second;
}

IScenario& CScenarioManager::getScenario(const CIdentifier& scenarioID) const { return const_cast<CScenarioManager*>(this)->getScenario(scenarioID); }

CIdentifier CScenarioManager::getUnusedIdentifier() const
{
    uint64_t id = (uint64_t(rand()) << 32) + uint64_t(rand());
    CIdentifier res;
    std::map<CIdentifier, CScenario*>::const_iterator i;
    do
    {
        id++;
        res = CIdentifier(id);
        i   = m_scenarios.find(res);
    } while (i != m_scenarios.end() || res == CIdentifier::undefined());
    return res;
}

}  // namespace Kernel
}  // namespace OpenViBE
