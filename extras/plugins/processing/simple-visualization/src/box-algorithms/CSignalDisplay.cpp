/**-------------------------------------------------------------------------------------------------
 * \file CSignalDisplay.cpp
 * \brief Classes implementation for the Box Signal display.
 * \author Bruno Renier, Yann Renard, Alison Cellard, Jussi T. Lindgren (INRIA/IRISA).
 * \version 0.3.
 * \copyright Copyright (C) 2022 Inria
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *-------------------------------------------------------------------------------------------------
 */

#include "CSignalDisplay.h"

#include "../CSignalDisplayController.h"

#include <visualization-toolkit/ovviz_all.h>

#include <QCoreApplication>
#include <QThread>

#include <string>
#include <utility>
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

// UG fork: the box settings, by name, are DISPLAY_SETTING_NAMES in CSignalDisplay.h, shared with the LSL display.
// "Scale refresh interval (secs)" is gone, as the scale is worked out again for every page. The scenarios in this repository
// no longer have it; others still play, as settings are read by name and the extra one is not read.

void addDisplaySettings(Kernel::IBoxProto& prototype)
{
    // UG fork: moved here from CSignalDisplayDesc, in the same order, so that the Signal display's prototype is unchanged
    prototype.addSetting("Display Mode", TypeId_SignalDisplayMode, "Scan"); //list scroll or scan
    prototype.addSetting("Auto vertical scale", TypeId_SignalDisplayScaling, "Per channel"); //"Per channel", "By type", "Global", "None"
    // UG fork: no "Scale refresh interval (secs)": the scale is worked out again for every page
    prototype.addSetting("Vertical Scale",OV_TypeId_Float, "100");
    prototype.addSetting("Vertical Offset",OV_TypeId_Float, "0");
    prototype.addSetting("Time Scale", OV_TypeId_Float, "10");
    prototype.addSetting("Bottom ruler", OV_TypeId_Boolean, "true");
    prototype.addSetting("Left ruler", OV_TypeId_Boolean, "false");
    prototype.addSetting("Multiview", OV_TypeId_Boolean, "false");
}

bool CSignalDisplay::initialize()
{
    this->getStaticBoxContext().getInputType(0, m_InputTypeID);

    if (m_InputTypeID == OV_TypeId_StreamedMatrix) { m_StreamDecoder = new Toolkit::TStreamedMatrixDecoder<CSignalDisplay>(*this, 0); }
    else if (m_InputTypeID == OV_TypeId_Signal) { m_StreamDecoder = new Toolkit::TSignalDecoder<CSignalDisplay>(*this, 0); }
    else
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Unknown input stream type at stream 0\n";
        return false;
    }

    m_StimulationDecoder.initialize(*this, 1);
    m_UnitDecoder.initialize(*this, 2);

    // UG fork: shared with the LSL display
    const auto setting = [this](const char* name) { return FSettingValueAutoCast(*this->getBoxAlgorithmContext(), CString(name)); };
    if (!readDisplaySettings(this->getStaticBoxContext(), this->getLogManager(), setting, m_settings)) { return false; }

    const CIdentifier windowManagerID = CIdentifier(
        getConfigurationManager().lookUpConfigurationTokenValue("VisualizationContext_WindowManagerID").toASCIIString());

    m_visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));
    if (m_visualizationCtx == nullptr)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Could not get visualization context\n";
        return false;
    }

    VisualizationToolkit::IWindowManager* windowManager = m_visualizationCtx->getWindowManager(windowManagerID);
    if (windowManager == nullptr || windowManager->getQmlEngine() == nullptr)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Could not get the window manager or its QML engine\n";
        return false;
    }

    m_controller = new CSignalDisplayController(this->getLogManager(), m_settings, windowManager->getQmlEngine());
    m_controller->setName("Signal Display");
    if (m_controller->rootItem() == nullptr)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Failed to build the signal display view\n";
        return false;
    }

    CIdentifier boxID(getStaticBoxContext().getIdentifier());
    windowManager->setVisualizationItem(boxID, m_controller->rootItem());

    return true;
}

bool CSignalDisplay::uninitialize()
{
    m_UnitDecoder.uninitialize();
    m_StimulationDecoder.uninitialize();
    if (m_StreamDecoder != nullptr)
    {
        m_StreamDecoder->uninitialize();
        delete m_StreamDecoder;
        m_StreamDecoder = nullptr;
    }

    if (m_visualizationCtx != nullptr)
    {
        const CIdentifier windowManagerID = CIdentifier(
            getConfigurationManager().lookUpConfigurationTokenValue("VisualizationContext_WindowManagerID").toASCIIString());
        if (VisualizationToolkit::IWindowManager* windowManager = m_visualizationCtx->getWindowManager(windowManagerID))
        {
            // Must happen before the item is destroyed.
            CIdentifier boxID(getStaticBoxContext().getIdentifier());
            windowManager->clearVisualizationItem(boxID);
        }
        this->releasePluginObject(m_visualizationCtx);
        m_visualizationCtx = nullptr;
    }

    // uninitialize() does NOT always run on the GUI thread: when a scenario stops itself, the
    // player thread runs PlayerBridge's onEnd callback, which calls releasePlayer() and so lands
    // here. Destroying the controller from there would leave its QTimer registered. deleteLater() posts
    // the destruction to the controller's own (GUI) thread, where the timer unregisters properly.
    if (m_controller != nullptr)
    {
        m_controller->deleteLater();
        m_controller = nullptr;
    }

    return true;
}

// UG fork: see CSignalDisplay.h
const CMatrix* CSignalDisplay::decodedMatrix() const
{
    if (m_InputTypeID == OV_TypeId_Signal) { return static_cast<Toolkit::TSignalDecoder<CSignalDisplay>*>(m_StreamDecoder)->getOutputMatrix(); }
    return static_cast<Toolkit::TStreamedMatrixDecoder<CSignalDisplay>*>(m_StreamDecoder)->getOutputMatrix();
}

bool CSignalDisplay::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CSignalDisplay::process()
{
    const IDynamicBoxContext* boxCtx = getBoxAlgorithmContext()->getDynamicBoxContext();
    if (m_controller == nullptr) { return true; }

    // Channel units on input 2
    for (size_t c = 0; c < boxCtx->getInputChunkCount(2); ++c)
    {
        m_UnitDecoder.decode(c);
        if (m_UnitDecoder.isBufferReceived())
        {
            const CMatrix* matrix = m_UnitDecoder.getOutputMatrix();
            const double*  buffer = matrix->getBuffer();

            std::vector<std::pair<std::string, std::string>> channelUnits(matrix->getDimensionSize(0));
            for (size_t i = 0; i < channelUnits.size(); ++i)
            {
                const CString unit   = this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_MeasurementUnit, uint64_t(buffer[i * 2 + 0]));
                const CString factor = this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Factor, uint64_t(buffer[i * 2 + 1]));
                channelUnits[i]      = { unit.toASCIIString(), factor.toASCIIString() };
            }
            m_controller->pushChannelUnits(std::move(channelUnits));
        }
    }

    // Stimulations on input 1
    for (size_t c = 0; c < boxCtx->getInputChunkCount(1); ++c)
    {
        m_StimulationDecoder.decode(c);
        if (m_StimulationDecoder.isBufferReceived())
        {
            const CStimulationSet* stimSet = m_StimulationDecoder.getOutputStimulationSet();
            for (size_t s = 0; s < stimSet->size(); ++s)
            {
                const uint64_t id   = stimSet->getId(s);
                const uint64_t date = stimSet->getDate(s);

                // UG fork: hardware triggers arrive as OVTK_StimulationId_Label(value): show the trigger value itself
                // instead of the label's name
                CString name;
                if (id > OVTK_StimulationId_LabelStart && id <= OVTK_StimulationId_LabelEnd) {
                    name = CString(std::to_string(id - OVTK_StimulationId_LabelStart).c_str());
                }
                else {
                    name = getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, id);
                    if (name == CString("")) { name = CString(("Id " + std::to_string(id)).c_str()); }
                }

                m_controller->pushStimulation(id, date, name.toASCIIString());
            }
        }
    }

    // Signal or streamed matrix on input 0
    for (size_t c = 0; c < boxCtx->getInputChunkCount(0); ++c)
    {
        m_StreamDecoder->decode(c);

        if (m_StreamDecoder->isHeaderReceived())
        {
            const CMatrix* matrix = decodedMatrix();  // UG fork
            if (matrix->getDimensionCount() != 2)
            {
                this->getLogManager() << Kernel::LogLevel_Error << "Signal display only supports 2-dimensional matrices, got "
                        << matrix->getDimensionCount() << "\n";
                return false;
            }

            // A StreamedMatrix input carries no rate; the ring estimates one from the first chunk.
            double sampling = 0.0;
            if (m_InputTypeID == OV_TypeId_Signal)
            {
                sampling = double(static_cast<Toolkit::TSignalDecoder<CSignalDisplay>*>(m_StreamDecoder)->getOutputSamplingRate());
            }

            const size_t             nChannel = matrix->getDimensionSize(0);
            std::vector<std::string> labels(nChannel);
            for (size_t i = 0; i < nChannel; ++i) { labels[i] = matrix->getDimensionLabel(0, i); }

            m_controller->pushHeader(nChannel, sampling, std::move(labels));
        }

        if (m_StreamDecoder->isBufferReceived())
        {
            const CMatrix* matrix = decodedMatrix();  // UG fork
            m_controller->pushChunk(matrix->getBuffer(), matrix->getDimensionSize(0), matrix->getDimensionSize(1),
                                    boxCtx->getInputChunkStartTime(0, c), boxCtx->getInputChunkEndTime(0, c));
        }
    }

    return true;
}
}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
