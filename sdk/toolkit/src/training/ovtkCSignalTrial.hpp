#pragma once

#include "ovtkISignalTrial.h"

#include <map>
#include <string>

namespace OpenViBE {
namespace Toolkit {
class CSignalTrial final : public ISignalTrial
{
public:

    CSignalTrial() {}
    ~CSignalTrial() override { for (auto& s : m_channelSamples) { delete [] s.second; } }
    bool setSamplingRate(size_t sampling) override;
    bool setChannelCount(size_t count) override;
    bool setChannelName(size_t index, const char* name) override;
    bool setLabelIdentifier(const CIdentifier& labelID) override;
    bool setSampleCount(size_t count, bool preserve) override;
    size_t getSamplingRate() const override { return m_sampling; }
    size_t getChannelCount() const override { return m_nChannel; }
    const char* getChannelName(const size_t index) const override;
    CIdentifier getLabelIdentifier() const override { return m_labelID; }
    size_t getSampleCount() const override { return m_nSample; }
    uint64_t getDuration() const override { return (m_sampling ? CTime(m_sampling, m_nSample).time() : 0); }
    double* getChannelSampleBuffer(size_t index) const override;

    _IsDerivedFromClass_Final_(ISignalTrial, OVTK_ClassId_)

protected:

    std::map<size_t, std::string> m_channelNames;
    std::map<size_t, double*> m_channelSamples;
    size_t m_nChannel        = 0;
    size_t m_nSample         = 0;
    size_t m_nSampleReserved = 0;
    size_t m_sampling        = 0;
    CIdentifier m_labelID    = CIdentifier::undefined();
};
}  // namespace Toolkit
}  // namespace OpenViBE
