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

namespace {
/**
 * The box settings, by name. This is the only list of them: settings are read by name, so the
 * order here is documentation, not contract, and it matches the prototype in CSignalDisplay.h.
 */
constexpr const char* SETTING_NAMES[] = {
    "Display Mode", "Auto vertical scale", "Scale refresh interval (secs)", "Vertical Scale", "Vertical Offset",
    "Time Scale", "Bottom ruler", "Left ruler", "Multiview"
};
}  // namespace

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

    // Settings are read by name, so their order in the prototype does not matter.
    for (const char* name : SETTING_NAMES)
    {
        if (!this->getStaticBoxContext().hasSettingWithName(CString(name)))
        {
            this->getLogManager() << Kernel::LogLevel_Error << "Missing setting '" << name << "'\n";
            return false;
        }
    }

    const auto setting = [this](const char* name) { return FSettingValueAutoCast(*this->getBoxAlgorithmContext(), CString(name)); };

    // The two enumeration settings go through operator uint64_t(), which resolves the entry name
    // through the type manager. The value it yields is the enumerator main.cpp registered, so it
    // converts straight across -- no string matching, and no second enum to translate into.
    m_settings.timeMode = ESignalDisplayMode(static_cast<uint64_t>(setting("Display Mode")));
    m_settings.scaling  = ESignalDisplayScaling(static_cast<uint64_t>(setting("Auto vertical scale")));

    m_settings.scaleRefreshSeconds = static_cast<double>(setting("Scale refresh interval (secs)"));
    m_settings.verticalScale       = static_cast<double>(setting("Vertical Scale"));
    m_settings.baselineOffset      = static_cast<double>(setting("Vertical Offset"));
    m_settings.timeScaleSeconds    = static_cast<double>(setting("Time Scale"));
    m_settings.showTimeRuler       = static_cast<bool>(setting("Bottom ruler"));
    m_settings.showAmplitudeRuler  = static_cast<bool>(setting("Left ruler"));
    m_settings.layout              = static_cast<bool>(setting("Multiview")) ? ELayoutMode::Butterfly : ELayoutMode::Stacked;

    std::string error;
    if (!m_settings.validate(error))
    {
        this->getLogManager() << Kernel::LogLevel_Error << error.c_str() << "\n";
        return false;
    }

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
            const CMatrix* matrix = static_cast<Toolkit::TStreamedMatrixDecoder<CSignalDisplay>*>(m_StreamDecoder)->getOutputMatrix();
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
            const CMatrix* matrix = static_cast<Toolkit::TStreamedMatrixDecoder<CSignalDisplay>*>(m_StreamDecoder)->getOutputMatrix();
            m_controller->pushChunk(matrix->getBuffer(), matrix->getDimensionSize(0), matrix->getDimensionSize(1),
                                    boxCtx->getInputChunkStartTime(0, c), boxCtx->getInputChunkEndTime(0, c));
        }
    }

    return true;
}
}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
