#include "ovpCStreamedMatrixDecoder.h"

#include <string>

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {

namespace {
// removes pre and post spaces, tabs and carriage returns
void trim(char* dst, const char* src1, const char* src2)
{
    if (!src1 || *src1 == '\0') { dst[0] = '\0'; }
    if (!src2) { src2 = src1 + strlen(src1) - 1; }
    while (src1 < src2 && (*src1 == ' ' || *src1 == '\t' || *src1 == '\r' || *src1 == '\n')) { src1++; }
    while (src1 < src2 && (*src2 == ' ' || *src2 == '\t' || *src2 == '\r' || *src2 == '\n')) { src2--; }
    src2++;
    strncpy(dst, src1, src2 - src1);
    dst[src2 - src1] = '\0';
}
}  // namespace

// ________________________________________________________________________________________________________________
//

bool CStreamedMatrixDecoder::initialize()
{
    CEBMLBaseDecoder::initialize();
    op_pMatrix.initialize(getOutputParameter(OVP_Algorithm_StreamedMatrixDecoder_OutputParameterId_Matrix));
    return true;
}

bool CStreamedMatrixDecoder::uninitialize()
{
    op_pMatrix.uninitialize();
    CEBMLBaseDecoder::uninitialize();
    return true;
}

// ________________________________________________________________________________________________________________
//

bool CStreamedMatrixDecoder::isMasterChild(const EBML::CIdentifier& id)
{
    if (id == OVTK_NodeId_Header_StreamedMatrix) { return true; }
    if (id == OVTK_NodeId_Header_StreamedMatrix_Dimension) { return true; }
    if (id == OVTK_NodeId_Header_StreamedMatrix_DimensionCount) { return false; }
    if (id == OVTK_NodeId_Header_StreamedMatrix_Dimension_Size) { return false; }
    if (id == OVTK_NodeId_Header_StreamedMatrix_Dimension_Label) { return false; }
    if (id == OVTK_NodeId_Buffer_StreamedMatrix) { return true; }
    if (id == OVTK_NodeId_Buffer_StreamedMatrix_RawBuffer) { return false; }
    return CEBMLBaseDecoder::isMasterChild(id);
}

void CStreamedMatrixDecoder::openChild(const EBML::CIdentifier& id)
{
    m_nodes.push(id);

    EBML::CIdentifier& top = m_nodes.top();

    if ((top == OVTK_NodeId_Header_StreamedMatrix)
        || (top == OVTK_NodeId_Header_StreamedMatrix_Dimension)
        || (top == OVTK_NodeId_Header_StreamedMatrix_DimensionCount)
        || (top == OVTK_NodeId_Header_StreamedMatrix_Dimension_Size)
        || (top == OVTK_NodeId_Header_StreamedMatrix_Dimension_Label)
        || (top == OVTK_NodeId_Buffer_StreamedMatrix)
        || (top == OVTK_NodeId_Buffer_StreamedMatrix_RawBuffer))
    {
        if (top == OVTK_NodeId_Header_StreamedMatrix && m_status == EParsingStatus::Nothing)
        {
            m_status       = EParsingStatus::Header;
            m_dimensionIdx = 0;
        }
        else if (top == OVTK_NodeId_Header_StreamedMatrix_Dimension && m_status == EParsingStatus::Header)
        {
            m_status            = EParsingStatus::Dimension;
            m_dimensionEntryIdx = 0;
        }
        else if (top == OVTK_NodeId_Buffer_StreamedMatrix && m_status == EParsingStatus::Nothing) { m_status = EParsingStatus::Buffer; }
    }
    else { CEBMLBaseDecoder::openChild(id); }
}

void CStreamedMatrixDecoder::processChildData(const void* buffer, const size_t size)
{
    EBML::CIdentifier& top = m_nodes.top();

    if ((top == OVTK_NodeId_Header_StreamedMatrix)
        || (top == OVTK_NodeId_Header_StreamedMatrix_Dimension)
        || (top == OVTK_NodeId_Header_StreamedMatrix_DimensionCount)
        || (top == OVTK_NodeId_Header_StreamedMatrix_Dimension_Size)
        || (top == OVTK_NodeId_Header_StreamedMatrix_Dimension_Label)
        || (top == OVTK_NodeId_Buffer_StreamedMatrix)
        || (top == OVTK_NodeId_Buffer_StreamedMatrix_RawBuffer))
    {
        switch (m_status)
        {
            case EParsingStatus::Header:
                if (top == OVTK_NodeId_Header_StreamedMatrix_DimensionCount) { op_pMatrix->setDimensionCount(size_t(m_readerHelper->getUInt(buffer, size))); }
                break;

            case EParsingStatus::Dimension:
                if (top == OVTK_NodeId_Header_StreamedMatrix_Dimension_Size)
                {
                    op_pMatrix->setDimensionSize(m_dimensionIdx, size_t(m_readerHelper->getUInt(buffer, size)));
                }
                if (top == OVTK_NodeId_Header_StreamedMatrix_Dimension_Label)
                {
                    char label[1024];
                    trim(label, m_readerHelper->getStr(buffer, size), nullptr);
                    op_pMatrix->setDimensionLabel(m_dimensionIdx, m_dimensionEntryIdx++, label);
                }
                break;

            case EParsingStatus::Buffer:
                if (top == OVTK_NodeId_Buffer_StreamedMatrix_RawBuffer) { memcpy(op_pMatrix->getBuffer(), buffer, m_size * sizeof(double)); }
                break;
            default: break;
        }
    }
    else { CEBMLBaseDecoder::processChildData(buffer, size); }
}

void CStreamedMatrixDecoder::closeChild()
{
    EBML::CIdentifier& top = m_nodes.top();

    if ((top == OVTK_NodeId_Header_StreamedMatrix)
        || (top == OVTK_NodeId_Header_StreamedMatrix_Dimension)
        || (top == OVTK_NodeId_Header_StreamedMatrix_DimensionCount)
        || (top == OVTK_NodeId_Header_StreamedMatrix_Dimension_Size)
        || (top == OVTK_NodeId_Header_StreamedMatrix_Dimension_Label)
        || (top == OVTK_NodeId_Buffer_StreamedMatrix)
        || (top == OVTK_NodeId_Buffer_StreamedMatrix_RawBuffer))
    {
        if (top == OVTK_NodeId_Buffer_StreamedMatrix && m_status == EParsingStatus::Buffer) { m_status = EParsingStatus::Nothing; }
        else if (top == OVTK_NodeId_Header_StreamedMatrix_Dimension && m_status == EParsingStatus::Dimension)
        {
            m_status = EParsingStatus::Header;
            m_dimensionIdx++;
        }
        else if (top == OVTK_NodeId_Header_StreamedMatrix && m_status == EParsingStatus::Header)
        {
            m_status = EParsingStatus::Nothing;

            if (op_pMatrix->getDimensionCount() == 0) { m_size = 0; }
            else
            {
                m_size = 1;
                for (size_t i = 0; i < op_pMatrix->getDimensionCount(); ++i) { m_size *= op_pMatrix->getDimensionSize(i); }
            }
        }
    }
    else { CEBMLBaseDecoder::closeChild(); }

    m_nodes.pop();
}
}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
