///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmStatisticGenerator.cpp
/// \author Serriere Guillaume (Inria)
/// \version 1.0.
/// \date 30/04/2015.
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

#include "CBoxAlgorithmStatisticGenerator.hpp"

#include <sstream>

#include "tinyxml2.h"
#include <limits>
#include <iomanip>

namespace {
const char* const STATISTIC_ROOT_NODE = "Statistic";

const char* const STIM_LIST_NODE  = "Stimulations-list";
const char* const STIM_NODE       = "Stimulation";
const char* const STIM_ID_NODE    = "Identifier";
const char* const STIM_LABEL_NODE = "Label";
const char* const STIM_COUNT_NODE = "Count";

const char* const CHANNEL_LIST_NODE  = "Channel-list";
const char* const CHANNEL_NODE       = "Channel";
const char* const CHANNEL_LABEL_NODE = "Name";
const char* const CHANNEL_MIN_NODE   = "Minimum";
const char* const CHANNEL_MAX_NODE   = "Maximum";
const char* const CHANNEL_MEAN_NODE  = "Mean";
}  // namespace


namespace OpenViBE {
namespace Plugins {
namespace Evaluation {

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmStatisticGenerator::initialize()
{
    m_signalDecoder.initialize(*this, 0);
    m_stimDecoder.initialize(*this, 1);

    m_stimulations.clear();
    m_hasBeenStreamed = false;
    m_filename        = std::string(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0));

    if (m_filename.empty()) {
        this->getLogManager() << Kernel::LogLevel_Error << "The filename is empty\n";
        return false;
    }
    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmStatisticGenerator::uninitialize()
{
    m_signalDecoder.uninitialize();
    m_stimDecoder.uninitialize();

    if (m_hasBeenStreamed) { return saveXML(); }
    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmStatisticGenerator::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmStatisticGenerator::process()
{
    const Kernel::IBoxIO& boxCtx = this->getDynamicBoxContext();

    for (size_t i = 0; i < boxCtx.getInputChunkCount(0); ++i) {
        m_signalDecoder.decode(i);
        if (m_signalDecoder.isHeaderReceived()) {
            const size_t mountChannel = m_signalDecoder.getOutputMatrix()->getDimensionSize(0);
            m_hasBeenStreamed         = true;
            for (size_t j = 0; j < mountChannel; ++j) {
                signal_info_t info = {
                    m_signalDecoder.getOutputMatrix()->getDimensionLabel(0, j), std::numeric_limits<double>::max(), -std::numeric_limits<double>::max(), 0, 0
                };
                m_signalInfos.push_back(info);
            }
        }
        if (m_signalDecoder.isBufferReceived()) {
            const size_t nSample = m_signalDecoder.getOutputMatrix()->getDimensionSize(1);
            const double* buffer = m_signalDecoder.getOutputMatrix()->getBuffer();
            for (size_t j = 0; j < m_signalInfos.size(); ++j) {
                signal_info_t& info = m_signalInfos[j];
                for (size_t k = 0; k < nSample; ++k) {
                    const double sample = buffer[j * nSample + k];
                    info.sum += sample;

                    if (sample < info.min) { info.min = sample; }
                    if (sample > info.max) { info.max = sample; }
                }
                info.nSample += nSample;
            }
        }
    }

    for (size_t i = 0; i < boxCtx.getInputChunkCount(1); ++i) {
        m_stimDecoder.decode(i);
        if (m_stimDecoder.isHeaderReceived()) { m_hasBeenStreamed = true; }
        if (m_stimDecoder.isBufferReceived()) {
            CStimulationSet& stimSet = *(m_stimDecoder.getOutputStimulationSet());
            for (size_t j = 0; j < stimSet.size(); ++j) { m_stimulations[stimSet.getId(j)]++; }
        }
    }

    return true;
}

///-------------------------------------------------------------------------------------------------
std::string CBoxAlgorithmStatisticGenerator::getFixedvalue(const double value, const size_t precision)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision) << value;
    return ss.str();
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmStatisticGenerator::saveXML()
{
    tinyxml2::XMLDocument xmlDoc;
    // Create Root
    tinyxml2::XMLNode* root = xmlDoc.NewElement(STATISTIC_ROOT_NODE);       // Create root node
    xmlDoc.InsertFirstChild(root);                                          // Add root to XML

    // Stimulations list
    tinyxml2::XMLElement* stimList = xmlDoc.NewElement(STIM_LIST_NODE);     // Create Stim List node
    for (const auto& s : m_stimulations) {
        tinyxml2::XMLElement* node   = xmlDoc.NewElement(STIM_NODE);        // Create Main node for Stim
        tinyxml2::XMLElement* idNode = xmlDoc.NewElement(STIM_ID_NODE);     // Create Id node for Stim
        tinyxml2::XMLElement* label  = xmlDoc.NewElement(STIM_LABEL_NODE);  // Create Label node for Stim
        tinyxml2::XMLElement* count  = xmlDoc.NewElement(STIM_COUNT_NODE);  // Create Count node for Stim

        CIdentifier id = s.first;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(10) << m_stimulations[id];

        idNode->SetText(id.str().c_str());
        label->SetText(this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, id.id()).toASCIIString());
        count->SetText(ss.str().c_str());

        node->InsertEndChild(idNode);
        node->InsertEndChild(label);
        node->InsertEndChild(count);
        stimList->InsertEndChild(node);
    }
    root->InsertEndChild(stimList);

    // Channels list
    tinyxml2::XMLElement* channelList = xmlDoc.NewElement(CHANNEL_LIST_NODE);   // Create Channel List node
    for (const auto& s : m_signalInfos) {
        tinyxml2::XMLElement* node    = xmlDoc.NewElement(CHANNEL_NODE);        // Create Main node for Channel
        tinyxml2::XMLElement* name    = xmlDoc.NewElement(CHANNEL_LABEL_NODE);  // Create Name node for Channel
        tinyxml2::XMLElement* maximum = xmlDoc.NewElement(CHANNEL_MAX_NODE);    // Create Maximum node for Channel
        tinyxml2::XMLElement* minimum = xmlDoc.NewElement(CHANNEL_MIN_NODE);    // Create Minimum node for Channel
        tinyxml2::XMLElement* mean    = xmlDoc.NewElement(CHANNEL_MEAN_NODE);   // Create Mean node for Channel

        name->SetText(s.name.toASCIIString());
        maximum->SetText(getFixedvalue(s.max).c_str());
        minimum->SetText(getFixedvalue(s.min).c_str());
        mean->SetText(getFixedvalue(s.sum / double(s.nSample)).c_str());

        node->InsertEndChild(name);
        node->InsertEndChild(maximum);
        node->InsertEndChild(minimum);
        node->InsertEndChild(mean);
        channelList->InsertEndChild(node);
    }
    root->InsertEndChild(channelList);

    return xmlDoc.SaveFile(m_filename.c_str()) == 0;    // save XML (if != 0 it means error)
}
}  // namespace Evaluation
}  // namespace Plugins
}  // namespace OpenViBE
