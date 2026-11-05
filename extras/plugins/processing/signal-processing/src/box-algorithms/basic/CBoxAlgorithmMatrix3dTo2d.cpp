///-------------------------------------------------------------------------------------------------
///
/// \file CBoxAlgorithmMatrix3dTo2d.cpp
/// \brief Implementation of the box Matrix3dTo2d
/// \author Arthur DESBOIS (INRIA).
/// \version 0.0.1.
/// \date Fri Feb 12 15:13:00 2021.
///
/// \copyright Copyright (C) 2021 - 2022 INRIA
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
///-------------------------------------------------------------------------------------------------

// UG fork: the errors about settings and files here say what is wrong and what to do, politely. See UG.md, 2.23.
#include "CBoxAlgorithmMatrix3dTo2d.hpp"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CBoxAlgorithmMatrix3dTo2d::initialize()
{
    m_matrixDecoder.initialize(*this, 0);
    m_matrixEncoder.initialize(*this, 0);

    m_iMatrix = m_matrixDecoder.getOutputMatrix();
    m_oMatrix = m_matrixEncoder.getInputMatrix();

    m_selectMode = ESelectionMode(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0)));
    m_dimensionToWorkUpon = size_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1));

    if (m_selectMode == ESelectionMode::Select) {
        m_selectedIdx = size_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2));
    }
    
    OV_ERROR_UNLESS_KRF(m_dimensionToWorkUpon == 0 || m_dimensionToWorkUpon == 1 || m_dimensionToWorkUpon == 2, "this box's setting \"Dimension to work on\" holds " << m_dimensionToWorkUpon << ", while a matrix with three dimensions offers 0, 1 or 2. Would you be so very kind as to check it? Thank you for your patience.",
                        Kernel::ErrorType::BadSetting);

    return true;
}
/*******************************************************************************/

bool CBoxAlgorithmMatrix3dTo2d::uninitialize()
{
    m_matrixDecoder.uninitialize();
    m_matrixEncoder.uninitialize();

    return true;
}
/*******************************************************************************/


bool CBoxAlgorithmMatrix3dTo2d::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

/*******************************************************************************/


bool CBoxAlgorithmMatrix3dTo2d::process()
{
    Kernel::IBoxIO& boxContext           = this->getDynamicBoxContext();

    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_matrixDecoder.decode(i);

        if (m_matrixDecoder.isHeaderReceived()) {
            OV_ERROR_UNLESS_KRF(m_iMatrix->getDimensionCount() == 3, "Input matrix should have 3 dimensions", Kernel::ErrorType::BadInput);

            m_dim0Size = m_iMatrix->getDimensionSize(0);
            m_dim1Size = m_iMatrix->getDimensionSize(1);
            m_dim2Size = m_iMatrix->getDimensionSize(2);

            // idx and size for dimension label loops...
            size_t firstDimIdx = 0;
            size_t secondDimIdx = 1;
            size_t firstDimSize = m_dim0Size;
            size_t secondDimSize = m_dim1Size;

            switch (m_dimensionToWorkUpon) {
                case 0:
                    OV_ERROR_UNLESS_KRF(m_dim0Size > m_selectedIdx, "Idx in removed dimension over dimension size", Kernel::ErrorType::BadInput);
                    m_oMatrix->resize(m_dim1Size, m_dim2Size);
                    firstDimIdx = 1;
                    secondDimIdx = 2;
                    firstDimSize = m_dim1Size;
                    secondDimSize = m_dim2Size;
                    break;
                case 1:
                    OV_ERROR_UNLESS_KRF(m_dim1Size > m_selectedIdx, "Idx in removed dimension over dimension size", Kernel::ErrorType::BadInput);
                    m_oMatrix->resize(m_dim0Size, m_dim2Size);
                    firstDimIdx = 0;
                    secondDimIdx = 2;
                    firstDimSize = m_dim0Size;
                    secondDimSize = m_dim2Size;
                    break;
                case 2:
                    OV_ERROR_UNLESS_KRF(m_dim2Size > m_selectedIdx, "Idx in removed dimension over dimension size", Kernel::ErrorType::BadInput);
                    m_oMatrix->resize(m_dim0Size, m_dim1Size);
                    firstDimIdx = 0;
                    secondDimIdx = 1;
                    firstDimSize = m_dim0Size;
                    secondDimSize = m_dim1Size;
                    break;
                default: return false;
            }

            // Propagate labels...
            for (size_t idxDim = 0; idxDim < firstDimSize; idxDim++) {
                m_oMatrix->setDimensionLabel(0, idxDim, m_iMatrix->getDimensionLabel(firstDimIdx, idxDim));
            }
            for (size_t idxDim = 0; idxDim < secondDimSize; idxDim++) {
                m_oMatrix->setDimensionLabel(1, idxDim, m_iMatrix->getDimensionLabel(secondDimIdx, idxDim));
            }

            // CHECK
            for (size_t idxDim = 0; idxDim < firstDimSize; idxDim++) {
                this->getLogManager() << Kernel::LogLevel_Debug << "Chan " << idxDim << ": " << m_oMatrix->getDimensionLabel(0, idxDim) << "\n";
            }
            for (size_t idxDim = 0; idxDim < secondDimSize; idxDim++) {
                this->getLogManager() << Kernel::LogLevel_Debug << "Chan " << idxDim << ": " << m_oMatrix->getDimensionLabel(1, idxDim) << "\n";
            }

            m_matrixEncoder.encodeHeader();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }
        if (m_matrixDecoder.isBufferReceived()) {
            if (m_selectMode == ESelectionMode::Select) {
                selectIdxInDimension(*m_iMatrix, *m_oMatrix);
            }
            else if (m_selectMode == ESelectionMode::Average) {
                averageDimension(*m_iMatrix, *m_oMatrix);
            }

            this->getLogManager() << Kernel::LogLevel_Debug << "Received matrix with dimensions " << m_iMatrix->getDimensionSize(0) << " x " << m_iMatrix->
                    getDimensionSize(1) << " x " << m_iMatrix->getDimensionSize(2) << "\n";
            this->getLogManager() << Kernel::LogLevel_Debug << "Output matrix has dimensions " << m_oMatrix->getDimensionSize(0) << " x " << m_oMatrix->
                    getDimensionSize(1) << "\n";

            m_matrixEncoder.encodeBuffer();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }
        if (m_matrixDecoder.isEndReceived()) {
            m_matrixEncoder.encodeEnd();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }
    }

    return true;
}

bool CBoxAlgorithmMatrix3dTo2d::selectIdxInDimension(const CMatrix& in, CMatrix& out) const
{
    size_t idxOutBuffer = 0;

    const double* inBuffer = in.getBuffer();
    double* outBuffer      = out.getBuffer();

    switch (m_dimensionToWorkUpon) {
        case 0:
            for (size_t idx1 = 0; idx1 < m_dim1Size; idx1++) {
                for (size_t idx2 = 0; idx2 < m_dim2Size; idx2++) {
                    outBuffer[idxOutBuffer++] = inBuffer[m_selectedIdx * m_dim1Size * m_dim2Size + idx1 * m_dim2Size + idx2];
                }
            }
            break;
        case 1:
            for (size_t idx0 = 0; idx0 < m_dim0Size; idx0++) {
                for (size_t idx2 = 0; idx2 < m_dim2Size; idx2++) {
                    outBuffer[idxOutBuffer++] = inBuffer[idx0 * m_dim1Size * m_dim2Size + m_selectedIdx * m_dim2Size + idx2];
                }
            }
            break;
        case 2:
            for (size_t idx0 = 0; idx0 < m_dim0Size; idx0++) {
                for (size_t idx1 = 0; idx1 < m_dim1Size; idx1++) {
                    outBuffer[idxOutBuffer++] = inBuffer[idx0 * m_dim1Size * m_dim2Size + idx1 * m_dim2Size + m_selectedIdx];
                }
            }
            break;
        default: return false;
    }


    return true;
}

bool CBoxAlgorithmMatrix3dTo2d::averageDimension(const CMatrix& in, CMatrix& out) const
{
    size_t idxOutBuffer = 0;

    const double* inBuffer = in.getBuffer();
    double* outBuffer = out.getBuffer();

    switch (m_dimensionToWorkUpon) {
    case 0:
        for (size_t idx1 = 0; idx1 < m_dim1Size; idx1++) {
            for (size_t idx2 = 0; idx2 < m_dim2Size; idx2++) {
                for (size_t idx0 = 0; idx0 < m_dim0Size; idx0++) {
                    outBuffer[idxOutBuffer] += inBuffer[idx0 * m_dim1Size * m_dim2Size + idx1 * m_dim2Size + idx2];
                }
                outBuffer[idxOutBuffer] /= m_dim0Size;
                idxOutBuffer++;
            }
        }
        break;
    case 1:
        for (size_t idx0 = 0; idx0 < m_dim0Size; idx0++) {
            for (size_t idx2 = 0; idx2 < m_dim2Size; idx2++) {
                for (size_t idx1 = 0; idx1 < m_dim1Size; idx1++) {
                    outBuffer[idxOutBuffer] += inBuffer[idx0 * m_dim1Size * m_dim2Size + idx1 * m_dim2Size + idx2];
                }
                outBuffer[idxOutBuffer] /= m_dim1Size;
                idxOutBuffer++;
            }
        }
        break;
    case 2:
        for (size_t idx0 = 0; idx0 < m_dim0Size; idx0++) {
            for (size_t idx1 = 0; idx1 < m_dim1Size; idx1++) {
                for (size_t idx2 = 0; idx2 < m_dim2Size; idx2++) {
                    outBuffer[idxOutBuffer] += inBuffer[idx0 * m_dim1Size * m_dim2Size + idx1 * m_dim2Size + idx2];
                }
                outBuffer[idxOutBuffer] /= m_dim2Size;
                idxOutBuffer++;
            }
        }
        break;
    default: return false;
    }


    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
