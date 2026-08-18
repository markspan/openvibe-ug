#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <boost/regex.hpp>
#include <boost/predef/other/endian.h>

#include "ovpCBCI2000ReaderHelper.h"

namespace BCI2000 {

void CBCI2000ReaderHelper::printInfo(std::ostream& os) const
{
    if (m_good)
    {
        os << "version:              " << m_bci2000Version << std::endl;
        os << "header length:        " << m_headerLength << std::endl;
        os << "source channels:      " << m_nSrcChannel << std::endl;
        os << "state vector length:  " << m_stateVectorLength << std::endl;
        os << "data format:          " << m_dataFormat << std::endl;
        os << "samples:              " << m_nSamples << std::endl;
        os << "samples left:         " << m_samplesLeft << std::endl;
        os << "sample size:          " << m_sampleSize << std::endl;
        os << "rate:                 " << getRate() << std::endl;
    }
    else { os << "bad file (unreadable or bad header)" << std::endl; }
}

bool CBCI2000ReaderHelper::parseMeta(OpenViBE::CString& meta)
{
    static const boost::regex RE_META("BCI2000V= ([0-9.]+) HeaderLen= ([0-9]+) SourceCh= ([0-9]+) StateVectorLen(gth)?= ([0-9]+) DataFormat= ([a-z0-9]+)\r?$",
                                      boost::regex::perl | boost::regex::icase);
    static const boost::regex RE_OLD_META("HeaderLen= +([0-9]+) SourceCh= +([0-9]+) StatevectorLen= +([0-9]+)\r$", boost::regex::perl | boost::regex::icase);

    boost::smatch match;

    if (regex_match(std::string(meta), match, RE_META))
    {
        m_bci2000Version = float(atof(match.str(1).c_str()));
        m_headerLength   = int(atoi(match.str(2).c_str()));
        m_nSrcChannel    = int(atoi(match.str(3).c_str()));
        // 4 is dropped (could be used for len/length syntax check)
        m_stateVectorLength = int(atoi(match.str(5).c_str()));
        m_dataFormat        = match.str(6).c_str();
    }
    else
    {
        if (regex_match(std::string(meta), match, RE_OLD_META))
        {
            m_bci2000Version    = 1.0;
            m_headerLength      = int(atoi(match.str(1).c_str()));
            m_nSrcChannel       = int(atoi(match.str(2).c_str()));
            m_stateVectorLength = int(atoi(match.str(3).c_str()));
            m_dataFormat        = "int16_t";
        }
        else
        {
            std::cerr << " *** bci2000helper error: cannot parse meta information header" << std::endl;
            return false;
        }
    }
    return true;
}


bool CBCI2000ReaderHelper::parseHeader(std::istream& is)
{
    static const boost::regex REGEXP_SECTION("\\[ *(.*[^ ]) *\\].*");
    static const boost::regex REGEXP_PARAMETER("([^ ]+ [^ ]+ [^ ]+)= (.*)$"); // (section) (type) (name)= (value)
    static const boost::regex REGEXP_FIELD("([^ ]+) ([^ ]+) ([^ ]+) ([^ ]+) ([^ ]+)$"); // Name Length Value ByteLocation BitLocation
    std::string section = "NONAME";
    std::string line;
    boost::smatch match;
    std::getline(is, line);
    while (line.length() > 2)
    {
        // is it a section name ?
        if (regex_match(line, match, REGEXP_SECTION)) { section = match[1]; }
        else // not section, parse if interesting
        {
            if (section == "Parameter Definition")
            {
                if (regex_match(line, match, REGEXP_PARAMETER))
                {
                    m_parameters[match[1].str().c_str()] = match[2].str().c_str();
                    //std::cout << "parameter added : \n\t\t" << match[1].str() << " \n\t\t" << match[2].str() << "\n";
                }
                else
                {
                    // should never happen: malformed file
                    std::cerr << " *** bci2000helper error: cannot parse Parameters" << std::endl;
                    return false;
                }
            }
            if (section == "State Vector Definition")
            {
                if (regex_match(line, match, REGEXP_FIELD))
                {
                    const int length  = atoi(match.str(2).c_str());
                    const int value   = atoi(match.str(3).c_str());
                    const int bytePos = atoi(match.str(4).c_str());
                    const int bitPos  = atoi(match.str(5).c_str());
                    m_bitfield.addField(bytePos, bitPos, length, match.str(1).c_str(), value);
                }
                else
                {
                    // should never happen: malformed file
                    std::cerr << " *** bci2000helper error: cannot parse Parameters" << std::endl;
                    return false;
                }
            }
        }
        std::getline(is, line);
    }
    return true;
}

float CBCI2000ReaderHelper::getRate() const
{
    // Warning: the SamplingRate field is not clearly defined
    // in the BCI2000 doc; it could be an int, or a float;
    // it should be in Section Source, but Source:Garbage:Stuff is possible
    // in some case, there is a pending "Hz" after the numbers
    // Conclusion: we should not fix this until it's clear...

    OpenViBE::CString key = "Source int SamplingRate";
    if (m_parameters.count(key) == 1)
    {
        std::istringstream is(m_parameters.find(key)->second.toASCIIString());
        float rate;
        is >> rate;
        return rate;
    }
    key = "Source:Signal%20Properties:DataIOFilter int SamplingRate";
    if (m_parameters.count(key) == 1)
    {
        std::istringstream is(m_parameters.find(key)->second.toASCIIString());
        float rate;
        is >> rate;
        return rate;
    }
    return -1.0; // not found :-(
}

OpenViBE::CString CBCI2000ReaderHelper::getChannelName(const size_t index) const
{
    // To be checked on the different version of format.

    const OpenViBE::CString key = "Source:Signal%20Properties:DataIOFilter list ChannelNames";
    if (m_parameters.count(key) == 1)
    {
        std::istringstream is(static_cast<const char*>(m_parameters.find(key)->second));
        std::string token;
        for (size_t i = 0; i <= index + 1; ++i) //+1 because the channel count is in the parameter on first position (parameter:list)
        {
            token.clear();
            is >> token;
        }
        return token.c_str();
    }

    return ("Channel " + std::to_string(index + 1)).c_str();
}


CBCI2000ReaderHelper::CBCI2000ReaderHelper(const char* filename)
{
    m_file.open(filename, std::ios::binary);
    if (!m_file.good())
    {
        m_good = false;
        return;
    }

    std::stringbuf buffer;
    m_file.get(buffer);
    OpenViBE::CString meta = buffer.str().c_str();

    m_file.seekg(0, std::ios::end);
    const int fileSize = int(m_file.tellg());
    m_file.seekg(0, std::ios::beg);

    m_good = parseMeta(meta);
    if (!m_good) { return; }

    std::map<OpenViBE::CString, int> sizesOfMap;
    sizesOfMap["float"]   = 4;
    sizesOfMap["int"]     = 4;
    sizesOfMap["int16_t"] = 2;
    m_nSamples            = (fileSize - m_headerLength) / (sizesOfMap[m_dataFormat] * m_nSrcChannel + m_stateVectorLength);
    m_sampleSize          = sizesOfMap[m_dataFormat] * m_nSrcChannel + m_stateVectorLength;
    m_samplesLeft         = m_nSamples;
    m_good                = parseHeader(m_file);
}


std::vector<float> CBCI2000ReaderHelper::readSample()
{
    std::vector<float> samples;
    if (m_samplesLeft < 1)
    {
        return samples; // nothing to read, empty vector returned
    }
    char* data = new char[m_sampleSize];
    m_file.read(data, m_sampleSize);
    float* dataAsFloat = reinterpret_cast<float*>(data);
    for (int i = 0; i < m_nSrcChannel; ++i) { samples.push_back(dataAsFloat[i]); }
    delete[] data;
    m_samplesLeft--;
    return samples;
}

template <class TFrom, class TTo>
int CBCI2000ReaderHelper::readSamplesInternal(TTo* samples, uint32_t* states, int n)
{
    if (n > m_samplesLeft) { n = m_samplesLeft; }
    if (n < 1) { return 0; }
    char* data = new char[m_sampleSize * n];
    m_file.read(data, m_sampleSize * n);
    for (int i = 0; i < n; ++i)
    {
        if (samples != nullptr)
        {
            for (int j = 0; j < m_nSrcChannel; ++j)
            {
                TFrom sample                   = *reinterpret_cast<TFrom*>(data + i * m_sampleSize + j * sizeof(TFrom));
                samples[i * m_nSrcChannel + j] = sample;
                // check endianess ?
            }
        }
        if (states != nullptr)
        {
            unsigned char* state = reinterpret_cast<unsigned char*>(data) + i * m_sampleSize + m_nSrcChannel * sizeof(TFrom);
            m_bitfield.getFields(state, states + i * m_bitfield.size());
        }
        //std::copy(l_pData+i*m_i32SampleSize,l_pData+i*m_i32SampleSize+m_nSrcChannel*sizeof(T),samples+i*m_nSrcChannel);
    }
    delete[] data;
    m_samplesLeft -= n;
    return n;
}

int CBCI2000ReaderHelper::readSamples(double* samples, uint32_t* states, const int n)
{
    if (m_dataFormat == OpenViBE::CString("float")) { return readSamplesInternal<float, double>(samples, states, n); }
#if defined(BOOST_ENDIAN_LITTLE_BYTE)
    if (m_dataFormat == OpenViBE::CString("int")) { return readSamplesInternal<int, double>(samples, states, n); }
    if (m_dataFormat == OpenViBE::CString("int16_t")) { return readSamplesInternal<int16_t, double>(samples, states, n); }
#else
    std::cerr << "*** bci2000helper error: read_samples from int16_t or int is not implemented yet on bigendian machines" << std::endl;
    exit(EXIT_FAILURE);
#endif
    return -1; // should never happen... TODO: error checking
}

}  // namespace BCI2000
