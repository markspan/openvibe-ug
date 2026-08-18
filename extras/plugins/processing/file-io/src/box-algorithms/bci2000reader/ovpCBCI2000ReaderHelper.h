#pragma once
#include <fstream>
#include <ostream>
#include <vector>
#include <map>

#include "ovpCBitfield.h"

namespace BCI2000 {
/**
* \class CBCI2000ReaderHelper
* \author Olivier Rochel (INRIA)
* \brief BCI2000 file format parser and utilities. Uses the m_oBitfield utility class.
**/
class CBCI2000ReaderHelper
{
protected:
    std::ifstream m_file;

    float m_bci2000Version  = 0;    // file version.
    int m_headerLength      = 0;    // header size (inc. meta)
    int m_nSrcChannel       = 0;    // number of channels
    int m_stateVectorLength = 0;    // size of state field
    OpenViBE::CString m_dataFormat; // data format (float, int16_t...)

    std::vector<OpenViBE::CString> m_channelNames;

    int m_nSamples    = 0;
    int m_sampleSize  = 0;
    int m_samplesLeft = 0;

    bool m_good = false;            // m_bGood is true if file open, header looks m_bGood (may
    // still be truncated or broken in a silly way)

    std::map<OpenViBE::CString, OpenViBE::CString> m_parameters;
    // state vector
    CBitfield m_bitfield;
    // helpers
    bool parseMeta(OpenViBE::CString& meta);
    bool parseHeader(std::istream& is);

private:
    template <class TFrom, class TTo>
    int readSamplesInternal(TTo* samples, uint32_t* states, int n);

public:
    /**
    * Constructor from a BCI2000 file.
    * \param filename BCI2000 file name.
    **/
    explicit CBCI2000ReaderHelper(const char* filename);
    ~CBCI2000ReaderHelper() { if (m_file) { m_file.close(); } }

    void printInfo(std::ostream& os) const;
    float getRate() const;
    OpenViBE::CString getChannelName(size_t index) const;

    std::vector<float> readSample();
    int readSamples(double* samples, uint32_t* states, int n);

    // getters
    int getNbSamples() const { return m_nSamples; }
    int getSampleSize() const { return m_sampleSize; }
    int getChannels() const { return m_nSrcChannel; }
    int getSamplesLeft() const { return m_samplesLeft; }
    bool isGood() const { return m_good; }
    size_t getStateVectorSize() const { return m_bitfield.size(); }
    const OpenViBE::CString& getStateName(const int i) const { return m_bitfield.getFieldName(i); }
};
}  // namespace BCI2000
