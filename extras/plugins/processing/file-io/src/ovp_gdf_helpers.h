#pragma once

#include <openvibe/ov_all.h>

#include <cmath>
#include <fstream>
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {
#define NO_VALUE_I 0xffffffff
#define NO_VALUE_S "_unspecified_"

/**
 * Useful classes for GDF file format handling
*/
class GDF
{
public:

    class CFixedGDFHeader
    {
    public:
        virtual ~CFixedGDFHeader() {}

        virtual bool read(std::ifstream& file) = 0;

        virtual bool save(std::ofstream& file) = 0;
        virtual bool update(std::ofstream& file) = 0;

        virtual uint64_t getSubjectID() { return NO_VALUE_I; }
        virtual std::string getSubjectName() { return NO_VALUE_S; }
        virtual uint64_t getSubjectSex() { return NO_VALUE_I; }
        virtual uint64_t getSubjectAge() { return NO_VALUE_I; }
        virtual uint64_t getExperimentID() { return NO_VALUE_I; }
        virtual std::string getExperimentDate() { return NO_VALUE_S; }
        virtual uint64_t getLaboratoryID() { return NO_VALUE_I; }
        virtual uint64_t getTechnicianID() { return NO_VALUE_I; }
        virtual std::string getLaboratoryName() { return NO_VALUE_S; }
        virtual std::string getTechnicianName() { return NO_VALUE_S; }

        virtual double getDataRecordDuration() = 0;
        virtual uint64_t getNDataRecords() = 0;
        virtual size_t getChannelCount() = 0;
    };

    /**
      * An helper class to manipulate GDF1 fixed-size headers
     */
    class CFixedGDF1Header final : public CFixedGDFHeader
    {
    public:
        CFixedGDF1Header();
        ~CFixedGDF1Header() override {}

        /**
         * Reads a GDF1 fixed Header from a file
         * \param file The input file.
         * \return true if the operation was successful
         */
        bool read(std::ifstream& file) override;

        /**
         * Saves a GDF1 fixed Header in a file
         * \param file The output file.
         * \return true if the operation was successful
         */
        bool save(std::ofstream& file) override;

        /**
         * Updates the number of data records field in
         * a GDF1 fixed Header in a file
         * \param file The output file.
         * \return true if the operation was successful
         */
        bool update(std::ofstream& file) override;

        std::string getSubjectName() override;
        uint64_t getLaboratoryID() override;
        uint64_t getTechnicianID() override;

        double getDataRecordDuration() override { return double(m_DurationDataRecordNum) / double(m_DurationDataRecordDen); }
        uint64_t getNDataRecords() override { return m_NDataRecords; }
        size_t getChannelCount() override { return m_NSignals; }

        char m_VersionID[8];
        char m_PatientID[80];
        char m_RecordingID[80];
        char m_StartDateAndTimeOfRecording[16];
        int64_t m_NBytesInHeaderRecord = 0;
        uint64_t m_EquipmentProviderID = 0;
        uint64_t m_LaboratoryID        = 0;
        uint64_t m_TechnicianID        = 0;
        char m_ReservedSerialNumber[20];
        int64_t m_NDataRecords           = 0;
        uint32_t m_DurationDataRecordNum = 0;
        uint32_t m_DurationDataRecordDen = 0;
        uint32_t m_NSignals              = 0;
    };

    /**
     * An helper class to manipulate GDF2 fixed-size headers
     */
    class CFixedGDF2Header final : public CFixedGDFHeader
    {
    public:

        CFixedGDF2Header();
        ~CFixedGDF2Header() override {}

        /**
        * Reads a GDF2 fixed Header from a file
        * \param oFile The input file.
        * \return true if the operation was successful
         */
        bool read(std::ifstream& oFile) override;

        /**
         * Saves a GDF2 fixed Header in a file
         * \param file The output file.
         * \return true if the operation was successful
         */
        bool save(std::ofstream& file) override;

        /**
         * Updates the number of data records field in
         * a GDF2 fixed Header in a file
         * \param file The output file.
         * \return true if the operation was successful
         */
        bool update(std::ofstream& file) override { return true; }

        std::string getExperimentDate() override;
        std::string getSubjectName() override;
        uint64_t getSubjectSex() override { return m_SubjectInfo & 0x03; }

        uint64_t getSubjectAge() override { return uint64_t(std::floor(double((m_StartDateAndTimeOfRecording[1] - m_Birthday[1]) / 365.242189813))); }

        double getDataRecordDuration() override { return double(m_DurationRecordNum) / double(m_DurationDataRecordDen); }
        uint64_t getNDataRecords() override { return m_NDataRecords; }
        size_t getChannelCount() override { return m_NSignals; }

        char m_VersionID[8];
        char m_PatientID[66];

        uint8_t m_Reserved[10];
        uint8_t m_HealthInformation = 0; //smoking...
        uint8_t m_Weight            = 0;
        uint8_t m_Height            = 0;
        uint8_t m_SubjectInfo       = 0;//gender...

        char m_RecordingID[64];
        uint32_t m_RecordingLocation[4];
        uint32_t m_StartDateAndTimeOfRecording[2];
        uint32_t m_Birthday[2];
        uint16_t m_NBlocksInHeader = 0;
        uint8_t m_Reserved2[6];
        uint64_t m_EquipmentProviderID = 0;
        uint8_t m_IPAdress[6];
        uint16_t m_HeadSize[3];
        float m_PositionReferenceElectrode[3];
        float m_GroundElectrode[3];
        int64_t m_NDataRecords           = 0;
        uint32_t m_DurationRecordNum     = 0;
        uint32_t m_DurationDataRecordDen = 0;
        uint16_t m_NSignals              = 0;
        uint16_t m_Reserved3             = 0;
    };


    /**
     * An helper class to manipulate GDF2.51 fixed-size headers
     */
    class CFixedGDF251Header final : public CFixedGDFHeader
    {
    public:

        CFixedGDF251Header() { memset(&m_Header1, 0, sizeof(m_Header1)); }
        ~CFixedGDF251Header() override {}

        /**
        * Reads a GDF2 fixed Header from a file
        * \param oFile The input file.
        * \return true if the operation was successful
         */
        bool read(std::ifstream& oFile) override;

        /**
         * Saves a GDF2 fixed Header in a file
         * \param file The output file.
         * \return true if the operation was successful
         */
        bool save(std::ofstream& file) override;

        /**
         * Updates the number of data records field in
         * a GDF2 fixed Header in a file
         * \param file The output file.
         * \return true if the operation was successful
         */
        bool update(std::ofstream& file) override { return true; }

        std::string getExperimentDate() override;
        std::string getSubjectName() override;
        uint64_t getSubjectSex() override { return m_Header1.subjectInfo & 0x03; }

        uint64_t getSubjectAge() override
        {
            return uint64_t(floor(double((m_Header1.startDateAndTimeOfRecording[1] - m_Header1.birthday[1]) / 365.242189813)));
        }

        double getDataRecordDuration() override { return m_Header1.duration; }
        uint64_t getNDataRecords() override { return m_Header1.nDataRecords; }
        size_t getChannelCount() override { return m_Header1.nSignals; }

        struct SGDFFixedHeader1
        {
            char versionID[8];
            char patientID[66];

            uint8_t reserved[10];
            uint8_t healthInfo; //smoking...
            uint8_t weight;
            uint8_t height;
            uint8_t subjectInfo;//gender...

            char recordingID[64];
            uint32_t recordingLocation[4];
            uint32_t startDateAndTimeOfRecording[2];
            uint32_t birthday[2];
            uint16_t headerLength;
            char patientClassification[6];
            uint64_t equipmentProviderID;
            uint8_t reserved1[6];
            uint16_t headSize[3];
            float positionReferenceElectrode[3];
            float groundElectrode[3];
            int64_t nDataRecords;
            double duration;            // Not double in the 2.51 spec document at the time of writing this, but seems to be so in practice?
            uint16_t nSignals;
            uint16_t reserved2;
        };

        SGDFFixedHeader1 m_Header1;
    };

    /**
    * Base class for GDF file's variable headers
    */
    class CVariableGDFHeader
    {
    public:
        virtual ~CVariableGDFHeader() { }
        virtual bool save(std::ofstream& file) = 0;
    };

    /**
     * GDF1 variable header class
     */
    class CVariableGDF1Header final : public CVariableGDFHeader
    {
        //! Stores information for one channel
        class CVariableGDF1HeaderPerChannel
        {
        public:

            CVariableGDF1HeaderPerChannel();

            char m_Label[16];
            char m_TranducerType[80];
            char m_PhysicalDimension[8];
            double m_PhysicalMin = 0;
            double m_PhysicalMax = 0;
            int64_t m_DigitalMin = 0;
            int64_t m_DigitalMax = 0;
            char m_PreFiltering[80];
            uint32_t m_NSamplesInEachRecord = 0;
            uint32_t m_ChannelType          = 0;
            char m_Reserved[32];
        };

    public:
        ~CVariableGDF1Header() override {}

        /**
        * Saves a GDF1 variable Header in a file
        * \param file The output file.
        * \return true if the operation was successful
         */
        bool save(std::ofstream& file) override;

        /**
         * Updates the Physical/digital Min/max fields in
         * a GDF1 variable Header in a file
         * \param file The output file.
         * \return true if the operation was successful
         */
        bool update(std::ofstream& file);

        /**
         * Sets the number of channels in the file.
         * \param nChannel Number of channels.
         */
        void setChannelCount(const uint32_t nChannel) { m_VariableHeaders.resize(nChannel); }

        CVariableGDF1HeaderPerChannel& operator[](const uint32_t channel) { return m_VariableHeaders[channel]; }

        std::vector<CVariableGDF1HeaderPerChannel> m_VariableHeaders;
    };


    class CGDFEvent
    {
    public:
        uint32_t m_Position = 0;
        uint16_t m_Type     = 0;
    };

    enum EChannelType
    {
        ChannelType_int8_t = 1,
        ChannelType_uint8_t = 2,
        ChannelType_int16_t = 3,
        ChannelType_uint16_t = 4,
        ChannelType_int32_t = 5,
        ChannelType_uint32_t = 6,
        ChannelType_int64_t = 7,
        ChannelType_uint64_t = 8,
        ChannelType_float = 16,
        ChannelType_double = 17,
        ChannelType_float128 = 18,
        ChannelType_int24 = 279,
        ChannelType_uint24 = 535
    };

    /**
     * Gets the data size in bytes of the GDF data type
     * \param channelType The GDF type
     * \return The size in bytes of this GDF type's data
     */
    static uint16_t GDFDataSize(uint32_t channelType);
};
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
