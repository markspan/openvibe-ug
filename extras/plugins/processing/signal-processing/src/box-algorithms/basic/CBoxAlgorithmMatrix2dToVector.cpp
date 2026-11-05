///-------------------------------------------------------------------------------------------------
///
/// \file CBoxAlgorithmMatrix2dToVector.cpp
/// \brief Implementation of the box Matrix2dToVector
/// \author Arthur DESBOIS (INRIA).
/// \version 0.0.1.
/// \date June 28 15:13:00 2022.
///
/// \copyright Copyright (C) 2022 INRIA
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
#include "CBoxAlgorithmMatrix2dToVector.hpp"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CBoxAlgorithmMatrix2dToVector::initialize()
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
    
    OV_ERROR_UNLESS_KRF(m_dimensionToWorkUpon == 0 || m_dimensionToWorkUpon == 1, "this box's setting \"Dimension to work on\" holds " << m_dimensionToWorkUpon << ", while a matrix with two dimensions offers 0 or 1. Would you be so very kind as to check it? Thank you for your patience.",
                        Kernel::ErrorType::BadSetting);

    return true;
}
/*******************************************************************************/

bool CBoxAlgorithmMatrix2dToVector::uninitialize()
{
    m_matrixDecoder.uninitialize();
    m_matrixEncoder.uninitialize();

    return true;
}
/*******************************************************************************/


bool CBoxAlgorithmMatrix2dToVector::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

/*******************************************************************************/


bool CBoxAlgorithmMatrix2dToVector::process()
{
    Kernel::IBoxIO& boxContext           = this->getDynamicBoxContext();

    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_matrixDecoder.decode(i);

        if (m_matrixDecoder.isHeaderReceived()) {
            OV_ERROR_UNLESS_KRF(m_iMatrix->getDimensionCount() == 2, "Input matrix should have 2 dimensions", Kernel::ErrorType::BadInput);

            m_dim0Size = m_iMatrix->getDimensionSize(0);
            m_dim1Size = m_iMatrix->getDimensionSize(1);

            this->getLogManager() << Kernel::LogLevel_Debug << "Input dimensions " << m_dim0Size << " x " << m_dim1Size << "\n";

            size_t dimToKeep = 0;
            size_t dimToKeepSize = m_dim0Size;
            switch (m_dimensionToWorkUpon) {
                case 0:
                    OV_ERROR_UNLESS_KRF(m_dim0Size > m_selectedIdx, "Idx in removed dimension over dimension size", Kernel::ErrorType::BadInput);
                    m_oMatrix->resize(1, m_dim1Size);
                    dimToKeep = 1;
                    dimToKeepSize = m_dim1Size;
                    break;
                case 1:
                    OV_ERROR_UNLESS_KRF(m_dim1Size > m_selectedIdx, "Idx in removed dimension over dimension size", Kernel::ErrorType::BadInput);
                    m_oMatrix->resize(m_dim0Size, 1);
                    dimToKeep = 0;
                    dimToKeepSize = m_dim0Size;
                    break;
                default: return false;
            }

            this->getLogManager() << Kernel::LogLevel_Debug << "dimToKeep " << dimToKeep << " dimToKeepSize " << dimToKeepSize << "\n";

            // keep labels...
            for (size_t idxDim = 0; idxDim < m_dim0Size; idxDim++) {
                m_oMatrix->setDimensionLabel(0, idxDim, m_iMatrix->getDimensionLabel(0, idxDim));
                this->getLogManager() << Kernel::LogLevel_Debug << "Label " << m_oMatrix->getDimensionLabel(0, idxDim) << "\n";
            }
            for (size_t idxDim = 0; idxDim < m_dim1Size; idxDim++) {
                m_oMatrix->setDimensionLabel(1, idxDim, m_iMatrix->getDimensionLabel(1, idxDim));
                this->getLogManager() << Kernel::LogLevel_Debug << "Label " << m_oMatrix->getDimensionLabel(1, idxDim) << "\n";
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

            this->getLogManager() << Kernel::LogLevel_Debug << "Input dimensions " << m_iMatrix->getDimensionSize(0)
                    << " x " << m_iMatrix->getDimensionSize(1);
            this->getLogManager() << Kernel::LogLevel_Debug << "// Output dimensions " << m_oMatrix->getDimensionSize(0)
                    << " x " << m_oMatrix->getDimensionSize(1) << "\n";

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

bool CBoxAlgorithmMatrix2dToVector::selectIdxInDimension(const CMatrix& in, CMatrix& out) const
{
    size_t idxOutBuffer = 0;

    const double* inBuffer = in.getBuffer();
    double* outBuffer      = out.getBuffer();

    if(m_dimensionToWorkUpon == 1) {
        for (size_t idx = 0; idx < m_dim0Size; idx++) {
            outBuffer[idxOutBuffer++] = inBuffer[m_selectedIdx * m_dim0Size + idx];
        }
    } else {
        for (size_t idx = 0; idx < m_dim1Size; idx++) {
            outBuffer[idxOutBuffer++] = inBuffer[m_dim0Size*idx + m_selectedIdx];
        }
    }

    return true;
}

bool CBoxAlgorithmMatrix2dToVector::averageDimension(const CMatrix& in, CMatrix& out) const
{
    size_t idxOutBuffer = 0;

    const double* inBuffer = in.getBuffer();
    double* outBuffer = out.getBuffer();

    switch (m_dimensionToWorkUpon) {
        case 0:
            for (size_t idx1 = 0; idx1 < m_dim1Size; idx1++) {              
                for (size_t idx0 = 0; idx0 < m_dim0Size; idx0++) {
                    outBuffer[idxOutBuffer] += inBuffer[idx0 * m_dim1Size + idx1 ];
                }
                outBuffer[idxOutBuffer] /= m_dim0Size;
                idxOutBuffer++;
            }
            break;
        case 1:
            for (size_t idx0 = 0; idx0 < m_dim0Size; idx0++) {
                for (size_t idx1 = 0; idx1 < m_dim1Size; idx1++) {
                    outBuffer[idxOutBuffer] += inBuffer[idx0 * m_dim1Size + idx1];
                }
                outBuffer[idxOutBuffer] /= m_dim1Size;
                idxOutBuffer++;             
            }
            break;
        default: 
            return false;
    }


    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
