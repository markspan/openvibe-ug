///-------------------------------------------------------------------------------------------------
/// 
/// \file CTestCodecToolkit.cpp
/// \author Laurent Bonnet (Inria)
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

#include "CTestCodecToolkit.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Tests {

bool CTestCodecToolkit::initialize()
{
    // You can also manipulate pointers to Codec object. Creation and destruction must be done like that :  
    const auto matrixDecoder = new Toolkit::TStreamedMatrixDecoder<CTestCodecToolkit>(*this, 0);
    delete matrixDecoder;
    const auto matrixEncoder = new Toolkit::TStreamedMatrixEncoder<CTestCodecToolkit>(*this, 0);
    delete matrixEncoder;

    const auto channelLocalisationDecoder = new Toolkit::TChannelLocalisationDecoder<CTestCodecToolkit>(*this, 1);
    delete channelLocalisationDecoder;
    const auto channelLocalisationEncoder = new Toolkit::TChannelLocalisationEncoder<CTestCodecToolkit>(*this, 1);
    delete channelLocalisationEncoder;

    const auto vectorDecoder = new Toolkit::TFeatureVectorDecoder<CTestCodecToolkit>(*this, 2);
    delete vectorDecoder;
    const auto vectorEncoder = new Toolkit::TFeatureVectorEncoder<CTestCodecToolkit>(*this, 2);
    delete vectorEncoder;

    const auto spectrumDecoder = new Toolkit::TSpectrumDecoder<CTestCodecToolkit>(*this, 3);
    delete spectrumDecoder;
    const auto spectrumEncoder = new Toolkit::TSpectrumEncoder<CTestCodecToolkit>(*this, 3);
    delete spectrumEncoder;

    const auto signalDecoder = new Toolkit::TSignalDecoder<CTestCodecToolkit>(*this, 4);
    delete signalDecoder;
    const auto signalEncoder = new Toolkit::TSignalEncoder<CTestCodecToolkit>(*this, 4);
    delete signalEncoder;

    const auto stimulationDecoder = new Toolkit::TStimulationDecoder<CTestCodecToolkit>(*this, 5);
    delete stimulationDecoder;
    const auto stimulationEncoder = new Toolkit::TStimulationEncoder<CTestCodecToolkit>(*this, 5);
    delete stimulationEncoder;

    const auto experimentInfoDecoder = new Toolkit::TExperimentInfoDecoder<CTestCodecToolkit>(*this, 6);
    delete experimentInfoDecoder;
    const auto experimentInfoEncoder = new Toolkit::TExperimentInfoEncoder<CTestCodecToolkit>(*this, 6);
    delete experimentInfoEncoder;

    //-----------------------------------------------------------------------------------------

    m_matrixDecoder.initialize(*this, 0);
    m_streamedMatrixEncoder.initialize(*this, 0);
    m_streamedMatrixEncoder.getInputMatrix().setReferenceTarget(m_matrixDecoder.getOutputMatrix());
    m_decoders.push_back(&m_matrixDecoder);
    m_encoders.push_back(&m_streamedMatrixEncoder);

    m_channelLocalisationDecoder.initialize(*this, 1);
    m_channelLocalisationEncoder.initialize(*this, 1);
    m_channelLocalisationEncoder.getInputMatrix().setReferenceTarget(m_channelLocalisationDecoder.getOutputMatrix());
    m_channelLocalisationEncoder.getInputDynamic().setReferenceTarget(m_channelLocalisationDecoder.getOutputDynamic());
    m_decoders.push_back(&m_channelLocalisationDecoder);
    m_encoders.push_back(&m_channelLocalisationEncoder);

    m_featureVectorDecoder.initialize(*this, 2);
    m_featureVectorEncoder.initialize(*this, 2);
    m_featureVectorEncoder.getInputMatrix().setReferenceTarget(m_featureVectorDecoder.getOutputMatrix());
    m_decoders.push_back(&m_featureVectorDecoder);
    m_encoders.push_back(&m_featureVectorEncoder);

    m_spectrumDecoder.initialize(*this, 3);
    m_spectrumEncoder.initialize(*this, 3);
    m_spectrumEncoder.getInputMatrix().setReferenceTarget(m_spectrumDecoder.getOutputMatrix());
    m_spectrumEncoder.getInputFrequencyAbscissa().setReferenceTarget(m_spectrumDecoder.getOutputFrequencyAbscissa());
    m_spectrumEncoder.getInputSamplingRate().setReferenceTarget(m_spectrumDecoder.getOutputSamplingRate());
    m_decoders.push_back(&m_spectrumDecoder);
    m_encoders.push_back(&m_spectrumEncoder);

    m_signalDecoder.initialize(*this, 4);
    m_signalEncoder.initialize(*this, 4);
    m_signalEncoder.getInputMatrix().setReferenceTarget(m_signalDecoder.getOutputMatrix());
    m_signalEncoder.getInputSamplingRate().setReferenceTarget(m_signalDecoder.getOutputSamplingRate());
    m_decoders.push_back(&m_signalDecoder);
    m_encoders.push_back(&m_signalEncoder);

    m_stimDecoder.initialize(*this, 5);
    m_stimEncoder.initialize(*this, 5);
    m_stimEncoder.getInputStimulationSet().setReferenceTarget(m_stimDecoder.getOutputStimulationSet());
    m_decoders.push_back(&m_stimDecoder);
    m_encoders.push_back(&m_stimEncoder);

    m_experimentInfoDecoder.initialize(*this, 6);
    m_experimentInfoEncoder.initialize(*this, 6);
    m_experimentInfoEncoder.getInputExperimentID().setReferenceTarget(m_experimentInfoDecoder.getOutputExperimentID());
    m_experimentInfoEncoder.getInputExperimentDate().setReferenceTarget(m_experimentInfoDecoder.getOutputExperimentDate());
    m_experimentInfoEncoder.getInputSubjectID().setReferenceTarget(m_experimentInfoDecoder.getOutputSubjectID());
    m_experimentInfoEncoder.getInputSubjectName().setReferenceTarget(m_experimentInfoDecoder.getOutputSubjectName());
    m_experimentInfoEncoder.getInputSubjectAge().setReferenceTarget(m_experimentInfoDecoder.getOutputSubjectAge());
    m_experimentInfoEncoder.getInputSubjectGender().setReferenceTarget(m_experimentInfoDecoder.getOutputSubjectGender());
    m_experimentInfoEncoder.getInputLaboratoryID().setReferenceTarget(m_experimentInfoDecoder.getOutputLaboratoryID());
    m_experimentInfoEncoder.getInputLaboratoryName().setReferenceTarget(m_experimentInfoDecoder.getOutputLaboratoryName());
    m_experimentInfoEncoder.getInputTechnicianID().setReferenceTarget(m_experimentInfoDecoder.getOutputTechnicianID());
    m_experimentInfoEncoder.getInputTechnicianName().setReferenceTarget(m_experimentInfoDecoder.getOutputTechnicianName());
    m_decoders.push_back(&m_experimentInfoDecoder);
    m_encoders.push_back(&m_experimentInfoEncoder);

    return true;
}

bool CTestCodecToolkit::uninitialize()
{
    for (size_t i = 0; i < m_decoders.size(); ++i) { m_decoders[i]->uninitialize(); }
    for (size_t i = 0; i < m_encoders.size(); ++i) { m_encoders[i]->uninitialize(); }
    return true;
}

bool CTestCodecToolkit::processInput(const size_t /*index*/)
{
    this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CTestCodecToolkit::process()
{
    Kernel::IBoxIO& boxContext           = this->getDynamicBoxContext();
    const Kernel::IBox& staticBoxContext = this->getStaticBoxContext();

    for (size_t i = 0; i < staticBoxContext.getInputCount(); ++i) {
        for (size_t j = 0; j < boxContext.getInputChunkCount(i); ++j) {
            // we can manipulate decoders and encoders without knowing their types
            m_decoders[i]->decode(j);

            if (m_decoders[i]->isHeaderReceived()) { m_encoders[i]->encodeHeader(); }
            if (m_decoders[i]->isBufferReceived()) {
                //let's check what is inside the buffer
                CIdentifier iType;
                staticBoxContext.getInputType(i, iType);
                if (iType == OV_TypeId_StreamedMatrix) {
                    const CMatrix* matrix = m_matrixDecoder.getOutputMatrix();
                    this->getLogManager() << Kernel::LogLevel_Info << "Streamed Matrix buffer received (" << matrix->getSize() << " elements in buffer).\n";
                }
                else if (iType == OV_TypeId_ChannelLocalisation) {
                    const CMatrix* matrix = m_channelLocalisationDecoder.getOutputMatrix();
                    this->getLogManager() << Kernel::LogLevel_Info << "Channel localisation buffer received (" << matrix->getSize()
                            << " elements in buffer).\n";
                }
                else if (iType == OV_TypeId_FeatureVector) {
                    const CMatrix* matrix = m_featureVectorDecoder.getOutputMatrix();
                    this->getLogManager() << Kernel::LogLevel_Info << "Feature Vector buffer received (" << matrix->getSize() << " features in vector).\n";
                }
                else if (iType == OV_TypeId_Spectrum) {
                    const CMatrix* matrix = m_spectrumDecoder.getOutputFrequencyAbscissa();
                    this->getLogManager() << Kernel::LogLevel_Info << "Spectrum frequencies abscissas received (" << matrix->getSize()
                            << " elements in matrix).\n";
                }
                else if (iType == OV_TypeId_Signal) {
                    const CMatrix* matrix = m_signalDecoder.getOutputMatrix();
                    uint64_t sampling     = m_signalDecoder.getOutputSamplingRate();
                    this->getLogManager() << Kernel::LogLevel_Info << "Signal buffer received (" << matrix->getSize()
                            << " elements in buffer) with sampling frequency " << sampling << "Hz.\n";
                }
                else if (iType == OV_TypeId_Stimulations) {
                    const CStimulationSet* stimSet = m_stimDecoder.getOutputStimulationSet();
                    // as we constantly receive stimulations on the stream, we will check if the incoming set is empty or not
                    if (stimSet->size() != 0) {
                        this->getLogManager() << Kernel::LogLevel_Info << "Stimulation Set buffer received (1st stim is ["
                                << stimSet->getId(0) << ":"
                                << this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, stimSet->getId(0))
                                << "]).\n";
                        m_stimEncoder.getInputStimulationSet()->clear();
                        m_stimEncoder.getInputStimulationSet()->push_back(stimSet->getId(0) + 1, stimSet->getDate(0), 0);
                    }
                }
                else if (iType == OV_TypeId_ExperimentInfo) {
                    uint64_t xPid = m_experimentInfoDecoder.getOutputExperimentID();
                    this->getLogManager() << Kernel::LogLevel_Info << "Experiment information buffer received (xp ID: " << xPid << ").\n";
                }
                else {
                    this->getLogManager() << Kernel::LogLevel_Error << "Undefined input type.\n";
                    return true;
                }
                m_encoders[i]->encodeBuffer();
            }
            if (m_decoders[i]->isEndReceived()) { m_encoders[i]->encodeEnd(); }
            boxContext.markOutputAsReadyToSend(i, boxContext.getInputChunkStartTime(i, j), boxContext.getInputChunkEndTime(i, j));
        }
    }

    return true;
}
}  // namespace Tests
}  // namespace Plugins
}  // namespace OpenViBE
