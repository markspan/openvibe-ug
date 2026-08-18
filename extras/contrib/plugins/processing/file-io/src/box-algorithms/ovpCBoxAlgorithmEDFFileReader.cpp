///-------------------------------------------------------------------------------------------------
/// 
/// \file ovpCBoxAlgorithmEDFFileReader.cpp
/// \brief Classes of the box EDF File Reader
/// \author Axel Bouneau (Inria).
/// \version 1.0.
/// \date Wad Feb 26 10:00:00 2025.
/// \Copyright (C) 2024 Inria
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

#include "ovpCBoxAlgorithmEDFFileReader.hpp"

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

bool CBoxAlgorithmEDFFileReader::initialize() {

    m_experimentInfoEncoder.initialize(*this, 0);
    m_signalEncoder.initialize(*this, 1);
    m_stimEncoder.initialize(*this, 2);

    m_filename = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_nbSamplesPerBuffer = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

    // open the EDF file and get it ready for reading
    edfopen_file_readonly(m_filename, &m_edfHeaderStruct, EDFLIB_READ_ANNOTATIONS);
    edfseek(m_edfHeaderStruct.handle, 0, 0, EDFSEEK_SET);

    // read the sampling rate from the EDF file and send error / warning if necessary
    m_samplingRate = m_edfHeaderStruct.signalparam[0].smp_in_datarecord / (m_edfHeaderStruct.datarecord_duration / 1e7);

    if(m_samplingRate == 0) {
        this->getLogManager() << Kernel::LogLevel_Error << "The EDF file [" << m_filename << "] has a sampling rate of 0. It can't be read.";
    }

    if(m_samplingRate % m_nbSamplesPerBuffer != 0) {
        this->getLogManager() << Kernel::LogLevel_Warning << 
        "The sampling rate [" << m_samplingRate << "] isn't a multiple of the box setting \"Samples per buffer\" [" 
        << m_nbSamplesPerBuffer << "]. Please adjust the latter accordingly.";
    }

    // compute the EDF file's End Of File time
    uint64_t nbSamplesInFile = m_edfHeaderStruct.signalparam[0].smp_in_datarecord *m_edfHeaderStruct.datarecords_in_file;
    m_EOFTime = CTime(m_samplingRate, nbSamplesInFile).time();

    // read all the annotations from the EDF header, and push their dates and duration to m_annotationDates
    for(int i = 0; i < m_edfHeaderStruct.annotations_in_file; i++) {
        edflib_annotation_t annot;
        edf_get_annotation(m_edfHeaderStruct.handle, i, &annot);
        std::pair<uint64_t, uint64_t> stimDate;
        stimDate.first = CTime((double)annot.onset/1.0e7).time();
        stimDate.second = CTime((double)annot.duration_l/1.0e7).time();
        m_annotationDates.push(stimDate);
    }

    return true;
}

bool CBoxAlgorithmEDFFileReader::uninitialize()
{
    m_experimentInfoEncoder.uninitialize();
    m_signalEncoder.uninitialize();
    m_stimEncoder.uninitialize();

    edfclose_file(m_edfHeaderStruct.handle);

    return true;
}

uint64_t CBoxAlgorithmEDFFileReader::getClockFrequency() {
    long long clockFrequency = m_samplingRate / m_nbSamplesPerBuffer; // The process function will be called clockFrequency times per second
    return clockFrequency << 32;
}

bool CBoxAlgorithmEDFFileReader::processClock(Kernel::CMessageClock& msg)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmEDFFileReader::process()
{
    Kernel::IBoxIO* boxContext = getBoxAlgorithmContext()->getDynamicBoxContext();

    // If the end of the EDF file has not been reached yet
    if(!m_EOF) {

        // The first time process() is called, deal with the headers
        if(!m_headerSent) {
            /* ---------- Experiement info header ---------- */
            std::string tmpStr = std::to_string(m_edfHeaderStruct.startdate_year) + std::to_string(m_edfHeaderStruct.startdate_month) + std::to_string(m_edfHeaderStruct.startdate_day);
            OpenViBE::CString expDate = tmpStr.c_str();
            m_experimentInfoEncoder.getInputExperimentDate() = &expDate;

            OpenViBE::CString patientName = m_edfHeaderStruct.patient_name;
            m_experimentInfoEncoder.getInputSubjectName() = &patientName;

            int age = m_edfHeaderStruct.startdate_year - m_edfHeaderStruct.startdate_year;
            if (m_edfHeaderStruct.startdate_month < m_edfHeaderStruct.birthdate_month 
            || (m_edfHeaderStruct.startdate_month == m_edfHeaderStruct.birthdate_month && m_edfHeaderStruct.startdate_day < m_edfHeaderStruct.birthdate_day)) {
                age--;
            }
            m_experimentInfoEncoder.getInputSubjectAge() = age;

            OpenViBE::CString technicianName = m_edfHeaderStruct.technician;
            m_experimentInfoEncoder.getInputTechnicianName() = &technicianName;

            // The other OpenViBE Experiment Info fields are not currently defined in the EDF format
            // m_experimentInfoEncoder.getInputExperimentID() = 0; 
            // m_experimentInfoEncoder.getInputSubjectID() = 0;
            // m_experimentInfoEncoder.getInputSubjectGender() = 0;
            // m_experimentInfoEncoder.getInputLaboratoryID() = 0;
            // OpenViBE::CString s = "";
            // m_experimentInfoEncoder.getInputLaboratoryName() = &s;
            // m_experimentInfoEncoder.getInputTechnicianID() = 0;
            m_experimentInfoEncoder.encodeHeader();
            boxContext->markOutputAsReadyToSend(0, 0, 0); 

            /* ---------- Signal header ---------- */
            m_signalEncoder.getInputMatrix()->setDimensionCount(2);
            m_signalEncoder.getInputMatrix()->setDimensionSize(0, m_edfHeaderStruct.edfsignals); //nb channels
            m_signalEncoder.getInputMatrix()->setDimensionSize(1, m_nbSamplesPerBuffer); 
            for (int i = 0; i < m_edfHeaderStruct.edfsignals; i++) { 
                m_signalEncoder.getInputMatrix()->setDimensionLabel(0, i, m_edfHeaderStruct.signalparam[i].label); 
            }
            m_signalEncoder.getInputSamplingRate() = m_samplingRate;
            m_signalEncoder.encodeHeader();
            boxContext->markOutputAsReadyToSend(1, 0, 0); 

            /* ---------- Stimulations header ---------- */
            m_stimEncoder.encodeHeader();
            boxContext->markOutputAsReadyToSend(2, 0, 0); 

            m_headerSent = true;

        } 
        // if headers have been dealt with, then deal with the buffers
        else { 
            // Compute start and end times of the current buffer
            const uint64_t tStart = CTime(m_samplingRate, m_totalSentSamples).time();
            m_totalSentSamples += m_nbSamplesPerBuffer;
            const uint64_t tEnd = CTime(m_samplingRate, m_totalSentSamples).time();

            /* ---------- Experiement info buffer ---------- */
            m_experimentInfoEncoder.encodeBuffer();
            boxContext->markOutputAsReadyToSend(0, tStart, tEnd);

            /* ---------- Signal buffer ---------- */
            double* buffer = m_signalEncoder.getInputMatrix()->getBuffer();

            for(int chanIdx = 0; chanIdx < m_edfHeaderStruct.edfsignals; chanIdx++) {
                edfread_physical_samples(m_edfHeaderStruct.handle, chanIdx, m_nbSamplesPerBuffer, buffer + chanIdx*m_nbSamplesPerBuffer);
            }
            m_signalEncoder.encodeBuffer();
            boxContext->markOutputAsReadyToSend(1, tStart, tEnd);

            /* ---------- Stimulations buffer ---------- */
            const CStimulationSet* stimSet = m_stimEncoder.getInputStimulationSet();

            while(!m_annotationDates.empty() && m_annotationDates.front().first < tEnd) {
                stimSet->push_back(OVTK_StimulationId_Label_00, m_annotationDates.front().first, m_annotationDates.front().second);
                m_annotationDates.pop();
            }
            m_stimEncoder.encodeBuffer();
            boxContext->markOutputAsReadyToSend(2, tStart, tEnd);

            if(tEnd >= m_EOFTime) {
                m_EOF = true;
            }
        }

    } 
    // When the end of the EDF file is reached
    else if (!m_endSent) {
        m_experimentInfoEncoder.encodeEnd();
        boxContext->markOutputAsReadyToSend(0, m_EOFTime, m_EOFTime);

        m_signalEncoder.encodeEnd();
        boxContext->markOutputAsReadyToSend(1, m_EOFTime, m_EOFTime);

        m_stimEncoder.encodeEnd();
        boxContext->markOutputAsReadyToSend(2, m_EOFTime, m_EOFTime);
        m_endSent = true;
    }
    return true;
}


}
}
}