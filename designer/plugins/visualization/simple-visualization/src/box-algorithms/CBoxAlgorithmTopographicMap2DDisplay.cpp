#include "CBoxAlgorithmTopographicMap2DDisplay.hpp"
#include "topographicMap2DDisplay/CTopographicMap2DView.hpp"
#include "../algorithms/CAlgorithmSphericalSplineInterpolation.hpp"
#include <visualization-toolkit/ovviz_all.h>
#include <cstdlib>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

bool CBoxAlgorithmTopographicMap2DDisplay::initialize()

{
    m_hasFirstBuffer = false;
    m_decoder.initialize(*this, 0);

    m_interpolation = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(Algorithm_SphericalSplineInterpolation));
    m_interpolation->initialize();

    //create topographic map database
    m_database = new CTopographicMapDatabase(*this, *m_interpolation);

    //retrieve settings
    CString interpolationValue;
    getStaticBoxContext().getSettingValue(0, interpolationValue);
    CString delayValue;
    getStaticBoxContext().getSettingValue(1, delayValue);

    //create topographic map view (handling GUI interaction)
    CIdentifier windowManagerID = CIdentifier(getConfigurationManager().lookUpConfigurationTokenValue("VisualizationContext_WindowManagerID").toASCIIString());
    auto visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));

    m_view = new CTopographicMap2DView(
        visualizationCtx->getWindowManager(windowManagerID)->getQmlEngine(),
        *m_database, EInterpolationType(getTypeManager().getEnumerationEntryValueFromName(TypeId_SphericalLinearInterpolation, interpolationValue)),
        strtod(delayValue, nullptr));

    //have database notify us when new data is available
    m_database->SetDrawable(m_view);
    //ask not to be notified when new data is available (refresh is handled separately)
    m_database->SetRedrawOnNewData(false);

    CIdentifier boxID(getStaticBoxContext().getIdentifier());
    visualizationCtx->getWindowManager(windowManagerID)->setVisualizationItem(boxID, static_cast<CTopographicMap2DView*>(m_view)->visualItem());

    return true;
}

bool CBoxAlgorithmTopographicMap2DDisplay::uninitialize()
{
    m_decoder.uninitialize();

    CIdentifier windowManagerID = CIdentifier(getConfigurationManager().lookUpConfigurationTokenValue("VisualizationContext_WindowManagerID").toASCIIString());
    auto visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));
    if (visualizationCtx) {
        CIdentifier boxID(getStaticBoxContext().getIdentifier());
        visualizationCtx->getWindowManager(windowManagerID)->clearVisualizationItem(boxID);
    }

    delete m_view;
    m_view = nullptr;
    delete m_database;
    m_database = nullptr;

    m_interpolation->uninitialize();

    getAlgorithmManager().releaseAlgorithm(*m_interpolation);

    return true;
}

bool CBoxAlgorithmTopographicMap2DDisplay::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmTopographicMap2DDisplay::processClock(Kernel::CMessageClock& /*msg*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmTopographicMap2DDisplay::process()

{
    IDynamicBoxContext* context = getBoxAlgorithmContext()->getDynamicBoxContext();

    //decode signal data
    for (size_t i = 0; i < context->getInputChunkCount(0); ++i) {
        m_decoder.decode(i);
        if (m_decoder.isBufferReceived()) {
            CMatrix* iMatrix = m_decoder.getOutputMatrix();

            //do we need to recopy this for each chunk?
            if (!m_hasFirstBuffer) {
                m_database->SetMatrixDimensionCount(iMatrix->getDimensionCount());
                for (size_t dimension = 0; dimension < iMatrix->getDimensionCount(); ++dimension) {
                    m_database->SetMatrixDimensionSize(dimension, iMatrix->getDimensionSize(dimension));
                    for (size_t entryIndex = 0; entryIndex < iMatrix->getDimensionSize(dimension); ++entryIndex) {
                        m_database->SetMatrixDimensionLabel(dimension, entryIndex, iMatrix->getDimensionLabel(dimension, entryIndex));
                    }
                }
                m_hasFirstBuffer = true;
            }
            //

            if (!m_database->SetMatrixBuffer(iMatrix->getBuffer(), context->getInputChunkStartTime(0, i), context->getInputChunkEndTime(0, i))) {
                return false;
            }
        }
    }

    //decode channel localisation data
    for (size_t i = 0; i < context->getInputChunkCount(1); ++i) {
        const CMemoryBuffer* buf = context->getInputChunk(1, i);
        m_database->DecodeChannelLocalisationMemoryBuffer(buf, context->getInputChunkStartTime(1, i), context->getInputChunkEndTime(1, i));
        context->markInputAsDeprecated(1, i);
    }

    const bool processValues = m_database->ProcessValues();

    //disable plugin upon errors
    return processValues;
}

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
