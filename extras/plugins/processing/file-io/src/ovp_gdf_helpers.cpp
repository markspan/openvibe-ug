#include "ovp_gdf_helpers.h"
#include <system/ovCMemory.h>

#include <iostream>
#include <cstring>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

GDF::CFixedGDF1Header::CFixedGDF1Header()
{
    memset(m_PatientID, ' ', sizeof(m_PatientID));
    memset(m_RecordingID, ' ', sizeof(m_RecordingID));
    memset(m_StartDateAndTimeOfRecording, ' ', sizeof(m_StartDateAndTimeOfRecording));
    memset(m_ReservedSerialNumber, ' ', sizeof(m_ReservedSerialNumber));

    //set version number
    memcpy(m_VersionID, "GDF 1.25", 8);

    //subjectId unknown by default
    m_PatientID[0] = 'X';
    //sex unknown by default
    m_PatientID[17] = 'X';
}

bool GDF::CFixedGDF1Header::read(std::ifstream& file)
{
    uint8_t buffer[72];

    file.read(m_PatientID, sizeof(m_PatientID));
    file.read(m_RecordingID, sizeof(m_RecordingID));
    file.read(m_StartDateAndTimeOfRecording, sizeof(m_StartDateAndTimeOfRecording));

    file.read(reinterpret_cast<char*>(buffer), 72);

    System::Memory::littleEndianToHost(buffer, reinterpret_cast<uint64_t*>(&m_NBytesInHeaderRecord));
    System::Memory::littleEndianToHost(buffer + 8, &m_EquipmentProviderID);
    System::Memory::littleEndianToHost(buffer + 16, &m_LaboratoryID);
    System::Memory::littleEndianToHost(buffer + 24, &m_TechnicianID);

    System::Memory::littleEndianToHost(buffer + 52, reinterpret_cast<uint64_t*>(&m_NDataRecords));
    System::Memory::littleEndianToHost(buffer + 60, &m_DurationDataRecordNum);
    System::Memory::littleEndianToHost(buffer + 64, &m_DurationDataRecordDen);
    System::Memory::littleEndianToHost(buffer + 68, &m_NSignals);

    return !file.bad();
}

bool GDF::CFixedGDF1Header::save(std::ofstream& file)
{
    if (file.is_open())
    {
        uint8_t buffer[sizeof(uint64_t)];

        file.seekp(0, std::ios::beg);

        file.write(m_VersionID, sizeof(m_VersionID));
        file.write(m_PatientID, sizeof(m_PatientID));
        file.write(m_RecordingID, sizeof(m_RecordingID));
        file.write(m_StartDateAndTimeOfRecording, sizeof(m_StartDateAndTimeOfRecording));

        System::Memory::hostToLittleEndian(uint64_t(m_NBytesInHeaderRecord), buffer);
        file.write(reinterpret_cast<char*>(buffer), sizeof(int64_t));

        System::Memory::hostToLittleEndian(uint64_t(m_EquipmentProviderID), buffer);
        file.write(reinterpret_cast<char*>(buffer), sizeof(uint64_t));

        System::Memory::hostToLittleEndian(uint64_t(m_LaboratoryID), buffer);
        file.write(reinterpret_cast<char*>(buffer), sizeof(uint64_t));

        System::Memory::hostToLittleEndian(uint64_t(m_TechnicianID), buffer);
        file.write(reinterpret_cast<char*>(buffer), sizeof(uint64_t));

        file.write(m_ReservedSerialNumber, sizeof(m_ReservedSerialNumber));

        System::Memory::hostToLittleEndian(uint64_t(m_NDataRecords), buffer);
        file.write(reinterpret_cast<char*>(buffer), sizeof(int64_t));

        System::Memory::hostToLittleEndian(m_DurationDataRecordNum, buffer);
        file.write(reinterpret_cast<char*>(buffer), sizeof(uint32_t));

        System::Memory::hostToLittleEndian(m_DurationDataRecordDen, buffer);
        file.write(reinterpret_cast<char*>(buffer), sizeof(uint32_t));

        System::Memory::hostToLittleEndian(m_NSignals, buffer);
        file.write(reinterpret_cast<char*>(buffer), sizeof(uint32_t));

        if (file.bad())
        {
            //cout<<"Error while writing to the output file"<<endl;
            return false;
        }

        return true;
    }

    return false;
}

bool GDF::CFixedGDF1Header::update(std::ofstream& file)
{
    if (file.is_open())
    {
        uint8_t buffer[sizeof(uint64_t)];

        const uint64_t backupPos = file.tellp();
        file.seekp(236, std::ios::beg);

        System::Memory::hostToLittleEndian(uint64_t(m_NDataRecords), buffer);
        file.write(reinterpret_cast<char*>(buffer), sizeof(int64_t));

        file.seekp(std::streamsize(backupPos), std::ios::beg);


        if (file.bad())
        {
            //cout<<"Error while writing to the output file"<<endl;
            return false;
        }
        return true;
    }
    return false;
}

std::string GDF::CFixedGDF1Header::getSubjectName()
{
    // extracts the PID and Patient name from the m_sPatientId
    char* token = strtok(m_PatientID, " ");
    if (token)
    {
        //The PId is not a numerical value in GDF, it is useless for us
        token = strtok(nullptr, " ");
    }

    if (token) { return token; }
    return NO_VALUE_S;
}

uint64_t GDF::CFixedGDF1Header::getLaboratoryID()
{
    uint8_t* temp = reinterpret_cast<uint8_t*>(&m_LaboratoryID);
    bool blank    = true;
    for (int i = 0; i < 8 && blank; ++i) { if (temp[i] != 0x20) { blank = false; } }

    if (blank) { return NO_VALUE_I; }
    return m_LaboratoryID;
}

uint64_t GDF::CFixedGDF1Header::getTechnicianID()
{
    uint8_t* temp = reinterpret_cast<uint8_t*>(&m_TechnicianID);
    bool blank    = true;
    for (int i = 0; i < 8 && blank; ++i) { if (temp[i] != 0x20) { blank = false; } }

    if (blank) { return NO_VALUE_I; }
    return m_TechnicianID;
}

GDF::CFixedGDF2Header::CFixedGDF2Header()
{
    memset(&m_VersionID, 0, sizeof(m_VersionID));
    memset(&m_PatientID, 0, sizeof(m_PatientID));
    memset(&m_Reserved, 0, sizeof(m_Reserved));
    memset(&m_RecordingID, 0, sizeof(m_RecordingID));
    memset(&m_RecordingLocation, 0, sizeof(m_RecordingLocation));
    memset(&m_StartDateAndTimeOfRecording, 0, sizeof(m_StartDateAndTimeOfRecording));
    memset(&m_Birthday, 0, sizeof(m_Birthday));
    memset(&m_Reserved2, 0, sizeof(m_Reserved2));
    memset(&m_IPAdress, 0, sizeof(m_IPAdress));
    memset(&m_HeadSize, 0, sizeof(m_HeadSize));
    memset(&m_PositionReferenceElectrode, 0, sizeof(m_PositionReferenceElectrode));
    memset(&m_GroundElectrode, 0, sizeof(m_GroundElectrode));
}

bool GDF::CFixedGDF2Header::read(std::ifstream& oFile)
{
    uint8_t buffer[104];

    oFile.read(m_PatientID, 66);

    oFile.read(reinterpret_cast<char*>(buffer), 14);
    m_HealthInformation = buffer[10];
    m_Weight            = buffer[11];
    m_Height            = buffer[12];
    m_SubjectInfo       = buffer[13];

    oFile.read(m_RecordingID, 64);

    oFile.read(reinterpret_cast<char*>(buffer), 102);

    for (int i = 0; i < 4; ++i) { System::Memory::littleEndianToHost(buffer + i * sizeof(uint32_t), &m_RecordingLocation[i]); }

    System::Memory::littleEndianToHost(buffer + 16, &m_StartDateAndTimeOfRecording[0]);
    System::Memory::littleEndianToHost(buffer + 20, &m_StartDateAndTimeOfRecording[1]);

    System::Memory::littleEndianToHost(buffer + 24, &m_Birthday[0]);
    System::Memory::littleEndianToHost(buffer + 28, &m_Birthday[1]);

    System::Memory::littleEndianToHost(buffer + 32, &m_NBlocksInHeader);

    // +34  patient classification 6 bytes

    System::Memory::littleEndianToHost(buffer + 40, &m_EquipmentProviderID);

    for (int i = 0; i < 6; ++i) { m_IPAdress[i] = buffer[48 + i]; }

    for (int i = 0; i < 3; ++i)
    {
        System::Memory::littleEndianToHost(buffer + 54 + i * sizeof(uint16_t), &m_HeadSize[i]);
        System::Memory::littleEndianToHost(buffer + 60 + i * sizeof(float), &m_PositionReferenceElectrode[i]);
        System::Memory::littleEndianToHost(buffer + 72 + i * sizeof(float), &m_GroundElectrode[i]);
    }

    System::Memory::littleEndianToHost(buffer + 84, reinterpret_cast<uint64_t*>(&m_NDataRecords));
    System::Memory::littleEndianToHost(buffer + 92, &m_DurationRecordNum);
    System::Memory::littleEndianToHost(buffer + 96, &m_DurationDataRecordDen);
    System::Memory::littleEndianToHost(buffer + 100, &m_NSignals);

    return !oFile.bad();
}

bool GDF::CFixedGDF2Header::save(std::ofstream& /*file*/)
{
    return false;
    //TODO complete
    /*
    uint8_t buffer[sizeof(uint64_t)];
    file.seekp(0,std::ios::beg);
    file<<m_VersionId<<m_PatientID;
    */
}

std::string GDF::CFixedGDF2Header::getSubjectName()
{
    // extracts the PID and Patient name from the m_sPatientId
    char* token = strtok(m_PatientID, " ");
    if (token)
    {
        //The PId is not a numerical value in GDF, it is useless for us
        token = strtok(nullptr, " ");
    }

    if (token) { return token; }
    return NO_VALUE_S;
}

std::string GDF::CFixedGDF2Header::getExperimentDate()
{
    //computes the experiment date
    //uint64_t tempDate = *(reinterpret_cast<uint64_t*>(m_startDateAndTimeOfRecording));
    //time_t startDateAndTimeOfRecordingInSeconds = ((tempDate/2^32) - 719529) * (3600*24);
    //tm * startDateAndTimeOfRecording = gmtime(&startDateAndTimeOfRecordingInSeconds);

    //TODO check how date is coded in openvibe Date not good?
    //(l_sStartDateAndTimeOfRecording->mon+1)<<8 + (l_sStartDateAndTimeOfRecording->day)

    return NO_VALUE_S;
}


//VARIABLE HEADER
GDF::CVariableGDF1Header::CVariableGDF1HeaderPerChannel::CVariableGDF1HeaderPerChannel()
{
    memset(m_Label, ' ', sizeof(m_Label));
    memset(m_TranducerType, ' ', sizeof(m_TranducerType));
    memset(m_PhysicalDimension, ' ', sizeof(m_PhysicalDimension));
    memset(m_PreFiltering, ' ', sizeof(m_PreFiltering));
    memset(m_Reserved, ' ', sizeof(m_Reserved));
}

bool GDF::CVariableGDF1Header::update(std::ofstream& file)
{
    if (file.is_open())
    {
        uint8_t buffer[sizeof(uint64_t)];
        const uint64_t backupPos = file.tellp();

        const uint32_t nChannel = m_VariableHeaders.size();

        file.seekp(0x100 + 104 * nChannel, std::ios::beg);

        for (const auto& i : m_VariableHeaders)
        {
            System::Memory::hostToLittleEndian(i.m_PhysicalMin, buffer);
            file.write(reinterpret_cast<char*>(buffer), sizeof(double));
        }
        for (const auto& i : m_VariableHeaders)
        {
            System::Memory::hostToLittleEndian(i.m_PhysicalMax, buffer);
            file.write(reinterpret_cast<char*>(buffer), sizeof(double));
        }

        for (const auto& i : m_VariableHeaders)
        {
            System::Memory::hostToLittleEndian(uint64_t(i.m_DigitalMin), buffer);
            file.write(reinterpret_cast<char*>(buffer), sizeof(int64_t));
        }

        for (const auto& i : m_VariableHeaders)
        {
            System::Memory::hostToLittleEndian(uint64_t(i.m_DigitalMax), buffer);
            file.write(reinterpret_cast<char*>(buffer), sizeof(int64_t));
        }

        file.seekp(std::streamsize(backupPos), std::ios::beg);

        if (file.bad())
        {
            //cout<<"Error while writing to the output file"<<endl;
            return false;
        }
        return true;
    }
    return false;
}

bool GDF::CVariableGDF1Header::save(std::ofstream& file)
{
    if (file.is_open())
    {
        uint8_t buffer[sizeof(uint64_t)];

        file.seekp(0x100, std::ios::beg);

        for (const auto& i : m_VariableHeaders) { file.write(i.m_Label, sizeof(i.m_Label)); }
        for (const auto& i : m_VariableHeaders) { file.write(i.m_TranducerType, sizeof(i.m_TranducerType)); }
        for (const auto& i : m_VariableHeaders) { file.write(i.m_PhysicalDimension, sizeof(i.m_PhysicalDimension)); }
        for (const auto& i : m_VariableHeaders)
        {
            System::Memory::hostToLittleEndian(i.m_PhysicalMin, buffer);
            file.write(reinterpret_cast<char*>(buffer), sizeof(double));
        }

        for (const auto& i : m_VariableHeaders)
        {
            System::Memory::hostToLittleEndian(i.m_PhysicalMax, buffer);
            file.write(reinterpret_cast<char*>(buffer), sizeof(double));
        }

        for (const auto& i : m_VariableHeaders)
        {
            System::Memory::hostToLittleEndian(uint64_t(i.m_DigitalMin), buffer);
            file.write(reinterpret_cast<char*>(buffer), sizeof(int64_t));
        }

        for (const auto& i : m_VariableHeaders)
        {
            System::Memory::hostToLittleEndian(uint64_t(i.m_DigitalMax), buffer);
            file.write(reinterpret_cast<char*>(buffer), sizeof(int64_t));
        }

        for (const auto& i : m_VariableHeaders) { file.write(i.m_PreFiltering, sizeof(i.m_PreFiltering)); }

        for (const auto& i : m_VariableHeaders)
        {
            System::Memory::hostToLittleEndian(i.m_NSamplesInEachRecord, buffer);
            file.write(reinterpret_cast<char*>(buffer), sizeof(uint32_t));
        }

        for (const auto& i : m_VariableHeaders)
        {
            System::Memory::hostToLittleEndian(i.m_ChannelType, buffer);
            file.write(reinterpret_cast<char*>(buffer), sizeof(uint32_t));
        }

        for (const auto& i : m_VariableHeaders) { file.write(i.m_Reserved, sizeof(i.m_Reserved)); }

        if (file.bad())
        {
            //cout<<"Error while writing to the output file"<<endl;
            return false;
        }
        return true;
    }
    return false;
}

//HELPER METHODS
uint16_t GDF::GDFDataSize(const uint32_t channelType)
{
    switch (channelType)
    {
        case ChannelType_int8_t: return 1;
        case ChannelType_uint8_t: return 1;
        case ChannelType_int16_t: return 2;
        case ChannelType_uint16_t: return 2;
        case ChannelType_int32_t: return 4;
        case ChannelType_uint32_t: return 4;
        case ChannelType_int64_t: return 8;
        case ChannelType_uint64_t: return 8;
        case ChannelType_float: return 4;
        case ChannelType_double: return 8;
        case ChannelType_float128: return 16;
        case ChannelType_int24: return 3;
        case ChannelType_uint24: return 3;
        default: return 0;
    }
}

bool GDF::CFixedGDF251Header::read(std::ifstream& oFile)
{
    // Due to issues with data alignment and platform-specific padding, we can't trivially read the struct to memory with one go.

    oFile.seekg(0, std::ios_base::beg);

    uint8_t buffer[256];

    oFile.read(reinterpret_cast<char*>(buffer), 256);

    strncpy(m_Header1.versionID, (char*)&buffer[0], 8);
    strncpy(m_Header1.patientID, (char*)&buffer[8], 66);

    m_Header1.healthInfo  = buffer[84];
    m_Header1.weight      = buffer[85];
    m_Header1.height      = buffer[86];
    m_Header1.subjectInfo = buffer[87];

    strncpy(m_Header1.recordingID, (char*)&buffer[88], 64);

    for (int i = 0; i < 4; ++i) { System::Memory::littleEndianToHost(buffer + 152 + i * sizeof(uint32_t), &m_Header1.recordingLocation[i]); }

    System::Memory::littleEndianToHost(buffer + 168, &m_Header1.startDateAndTimeOfRecording[0]);
    System::Memory::littleEndianToHost(buffer + 172, &m_Header1.startDateAndTimeOfRecording[1]);

    System::Memory::littleEndianToHost(buffer + 176, &m_Header1.birthday[0]);
    System::Memory::littleEndianToHost(buffer + 180, &m_Header1.birthday[1]);

    System::Memory::littleEndianToHost(buffer + 184, &m_Header1.headerLength);

    // +34  patient classification 6 bytes

    System::Memory::littleEndianToHost(buffer + 192, &m_Header1.equipmentProviderID);

    for (int i = 0; i < 6; ++i) { m_Header1.reserved1[i] = buffer[200 + i]; }

    for (int i = 0; i < 3; ++i)
    {
        System::Memory::littleEndianToHost(buffer + 206 + i * sizeof(uint16_t), &m_Header1.headSize[i]);
        System::Memory::littleEndianToHost(buffer + 212 + i * sizeof(float), &m_Header1.positionReferenceElectrode[i]);
        System::Memory::littleEndianToHost(buffer + 224 + i * sizeof(float), &m_Header1.groundElectrode[i]);
    }

    System::Memory::littleEndianToHost(buffer + 236, reinterpret_cast<uint64_t*>(&m_Header1.nDataRecords));
    System::Memory::littleEndianToHost(buffer + 244, &m_Header1.duration);
    System::Memory::littleEndianToHost(buffer + 252, &m_Header1.nSignals);

    System::Memory::littleEndianToHost(buffer + 254, &m_Header1.reserved2);

    if (oFile.bad()) { return false; }
    return true;
}

bool GDF::CFixedGDF251Header::save(std::ofstream& /*file*/)
{
    return false;
    //TODO complete
    /*
    uint8_t tempBuffer[sizeof(uint64_t)];
    file.seekp(0,std::ios::beg);
    file << m_VersionId << m_PatientId;
    */
}

std::string GDF::CFixedGDF251Header::getSubjectName()
{
    // extracts the PID and Patient name from the m_sPatientId
    char* token = strtok(m_Header1.patientID, " ");
    if (token) { token = strtok(nullptr, " "); }    //The PId is not a numerical value in GDF, it is useless for us
    if (token) { return token; }
    return NO_VALUE_S;
}

std::string GDF::CFixedGDF251Header::getExperimentDate()
{
    //computes the experiment date
    //uint64_t tempDate= *(reinterpret_cast<uint64_t*>(m_startDateAndTimeOfRecording));

    //time_t startDateAndTimeOfRecordingInSeconds = ((tempDate/2^32) - 719529) * (3600*24);
    //tm * startDateAndTimeOfRecording = gmtime(&startDateAndTimeOfRecordingInSeconds);

    //TODO check how date is coded in openvibe Date not good?
    //(startDateAndTimeOfRecording->mon+1)<<8 + (startDateAndTimeOfRecording->day)

    return NO_VALUE_S;
}
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
