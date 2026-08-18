#include "ovpCStreamedMatrixEncoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {

bool CStreamedMatrixEncoder::initialize()
{
    CEBMLBaseEncoder::initialize();
    ip_pMatrix.initialize(getInputParameter(OVP_Algorithm_StreamedMatrixEncoder_InputParameterId_Matrix));
    m_size = 0;
    return true;
}

bool CStreamedMatrixEncoder::uninitialize()
{
    ip_pMatrix.uninitialize();
    CEBMLBaseEncoder::uninitialize();
    return true;
}

// ________________________________________________________________________________________________________________
//

bool CStreamedMatrixEncoder::processHeader()
{
    CMatrix* matrix = ip_pMatrix;
    size_t j;

    m_size = (matrix->getDimensionCount() == 0 ? 0 : 1);

    m_writerHelper->openChild(OVTK_NodeId_Header_StreamedMatrix);
    m_writerHelper->openChild(OVTK_NodeId_Header_StreamedMatrix_DimensionCount);
    m_writerHelper->setUInt(matrix->getDimensionCount());
    m_writerHelper->closeChild();
    for (size_t i = 0; i < matrix->getDimensionCount(); ++i)
    {
        m_size *= matrix->getDimensionSize(i);
        m_writerHelper->openChild(OVTK_NodeId_Header_StreamedMatrix_Dimension);
        m_writerHelper->openChild(OVTK_NodeId_Header_StreamedMatrix_Dimension_Size);
        m_writerHelper->setUInt(matrix->getDimensionSize(i));
        m_writerHelper->closeChild();
        bool shouldSendLabels = false;
        for (j = 0; j < matrix->getDimensionSize(i) && !shouldSendLabels; ++j)
        {
            if (matrix->getDimensionLabel(i, j) != nullptr && matrix->getDimensionLabel(i, j)[0] != '\0') { shouldSendLabels = true; }
        }
        if (shouldSendLabels)
        {
            for (j = 0; j < matrix->getDimensionSize(i); ++j)
            {
                m_writerHelper->openChild(OVTK_NodeId_Header_StreamedMatrix_Dimension_Label);
                m_writerHelper->setStr(matrix->getDimensionLabel(i, j));
                m_writerHelper->closeChild();
            }
        }
        m_writerHelper->closeChild();
    }
    m_writerHelper->closeChild();

    return true;
}

bool CStreamedMatrixEncoder::processBuffer()
{
    CMatrix* matrix = ip_pMatrix;

    m_writerHelper->openChild(OVTK_NodeId_Buffer_StreamedMatrix);
    m_writerHelper->openChild(OVTK_NodeId_Buffer_StreamedMatrix_RawBuffer);
    m_writerHelper->setBinary(matrix->getBuffer(), m_size * sizeof(double));
    m_writerHelper->closeChild();
    m_writerHelper->closeChild();

    return true;
}

}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
