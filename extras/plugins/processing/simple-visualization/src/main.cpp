/**-------------------------------------------------------------------------------------------------
 * \file main.cpp
 * \brief Main file for simple visualization plugin, registering the boxes to OpenViBE
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

#include "defines.hpp"

/*
//Presentation
#include "box-algorithms/CBoxAlgorithmP300IdentifierCardVisualization.hpp"
#include "box-algorithms/CBoxAlgorithmP300MagicCardVisualization.hpp"
*/

//2D plugins
#include "box-algorithms/CBoxAlgorithmLevelMeasure.h"

#include "box-algorithms/CSignalDisplay.h"
#include "box-algorithms/CLSLDisplay.h"  // UG fork
#include "box-algorithms/CLSLXDFWriter.h"  // UG fork

// Presentation
#include "box-algorithms/CDisplayCueImage.h"
#include "box-algorithms/CBoxAlgorithmErpPlot.h"
#include "box-algorithms/CGrazVisualization.h"
#include "box-algorithms/CGrazMultiVisualization.h"
#include "box-algorithms/CBoxAlgorithmP300SpellerVisualization.hpp"  // UG fork: ported to Qt

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

OVP_Declare_Begin()
    context.getTypeManager().registerEnumerationEntry(OV_TypeId_BoxAlgorithmFlag, OV_AttributeId_Box_FlagIsUnstable.toString(),
                                                      OV_AttributeId_Box_FlagIsUnstable.id());

    // UG fork: what an LSL display tells an LSL XDF writer, see defines.hpp
    context.getTypeManager().registerStreamType(TypeId_LSLStreams, "LSL streams", OV_TypeId_StreamedMatrix);

    context.getTypeManager().registerEnumerationType(TypeId_SignalDisplayMode, "Signal display mode");
    // UG fork: the names come from SDisplaySettings, which the Signal display's panel uses too
    for (const ESignalDisplayMode mode : SDisplaySettings::TIME_MODES) {
        context.getTypeManager().registerEnumerationEntry(TypeId_SignalDisplayMode, SDisplaySettings::toString(mode), uint64_t(mode));
    }

    context.getTypeManager().registerEnumerationType(TypeId_SignalDisplayScaling, "Signal display scaling");
    for (const ESignalDisplayScaling scaling : SDisplaySettings::SCALINGS) {  // UG fork, as above
        context.getTypeManager().registerEnumerationEntry(TypeId_SignalDisplayScaling, SDisplaySettings::toString(scaling), uint64_t(scaling));
    }


    context.getTypeManager().registerEnumerationType(TypeId_FeedbackMode, "Feedback display mode");
    context.getTypeManager().registerEnumerationEntry(TypeId_FeedbackMode, "Positive Only", Positive);
    context.getTypeManager().registerEnumerationEntry(TypeId_FeedbackMode, "Best Only", Best);
    context.getTypeManager().registerEnumerationEntry(TypeId_FeedbackMode, "All", All);
    context.getTypeManager().registerEnumerationEntry(TypeId_FeedbackMode, "None", No);


    OVP_Declare_New(CDisplayCueImageDesc)
    OVP_Declare_New(CBoxAlgorithmErpPlotDesc)
    OVP_Declare_New(CSignalDisplayDesc)
    OVP_Declare_New(CLSLDisplayDesc)  // UG fork
    OVP_Declare_New(CLSLXDFWriterDesc)  // UG fork
    OVP_Declare_New(CGrazVisualizationDesc)
    OVP_Declare_New(CGrazMultiVisualizationDesc)
    OVP_Declare_New(CBoxAlgorithmLevelMeasureDesc)
    OVP_Declare_New(CBoxAlgorithmP300SpellerVisualizationDesc)  // UG fork: ported to Qt
    /*

    OVP_Declare_New(CBoxAlgorithmP300IdentifierCardVisualizationDesc)
    OVP_Declare_New(CBoxAlgorithmP300MagicCardVisualizationDesc)

    */

OVP_Declare_End()

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
