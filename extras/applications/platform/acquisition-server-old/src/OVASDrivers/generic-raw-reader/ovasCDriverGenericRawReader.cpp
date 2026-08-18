/*
 * The raw reader expects the data to be formatted as follows
 *
 * [START][BLOCK0][BLOCK1][BLOCK2]...
 *  skip   parse   parse   parse  ...
 *
 * where each block is          [===========BLOCKX=================]
 *                  is read as  [===========dataFrameSize==========]
 *                  breaks to   [header====][sample====][footer====]
 *                  equals      [headerSize][sampleSize][footerSize]
 *                  means          skip        keep         skip
 *
 * For correct parsing, user must provide the exact sizes of the skipped parts "start", "header" and "footer" in bytes. 
 *
 */

#include "ovasCDriverGenericRawReader.h"

#include <toolkit/ovtk_all.h>

#include <system/ovCMemory.h>
#include <system/ovCTime.h>

namespace OpenViBE {
namespace AcquisitionServer {

// #define OPENVIBE_DEBUG_RAW_READER

template <class T>
static float decode_little_endian(const uint8_t* buffer)
{
    T t;
    System::Memory::littleEndianToHost(buffer, &t);
    return float(t);
}

template <class T>
static float decode_big_endian(const uint8_t* buffer)
{
    T t;
    System::Memory::bigEndianToHost(buffer, &t);
    return float(t);
}

static CDriverGenericRawReader::EParameter resolveEndian(const std::string &param) {
    if (param == "LittleEndian") { return CDriverGenericRawReader::Endian_Little; }
    if (param == "BigEndian") { return CDriverGenericRawReader::Endian_Big; }
    return CDriverGenericRawReader::Endian_Little;
}

static CDriverGenericRawReader::EParameter resolveFormat(const std::string &param) {
    if (param == "UInt8") { return CDriverGenericRawReader::Format_UnsignedInteger8; }
    if (param == "UInt16") { return CDriverGenericRawReader::Format_UnsignedInteger16; }
    if (param == "UInt32") { return CDriverGenericRawReader::Format_UnsignedInteger32; }
    if (param == "Int8") { return CDriverGenericRawReader::Format_SignedInteger8; }
    if (param == "Int16") { return CDriverGenericRawReader::Format_SignedInteger16; }
    if (param == "Int32") { return CDriverGenericRawReader::Format_SignedInteger32; }
    if (param == "Float32") { return CDriverGenericRawReader::Format_Float32; }
    if (param == "Float64") { return CDriverGenericRawReader::Format_Float64; }
    return CDriverGenericRawReader::Format_SignedInteger32;
}


CDriverGenericRawReader::CDriverGenericRawReader(IDriverContext& ctx) : IDriver(ctx),
    m_limitSpeed("LimitSpeed", "limit the speed", false),
    m_sampleFormat("SampleFormat", "sample format", {"UInt8", "UInt16", "UInt32", "Int8", "Int16", "Int32", "Float32", "Float64" },  "Int32"),
    m_sampleEndian("SampleEndian", "sample endian", {"LittleEndian", "BigEndian"}, "LittleEndian"),
    m_startSkip("StartSkip", "number of bytes to skip at the start", 0, 0, 1024),
    m_headerSkip("HeaderSkip", "number of bytes to skip at the start of each block", 0, 0, 1024),
    m_footerSkip("FooterSkip", "number of bytes to skip at the end of each block", 20, 0, 1024)
{
    m_header.setSamplingFrequency(512);
    m_header.setChannelCount(16);

    m_parameters.push_back(&m_limitSpeed);
    m_parameters.push_back(&m_sampleFormat);
    m_parameters.push_back(&m_sampleEndian);
    m_parameters.push_back(&m_startSkip);
    m_parameters.push_back(&m_headerSkip);
    m_parameters.push_back(&m_footerSkip);
}

//___________________________________________________________________//
//                                                                   //

bool CDriverGenericRawReader::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    if (m_driverCtx.isConnected()) { return false; }

    switch (resolveFormat(m_sampleFormat.value())) {
        case Format_UnsignedInteger8:
        case Format_SignedInteger8: m_sampleSize = 1;
            break;
        case Format_UnsignedInteger16:
        case Format_SignedInteger16: m_sampleSize = 2;
            break;
        case Format_UnsignedInteger32:
        case Format_SignedInteger32:
        case Format_Float32: m_sampleSize = 4;
            break;
        case Format_Float64: m_sampleSize = 8;
            break;
        default: m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unsupported data format " << m_sampleFormat.value() << "\n";
            return false;
    }

    m_dataFrameSize = m_sampleSize * m_header.getChannelCount();
    m_dataFrameSize += m_headerSkip;
    m_dataFrameSize += m_footerSkip;

    m_sample    = new float[m_header.getChannelCount()];
    m_dataFrame = new uint8_t[m_dataFrameSize];
    if (!m_sample || !m_dataFrame) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not allocate memory !\n";
        return false;
    }

    // open() should skip m_startSkip worth of bytes already
    if (!this->open()) { return false; }

    m_callback            = &callback;
    m_nSamplePerSentBlock = nSamplePerSentBlock;

    return true;
}

bool CDriverGenericRawReader::start()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }
    m_nTotalSample = 0;
    m_startTime    = System::Time::zgetTime();
    return true;
}

bool CDriverGenericRawReader::loop()
{
    if (!m_driverCtx.isConnected()) { return false; }
    // if(!m_driverCtx.isStarted()) { return true; }

    const uint64_t time = CTime(m_header.getSamplingFrequency(), m_nTotalSample).time();
    if (m_limitSpeed && (time > System::Time::zgetTime() - m_startTime)) { return true; }

#ifdef OPENVIBE_DEBUG_RAW_READER
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Decoded : ";
#endif

    for (uint32_t j = 0; j < m_nSamplePerSentBlock; ++j) {
        if (!this->read()) { return false; }

        for (size_t i = 0; i < m_header.getChannelCount(); ++i) {
            uint8_t* dataFrame = m_dataFrame + m_headerSkip + i * m_sampleSize;
            switch (resolveEndian(m_sampleEndian.value())) {
                case Endian_Little: switch (resolveFormat(m_sampleFormat.value())) {
                        case Format_UnsignedInteger8: m_sample[i] = *dataFrame;
                            break;
                        case Format_UnsignedInteger16: m_sample[i] = decode_little_endian<uint16_t>(dataFrame);
                            break;
                        case Format_UnsignedInteger32: m_sample[i] = decode_little_endian<uint32_t>(dataFrame);
                            break;
                        case Format_SignedInteger8: m_sample[i] = *dataFrame;
                            break;
                        case Format_SignedInteger16: m_sample[i] = decode_little_endian<int16_t>(dataFrame);
                            break;
                        case Format_SignedInteger32: m_sample[i] = decode_little_endian<int>(dataFrame);
                            break;
                        case Format_Float32: m_sample[i] = decode_little_endian<float>(dataFrame);
                            break;
                        case Format_Float64: m_sample[i] = decode_little_endian<double>(dataFrame);
                            break;
                        default: break;
                    }
                    break;

                case Endian_Big: switch (resolveFormat(m_sampleFormat.value())) {
                        case Format_UnsignedInteger8: m_sample[i] = *dataFrame;
                            break;
                        case Format_UnsignedInteger16: m_sample[i] = decode_big_endian<uint16_t>(dataFrame);
                            break;
                        case Format_UnsignedInteger32: m_sample[i] = decode_big_endian<uint32_t>(dataFrame);
                            break;
                        case Format_SignedInteger8: m_sample[i] = *dataFrame;
                            break;
                        case Format_SignedInteger16: m_sample[i] = decode_big_endian<int16_t>(dataFrame);
                            break;
                        case Format_SignedInteger32: m_sample[i] = decode_big_endian<int>(dataFrame);
                            break;
                        case Format_Float32: m_sample[i] = decode_big_endian<float>(dataFrame);
                            break;
                        case Format_Float64: m_sample[i] = decode_big_endian<double>(dataFrame);
                            break;
                        default: break;
                    }
                    break;
                default: break;
            }
#ifdef OPENVIBE_DEBUG_RAW_READER
            m_driverCtx.getLogManager() << m_sample[i] << " ";
#endif
        }
        if (m_driverCtx.isStarted()) { m_callback->setSamples(m_sample, 1); }
    }
#ifdef OPENVIBE_DEBUG_RAW_READER
    m_driverCtx.getLogManager() << "\n";
#endif

    if (m_driverCtx.isStarted()) { m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount()); }
    m_nTotalSample += m_nSamplePerSentBlock;
    return true;
}

bool CDriverGenericRawReader::stop()
{
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }
    return true;
}

bool CDriverGenericRawReader::uninitialize()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted() || !this->close()) { return false; }

    delete [] m_sample;
    delete [] m_dataFrame;
    m_sample    = nullptr;
    m_dataFrame = nullptr;
    m_callback  = nullptr;

    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
