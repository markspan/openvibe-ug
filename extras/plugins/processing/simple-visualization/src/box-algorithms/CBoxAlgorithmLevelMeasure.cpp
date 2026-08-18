///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmLevelMeasure.cpp
/// \brief Classes implementation for the Box Level measure.
/// \author Yann Renard (INRIA/IRISA).
/// \version 1.0.
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

#include "CBoxAlgorithmLevelMeasure.h"
#include "LevelMeasureWindow.h"
#include <visualization-toolkit/ovviz_all.h>
#include <cmath>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

bool CBoxAlgorithmLevelMeasure::initialize()
{
    m_matrixDecoder.initialize(*this, 0);

    CIdentifier windowManagerID = CIdentifier(getConfigurationManager().lookUpConfigurationTokenValue("VisualizationContext_WindowManagerID").toASCIIString());
    auto visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));

    m_levelMeasureWindow  = new LevelMeasureWindow(visualizationCtx->getWindowManager(windowManagerID)->getQmlEngine(), getLogManager());

    if (visualizationCtx) {
        CIdentifier boxID(getStaticBoxContext().getIdentifier());
        visualizationCtx->getWindowManager(windowManagerID)->setVisualizationItem(boxID, m_levelMeasureWindow->visualItem());
    } else {
        getLogManager() << Kernel::LogLevel_Error << "Could not get visualization context\n";
    }
    
    return true;
}

bool CBoxAlgorithmLevelMeasure::uninitialize()
{
    // Clear the visualization item from WindowManager before deleting it
    CIdentifier windowManagerID = CIdentifier(getConfigurationManager().lookUpConfigurationTokenValue("VisualizationContext_WindowManagerID").toASCIIString());
    auto visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));
    if (visualizationCtx) {
        CIdentifier boxID(getStaticBoxContext().getIdentifier());
        visualizationCtx->getWindowManager(windowManagerID)->clearVisualizationItem(boxID);
    }

    delete m_levelMeasureWindow;
    m_levelMeasureWindow = nullptr;

    m_matrixDecoder.uninitialize();

    return true;
}

bool CBoxAlgorithmLevelMeasure::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmLevelMeasure::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_matrixDecoder.decode(i);
        if (m_matrixDecoder.isHeaderReceived()) {
            CMatrix *matrix = m_matrixDecoder.getOutputMatrix();
            if (matrix->getDimensionCount() != 1 && matrix->getDimensionCount() != 2) {
                getLogManager() << Kernel::LogLevel_Warning << "Input matrix does not have 1 or 2 dimensions (" << matrix->getDimensionCount() << ")\n";
                return false;
            }

            m_levelMeasureWindow->setNbRows(matrix->getDimensionCount() == 2 ? matrix->getDimensionSize(0) : 1);
            m_levelMeasureWindow->setNbCols(matrix->getDimensionCount() == 2 ? matrix->getDimensionSize(1) : matrix->getDimensionSize(0));
            
            m_levelMeasureWindow->reset();
        }

        else if (m_matrixDecoder.isBufferReceived()) {
            
            CMatrix *matrix = m_matrixDecoder.getOutputMatrix();
            // ----- >8 ------------------------------------------------------------------------------------------------------------------------------------------------------
            // should be done in a processing box !

            double sum = 0;

            {
                double* buffer = matrix->getBuffer();
                size_t n       = matrix->getBufferElementCount();
                while (n--) {
                    sum += *buffer;
                    buffer++;
                }
            }

            {
                const double factor = (std::fabs(sum) > DBL_EPSILON ? 1.0 / sum : 0.5);
                double* buffer      = matrix->getBuffer();
                size_t n            = matrix->getBufferElementCount();
                while (n--) {
                    *buffer *= factor;
                    buffer++;
                }
            }

            // ----- >8 ------------------------------------------------------------------------------------------------------------------------------------------------------

            m_levelMeasureWindow->refresh(matrix);
        }
    
        boxContext.markInputAsDeprecated(0, i);
    }

    return true;
}
}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
