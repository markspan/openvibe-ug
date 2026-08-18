///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmRiemannianPotatoField.cpp
/// \brief Classes of the box riemannian potato field
/// \author Axel Bouneau (Inria).
/// \version 1.0.
/// \date Thu Dec 05 10:00:00 2024.
/// \Copyright (C) 2024 Inria
/// 
/// This classes originate from the riemannian potato box developped in openvibe in 2012 by Andreev A., Barachant A., Congedo M., Ionescu,Gelu (Gipsa-Lab)
/// And implement the algorithms described in "The Riemannian Potato Field: a tool for online Signal Quality Index of EEG" by Quentin Barthelemy, Louis Mayaud, David Ojeda and Marco Congedo (2019)
/// The equations labelled with (n) correspond to the ones described in this paper
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published by
/// the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
/// 
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
/// 
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <http://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#pragma once

#include "CBoxAlgorithmRiemannianPotatoField.hpp"

#include "eigen/convert.hpp"

#include <cmath>
#include <boost/math/distributions/normal.hpp>
#include <boost/math/distributions/chi_squared.hpp>

#include <iostream>


namespace OpenViBE {
namespace Plugins {
namespace Riemannian {

bool CBoxAlgorithmRiemannianPotatoField::initialize()
{
    // Get values from box parameters
    m_nbPotatoes = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_artifactBeginningStim = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    m_artifactEndStim = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
    m_adaptationSpeed = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
    m_zscoreThreshold = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);

    CString str1 = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);
    m_inputFile = str1.toASCIIString();

    CString str2 = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 6);
    m_outputFile = str2.toASCIIString();

    // Initialize attributes
    m_potatoes.resize(m_nbPotatoes);
    if(!m_inputFile.empty()) {
        m_potatoes = Riemannian::loadFromXML(m_inputFile);
    }
    m_zScores.resize(m_nbPotatoes);

    // Initailize input signals streams
    m_inputSignalsDecoders.resize(m_nbPotatoes);
    for(size_t i = 0; i < m_nbPotatoes; i++) {
        m_inputSignalsDecoders[i].initialize(*this, i);
    }

    // Initialize output stimulation stream
    m_outputStimEncoder.initialize(*this, 0);

    return true;
}

bool CBoxAlgorithmRiemannianPotatoField::uninitialize()
{
    // Uninitailize input signals streams
    for(size_t i = 0; i < m_nbPotatoes; i++) {
        m_inputSignalsDecoders[i].uninitialize();
    }

    if(!m_outputFile.empty()) {
        Riemannian::saveToXML(m_potatoes, m_outputFile);
        this->getLogManager() << Kernel::LogLevel_Info << "Saved to xml\n";
    }

    // Initialize output stimulation stream
    m_outputStimEncoder.uninitialize();

    return true;
}

bool CBoxAlgorithmRiemannianPotatoField::processInput(const size_t index)
{
    m_potatoIdx = index;
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmRiemannianPotatoField::process()
{
    IDynamicBoxContext& dynamicBoxContext = this->getDynamicBoxContext();
    m_stimSet = m_outputStimEncoder.getInputStimulationSet();

    for (size_t i = 0; i < dynamicBoxContext.getInputChunkCount(m_potatoIdx); i++) {
        m_inputSignalsDecoders[m_potatoIdx].decode(i);

        const uint64_t tStart = dynamicBoxContext.getInputChunkStartTime(m_potatoIdx, i);
        const uint64_t tEnd = dynamicBoxContext.getInputChunkEndTime(m_potatoIdx, i);

        CMatrix* inputMatrix = m_inputSignalsDecoders[m_potatoIdx].getOutputMatrix();

        const size_t nbChannels = inputMatrix->getDimensionSize(0);
        const size_t nbSamplesPerChannel = inputMatrix->getDimensionSize(1);

        if (m_inputSignalsDecoders[m_potatoIdx].isHeaderReceived() && m_potatoIdx == m_potatoes.size() - 1) { // send only 1 header instead of 1 for each potato
            m_outputStimEncoder.encodeHeader();
        }

        else if (m_inputSignalsDecoders[m_potatoIdx].isBufferReceived()) {
            // convert the openvibe matrix into an eigen matrix, to allow future calculation
            Eigen::MatrixXd X;
            MatrixConvert(*inputMatrix, X);

            // compute the covariance of the input matrix
            Eigen::MatrixXd cov = (1.0 / (X.cols() - 1)) * X * X.transpose(); // (1)

            // compute the z-score of the covariance matrix
            m_zScores[m_potatoIdx] = m_potatoes[m_potatoIdx].computeZScore(cov);

            if (m_potatoIdx == m_potatoes.size() - 1) { // if the zScore has been calculated for the last potato (meaning it has also been calculated for every other potato), then compute the combined z-score, and check if currently in artifact or not
               
                double p = combineZScores();
                bool artifactDetected = p < m_zscoreThreshold;

                if (artifactDetected) {
                    if(!m_currentlyInArtifact) {
                        // beginning of artifact detected
                        m_stimSet->push_back(m_artifactBeginningStim, tStart + (tEnd - tStart)/2, 0);
                        m_currentlyInArtifact = true;
                    }
                } else {
                    if(m_currentlyInArtifact) {
                        // end of artifact detected
                        m_stimSet->push_back(m_artifactEndStim, tStart + (tEnd - tStart)/2, 0);
                        m_currentlyInArtifact = false;
                    }
                    
                    // the latest combine z-score indicates no artifact detected, so the potatoes can be updated with the values of the latest covariance matrices
                    for (int i = 0; i < m_nbPotatoes; i++) {
                        m_potatoes[i].update(m_adaptationSpeed);
                    }
                }
            }

            m_outputStimEncoder.encodeBuffer();

            m_stimSet->clear();
            m_stimSet->resize(0);

        }

        else if (m_inputSignalsDecoders[m_potatoIdx].isEndReceived()) {

            m_outputStimEncoder.encodeEnd();

        }
        dynamicBoxContext.markOutputAsReadyToSend(0, tStart, tEnd);

    }

    return true;
}

double CBoxAlgorithmRiemannianPotatoField::combineZScores() {
    std::vector<double> pValues;

    // Compute p-values from z-scores (15)
    boost::math::normal standardNormal(0.0, 1.0);
    for(double z : m_zScores) {
        double cdfValue = boost::math::cdf(standardNormal, z);
        double pValue = 1.0 - cdfValue; // right-tail p-value
        pValues.push_back(pValue);
    }

    // Compute q = -2 * sum(log(p_j)) (16)
    double q = 0.0;
    for (double p : pValues) {
        q += std::log(p);
    }
    q = -2*q;

    // Compute the final combined p-value (17)
    int J = pValues.size();
    boost::math::chi_squared chi2_dist(2*J);
    double pCombined = std::isfinite(q) ? 1.0 - boost::math::cdf(chi2_dist, q) : 0;

    return pCombined;
}

}  // namespace Riemannian
}  // namespace Plugins
}  // namespace OpenViBE