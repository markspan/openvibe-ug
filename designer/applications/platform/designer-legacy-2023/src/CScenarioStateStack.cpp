///-------------------------------------------------------------------------------------------------
/// 
/// \file CScenarioStateStack.cpp
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

#include "CScenarioStateStack.hpp"

#include "CInterfacedScenario.hpp"
#include <zlib.h>
#include <ovp_global_defines.h>

namespace OpenViBE {
namespace Designer {

CScenarioStateStack::CScenarioStateStack(const Kernel::IKernelContext& ctx, CInterfacedScenario& interfacedScenario, Kernel::IScenario& scenario)
    : m_kernelCtx(ctx), m_interfacedScenario(interfacedScenario), m_scenario(scenario)
{
    m_currentState  = m_states.begin();
    m_nMaximumState = size_t(m_kernelCtx.getConfigurationManager().expandAsUInteger("${Designer_UndoRedoStackSize}", 64));
}

bool CScenarioStateStack::Undo()
{
    auto it = m_currentState;
    if (it == m_states.begin()) { return false; }

    --it;
    m_currentState = it;

    return this->restoreState(**m_currentState);
}

bool CScenarioStateStack::IsRedoPossible()
{
    auto it = m_currentState;
    if (it == m_states.end()) { return false; }

    ++it;
    return it != m_states.end();
}

bool CScenarioStateStack::Redo()
{
    auto it = m_currentState;
    if (it == m_states.end()) { return false; }

    ++it;
    if (it == m_states.end()) { return false; }

    m_currentState = it;

    return this->restoreState(**m_currentState);
}

bool CScenarioStateStack::Snapshot()
{
    CMemoryBuffer* newState = new CMemoryBuffer();

    if (!this->dumpState(*newState)) {
        delete newState;
        return false;
    }

    if (m_currentState != m_states.end()) { ++m_currentState; }

    while (m_currentState != m_states.end()) {
        delete*m_currentState;
        m_currentState = m_states.erase(m_currentState);
    }

    if (m_nMaximumState != 0) { while (m_states.size() >= m_nMaximumState) { m_states.erase(m_states.begin()); } }

    m_states.push_back(newState);

    m_currentState = m_states.end();
    --m_currentState;

    return true;
}

bool CScenarioStateStack::restoreState(const CMemoryBuffer& state) const
{
    CMemoryBuffer uncompressedBuffer;

    if (state.getSize() == 0) { return false; }

    const CIdentifier importerID = m_kernelCtx.getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_XMLScenarioImporter);
    if (importerID == CIdentifier::undefined()) { return false; }

    Kernel::IAlgorithmProxy* importer = &m_kernelCtx.getAlgorithmManager().getAlgorithm(importerID);
    if (!importer) { return false; }

    const uLongf srcSize   = uLongf(state.getSize()) - sizeof(uLongf);
    const Bytef* srcBuffer = state.getDirectPointer();

    uLongf dstSize = *(uLongf*)(state.getDirectPointer() + state.getSize() - sizeof(uLongf));
    uncompressedBuffer.setSize(dstSize, true);
    Bytef* dstBuffer = uncompressedBuffer.getDirectPointer();

    if (uncompress(dstBuffer, &dstSize, srcBuffer, srcSize) != Z_OK) { return false; }

    importer->initialize();

    Kernel::TParameterHandler<const CMemoryBuffer*> buffer(importer->getInputParameter(OV_Algorithm_ScenarioImporter_InputParameterId_MemoryBuffer));
    Kernel::TParameterHandler<Kernel::IScenario*> scenario(importer->getOutputParameter(OV_Algorithm_ScenarioImporter_OutputParameterId_Scenario));

    m_scenario.clear();

    buffer   = &uncompressedBuffer;
    scenario = &m_scenario;

    importer->process();
    importer->uninitialize();
    m_kernelCtx.getAlgorithmManager().releaseAlgorithm(*importer);

    // Find the VisualizationTree metadata
    Kernel::IMetadata* treeMetadata = nullptr;
    CIdentifier metadataID          = CIdentifier::undefined();
    while ((metadataID = m_scenario.getNextMetadataIdentifier(metadataID)) != CIdentifier::undefined()) {
        treeMetadata = m_scenario.getMetadataDetails(metadataID);
        if (treeMetadata && treeMetadata->getType() == OVVIZ_MetadataIdentifier_VisualizationTree) { break; }
    }

    VisualizationToolkit::IVisualizationTree* visualizationTree = m_interfacedScenario.m_Tree;
    if (treeMetadata && visualizationTree) { visualizationTree->deserialize(treeMetadata->getData()); }

    return true;
}

bool CScenarioStateStack::dumpState(CMemoryBuffer& state) const
{
    CMemoryBuffer uncompressedBuffer;
    CMemoryBuffer compressedBuffer;

    // Update the scenario metadata according to the current state of the visualization tree

    // Remove all VisualizationTree type metadata
    CIdentifier oldTreeMetadataID = CIdentifier::undefined();
    CIdentifier metadataID        = CIdentifier::undefined();
    while ((metadataID = m_scenario.getNextMetadataIdentifier(metadataID)) != CIdentifier::undefined()) {
        if (m_scenario.getMetadataDetails(metadataID)->getType() == OVVIZ_MetadataIdentifier_VisualizationTree) {
            oldTreeMetadataID = metadataID;
            m_scenario.removeMetadata(metadataID);
            metadataID = CIdentifier::undefined();
        }
    }

    // Insert new metadata
    m_scenario.addMetadata(metadataID, oldTreeMetadataID);
    m_scenario.getMetadataDetails(metadataID)->setType(OVVIZ_MetadataIdentifier_VisualizationTree);
    m_scenario.getMetadataDetails(metadataID)->setData(m_interfacedScenario.m_Tree->serialize());

    const CIdentifier exporterID = m_kernelCtx.getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_XMLScenarioExporter);

    if (exporterID == CIdentifier::undefined()) { return false; }

    Kernel::IAlgorithmProxy* exporter = &m_kernelCtx.getAlgorithmManager().getAlgorithm(exporterID);
    if (!exporter) { return false; }

    exporter->initialize();

    Kernel::TParameterHandler<const Kernel::IScenario*> scenario(exporter->getInputParameter(OV_Algorithm_ScenarioExporter_InputParameterId_Scenario));
    Kernel::TParameterHandler<CMemoryBuffer*> buffer(exporter->getOutputParameter(OV_Algorithm_ScenarioExporter_OutputParameterId_MemoryBuffer));

    scenario = &m_scenario;
    buffer   = &uncompressedBuffer;

    exporter->process();
    exporter->uninitialize();
    m_kernelCtx.getAlgorithmManager().releaseAlgorithm(*exporter);

    const uLongf srcSize   = uLongf(uncompressedBuffer.getSize());
    const Bytef* srcBuffer = uncompressedBuffer.getDirectPointer();

    compressedBuffer.setSize(12 + uint64_t(srcSize * 1.1), true);

    uLongf dstSize   = uLongf(compressedBuffer.getSize());
    Bytef* dstBuffer = compressedBuffer.getDirectPointer();

    if (compress(dstBuffer, &dstSize, srcBuffer, srcSize) != Z_OK) { return false; }

    state.setSize(0, true);
    state.append(compressedBuffer.getDirectPointer(), dstSize);
    state.append(reinterpret_cast<const uint8_t*>(&srcSize), sizeof(uLongf));

    return true;
}

}  // namespace Designer
}  // namespace OpenViBE
