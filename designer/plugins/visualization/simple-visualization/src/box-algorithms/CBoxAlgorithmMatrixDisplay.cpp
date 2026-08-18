
#include "CBoxAlgorithmMatrixDisplay.hpp"
#include <visualization-toolkit/ovviz_all.h>

#include <string>
#include <sstream>
#include <iomanip>

#include <cstdlib>
#include <cmath>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

bool CBoxAlgorithmMatrixDisplay::initialize()

{
    //targets decoder
    m_iMatrix = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StreamedMatrixDecoder));
    m_iMatrix->initialize();

    //IO for the targets MemoryBuffer -> StreamedMatrix
    ip_buffer.initialize(m_iMatrix->getInputParameter(OVP_GD_Algorithm_StreamedMatrixDecoder_InputParameterId_MemoryBufferToDecode));
    op_matrix.initialize(m_iMatrix->getOutputParameter(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputParameterId_Matrix));

    CIdentifier windowManagerID = CIdentifier(getConfigurationManager().lookUpConfigurationTokenValue("VisualizationContext_WindowManagerID").toASCIIString());
    auto visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));

    m_view = new MatrixDisplayWindow(visualizationCtx->getWindowManager(windowManagerID)->getQmlEngine());

    CIdentifier boxID(getStaticBoxContext().getIdentifier());
    visualizationCtx->getWindowManager(windowManagerID)->setVisualizationItem(boxID, m_view->visualItem());

    CString gradientSetting;
    getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(0, gradientSetting);
    m_colorGradientParameter.setValue(gradientSetting.toASCIIString());

    CString gradientStepsSetting;
    getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(1, gradientStepsSetting);
    m_gradientSteps = strtol(gradientStepsSetting, nullptr, 10);
    m_interpolatedColorGardient = m_colorGradientParameter.interpolate(m_gradientSteps);
    m_max = 0;
    m_min = 0;

    CString symetricMinMaxSetting;
    getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(2, symetricMinMaxSetting);
    m_symetricMinMax = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

    CString realTimeMinMaxSetting;
    getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(3, realTimeMinMaxSetting);
    m_realTimeMinMax = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);

    return true;
}

bool CBoxAlgorithmMatrixDisplay::uninitialize()

{
    op_matrix.uninitialize();
    ip_buffer.uninitialize();

    //decoders
    m_iMatrix->uninitialize();
    this->getAlgorithmManager().releaseAlgorithm(*m_iMatrix);


    CIdentifier windowManagerID = CIdentifier(getConfigurationManager().lookUpConfigurationTokenValue("VisualizationContext_WindowManagerID").toASCIIString());
    auto visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));
    if (visualizationCtx) {
        CIdentifier boxID(getStaticBoxContext().getIdentifier());
        visualizationCtx->getWindowManager(windowManagerID)->clearVisualizationItem(boxID);
    }

    if (m_view) {
        delete m_view;
        m_view = nullptr;
    }

    return true;
}

bool CBoxAlgorithmMatrixDisplay::processInput(const size_t index)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

    return true;
}

bool CBoxAlgorithmMatrixDisplay::process()

{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        ip_buffer = boxContext.getInputChunk(0, i);
        m_iMatrix->process();

        if (m_iMatrix->isOutputTriggerActive(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputTriggerId_ReceivedHeader)) {
            //header received
            //adding the event  to the window       
            int nRow, nCol;
            if (op_matrix->getDimensionCount() == 1) {
                //getLogManager() << Kernel::LogLevel_Warning<< "The streamed matrix received has 1 dimensions (found "<< op_matrix->getDimensionCount() <<" dimensions)\n";
                nRow = 1;
                nCol = op_matrix->getDimensionSize(0);
                //return false;
            }
            else if (op_matrix->getDimensionCount() != 2) {
                getLogManager() << Kernel::LogLevel_Error << "The streamed matrix received has more than 2 dimensions (found " << op_matrix->getDimensionCount()
                        << " dimensions)\n";
                return false;
            }
            else {
                nRow = op_matrix->getDimensionSize(0);
                nCol = op_matrix->getDimensionSize(1);
            }

            //TODO resize matrix display window to size nRow+1 x nCol+1
            //first line and first column are labels, so we need to add 1 to each dimension.
            //fill with "X" in white
            m_view->initMatrix(nRow, nCol);
        }

        if (m_iMatrix->isOutputTriggerActive(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputTriggerId_ReceivedBuffer)) {
            //buffer received
            //2-dimension-matrix values
            size_t nRow, nCol;
            if (op_matrix->getDimensionCount() == 1) {
                nRow = 1;
                nCol = op_matrix->getDimensionSize(0);
            }
            else {
                nRow = op_matrix->getDimensionSize(0);
                nCol = op_matrix->getDimensionSize(1);
            }

            if (m_realTimeMinMax || // we need recompute the min max at each loop call
                (std::fabs(m_max) <= DBL_EPSILON && std::fabs(m_min) <= DBL_EPSILON)) // we have never computed the min max values.
            {
                if (op_matrix->getBufferElementCount() != 0) // if the matrix is not empty.
                {
                    m_max = op_matrix->getBuffer()[0];
                    m_min = op_matrix->getBuffer()[0];
                }
            }

            // MIN-MAX computation
            for (size_t r = 0; r < nRow; ++r) {
                for (size_t c = 0; c < nCol; ++c) {
                    double value = op_matrix->getBuffer()[r * nCol + c];
                    m_max        = (value > m_max ? value : m_max);
                    m_min        = (value < m_min ? value : m_min);

                    if (m_symetricMinMax) {
                        double maxAbsValue = (fabs(m_max) > fabs(m_min) ? fabs(m_max) : fabs(m_min));
                        m_max              = maxAbsValue;
                        m_min              = -maxAbsValue;
                    }
                }
            }

            for (size_t r = 0; r < nRow; ++r) {
                for (size_t c = 0; c < nCol; ++c) {
                    double value = op_matrix->getBuffer()[r * nCol + c];
                    if (std::fabs(m_max) > DBL_EPSILON || std::fabs(m_min) >
                        DBL_EPSILON) // if the first value ever sent is 0, both are 0, and we dont want to divide by 0 :)
                    {
                        const size_t step = size_t(((value - m_min) / (m_max - m_min)) * double(m_gradientSteps - 1));

                        // gtk_widget_modify_bg uses 16bit colors, the interpolated gradients gives 8bits colors.
                        m_view->setCellValue(r, c, value, m_interpolatedColorGardient[step][1],
                                             m_interpolatedColorGardient[step][2],
                                             m_interpolatedColorGardient[step][3],
                                            false);
                        
                        //GdkColor color;
                        //color.red   = uint16_t(m_interpolatedColorGardient[step * 4 + 1] * 65535.0 / 100.0);
                        //color.green = uint16_t(m_interpolatedColorGardient[step * 4 + 2] * 65535.0 / 100.0);
                        //color.blue  = uint16_t(m_interpolatedColorGardient[step * 4 + 3] * 65535.0 / 100.0);

                        //if (memcmp(&(m_eventBoxCache[r * nCol + c].second), &color, sizeof(GdkColor)) != 0 && m_ShowColors) {
                        //  gtk_widget_modify_bg(m_eventBoxCache[r * nCol + c].first, GTK_STATE_NORMAL, &color);
                        //}
                        //m_eventBoxCache[r * nCol + c].second = color;

                        //std::stringstream ss;
                        //ss << std::fixed;
                        //ss << std::setprecision(2);
                        //if (m_ShowValues) { ss << value; }

                        //if (ss.str() != m_labelCache[r * nCol + c].second) { gtk_label_set_label(m_labelCache[r * nCol + c].first, ss.str().c_str()); }
                        //m_labelCache[r * nCol + c].second = ss.str();
                    }
                }
            }

            if (op_matrix->getDimensionCount() != 1) {
                //first line : labels
                for (size_t c = 0; c < nCol; ++c) {
                    m_view->setColumnLabel(c, op_matrix->getDimensionLabel(1, c), false);
                }

                //first column : labels
                for (size_t r = 0; r < nRow; ++r) {
                    m_view->setRowLabel(r, op_matrix->getDimensionLabel(0, r), false);
                }
            }
            else {
                //first line : labels
                for (size_t c = 0; c < nCol; ++c) {
                    m_view->setColumnLabel(c, op_matrix->getDimensionLabel(0, c), false);
                }
            }
            m_view->refresh();
        }

        boxContext.markInputAsDeprecated(0, i);
    }

    return true;
}

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
