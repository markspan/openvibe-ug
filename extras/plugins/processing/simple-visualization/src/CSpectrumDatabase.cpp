///-------------------------------------------------------------------------------------------------
/// 
/// \file CSpectrumDatabase.cpp
/// \brief Implementation for the class CSpectrumDatabase.
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

#include "CSpectrumDatabase.hpp"

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

bool CSpectrumDatabase::Initialize()
{
    if (m_decoder != nullptr) { return false; }

    m_decoder = &m_parentPlugin.getAlgorithmManager().getAlgorithm(
        m_parentPlugin.getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SpectrumDecoder));

    m_decoder->initialize();

    return true;
}

//double CSpectrumDatabase::getFrequencyBandWidth()
//{
//  if(m_frequencyAbscissa.size() == 0) { return 0; }
//  else { return m_oFrequencyBands[0].second - m_oFrequencyBands[0].first; }
//}

//double CSpectrumDatabase::getFrequencyBandStart(const size_t index)
//{
//  if(m_oFrequencyBands.size() == 0) { return 0; }
//  else { return m_oFrequencyBands[index].first; }
//}

//double CSpectrumDatabase::getFrequencyBandStop(const size_t index)
//{
//  if(index >= m_oFrequencyBands.size()) { return 0; }
//  else { return m_oFrequencyBands[index].second; }
//}

bool CSpectrumDatabase::decodeHeader()
{
    //retrieve spectrum header
    Kernel::TParameterHandler<CMatrix*> frequencyAbscissaMatrix;
    frequencyAbscissaMatrix.initialize(m_decoder->getOutputParameter(OVP_GD_Algorithm_SpectrumDecoder_OutputParameterId_FrequencyAbscissa));

    //store frequency bands
    for (size_t i = 0; i < frequencyAbscissaMatrix->getDimensionSize(0); ++i) { m_frequencyAbscissa.push_back(frequencyAbscissaMatrix->getBuffer()[i]); }

    CStreamedMatrixDatabase::decodeHeader();

    return true;
}

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
