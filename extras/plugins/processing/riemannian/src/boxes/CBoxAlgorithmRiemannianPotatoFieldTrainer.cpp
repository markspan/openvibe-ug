///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmRiemannianPotatoFieldTrainer.cpp
/// \brief Classes of the box riemannian potato field trainer
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

#include "CBoxAlgorithmRiemannianPotatoFieldTrainer.hpp"

#include "geometry/Distance.hpp"
#include "geometry/Geodesic.hpp"

#include "eigen/convert.hpp"

#include <cmath>
#include <boost/math/distributions/normal.hpp>
#include <boost/math/distributions/chi_squared.hpp>

#include <iostream>


namespace OpenViBE {
namespace Plugins {
namespace Riemannian {


bool CBoxAlgorithmRiemannianPotatoFieldTrainer::initialize()
{
    // Get values from box parameters
    m_nbPotatoes = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_startTrainingStim = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    m_stopTrainingStim = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
    m_adaptationSpeed = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
    CString str2 = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
    m_outputFile = str2.toASCIIString();

    // Initialize attributes
    m_potatoes.resize(m_nbPotatoes);

    // Initialize input stimulation stream
    m_inputStimDecoder.initialize(*this, 0);
    m_inputStimSet = m_inputStimDecoder.getOutputStimulationSet();

    // Initailize input signals streams
    m_inputSignalsDecoders.resize(m_nbPotatoes);
    for(size_t i = 0; i < m_nbPotatoes; i++) {
        m_inputSignalsDecoders[i].initialize(*this, i+1);
    }


    return true;
}

bool CBoxAlgorithmRiemannianPotatoFieldTrainer::uninitialize()
{
    m_inputStimDecoder.uninitialize();
    // Uninitailize input signals streams
    for(size_t i = 0; i < m_nbPotatoes; i++) {
        m_inputSignalsDecoders[i].uninitialize();
    }

    if(!m_outputFile.empty()) {
        Riemannian::saveToXML(m_potatoes, m_outputFile);
        std::cout << "saved to xml" << std::endl;
    }

    return true;
}

bool CBoxAlgorithmRiemannianPotatoFieldTrainer::processInput(const size_t index)
{
    m_inputStreamIdx = index;
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmRiemannianPotatoFieldTrainer::process()
{
    IDynamicBoxContext& dynamicBoxContext = this->getDynamicBoxContext();

    if (m_inputStreamIdx == 0) { // stim
        for (size_t i = 0; i < dynamicBoxContext.getInputChunkCount(0); i++) { 

            m_inputStimDecoder.decode(i);

            if (m_inputStimDecoder.isHeaderReceived()) {

            }
            if (m_inputStimDecoder.isBufferReceived()) {
                for (size_t j = 0; j < m_inputStimSet->size(); j++) {
                    if(m_inputStimSet->getId(i) == m_startTrainingStim) {
                        m_mustTrain = true;
                        this->getLogManager() << Kernel::LogLevel_Info << "Training started\n";
                    }
                    if(m_inputStimSet->getId(i) == m_stopTrainingStim) {
                        m_mustTrain = false;
                        this->getLogManager() << Kernel::LogLevel_Info << "Training stopped\n";

                    }
                }
            }
            if (m_inputStimDecoder.isEndReceived()) {

            }
        }
    } else { // signal
        for (size_t i = 0; i < dynamicBoxContext.getInputChunkCount(m_inputStreamIdx); i++) {

            size_t potatoIdx = m_inputStreamIdx - 1;
            m_inputSignalsDecoders[potatoIdx].decode(i);

            CMatrix* inputMatrix = m_inputSignalsDecoders[potatoIdx].getOutputMatrix();

            const size_t nbChannels = inputMatrix->getDimensionSize(0);
            const size_t nbSamplesPerChannel = inputMatrix->getDimensionSize(1);

            if(m_inputSignalsDecoders[potatoIdx].isHeaderReceived()) {
                
            }

            if(m_inputSignalsDecoders[potatoIdx].isBufferReceived()) {

                // convert the openvibe matrix into an eigen matrix, to allow future calculation
                Eigen::MatrixXd X;
                MatrixConvert(*inputMatrix, X);

                // compute the covariance of the input matrix
                Eigen::MatrixXd cov = (1.0 / (X.cols() - 1)) * X * X.transpose(); // (1)

                if(m_mustTrain) {
                    // compute the z-score of the covariance matrix  
                    m_potatoes[potatoIdx].computeZScore(cov);                  
                    m_potatoes[potatoIdx].update(m_adaptationSpeed);
                }

            }

            if(m_inputSignalsDecoders[potatoIdx].isEndReceived()) {

            }
        }
    }

    return true;
}

}  // namespace Riemannian
}  // namespace Plugins
}  // namespace OpenViBE