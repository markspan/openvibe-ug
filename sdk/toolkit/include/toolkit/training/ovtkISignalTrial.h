#pragma once

#include "../ovtk_base.h"

namespace OpenViBE {
namespace Toolkit {
class OVTK_API ISignalTrial : public IObject
{
public:

    virtual bool setSamplingRate(size_t sampling) = 0;
    virtual bool setChannelCount(size_t count) = 0;
    virtual bool setChannelName(size_t index, const char* name) = 0;
    virtual bool setLabelIdentifier(const CIdentifier& labelIdentifier) = 0;
    virtual bool setSampleCount(size_t count, bool preserve) = 0;

    virtual size_t getSamplingRate() const = 0;
    virtual size_t getChannelCount() const = 0;
    virtual const char* getChannelName(const size_t index) const = 0;
    virtual CIdentifier getLabelIdentifier() const = 0;
    virtual size_t getSampleCount() const = 0;
    virtual uint64_t getDuration() const = 0;
    virtual double* getChannelSampleBuffer(size_t index) const = 0;

    _IsDerivedFromClass_(IObject, OVTK_ClassId_)
};

extern OVTK_API ISignalTrial* createSignalTrial();
extern OVTK_API void releaseSignalTrial(ISignalTrial* trial);

// operations
extern OVTK_API ISignalTrial& copyHeader(ISignalTrial& trial, const ISignalTrial* srcTrial = nullptr);
extern OVTK_API ISignalTrial& copy(ISignalTrial& trial, const ISignalTrial* srcTrial = nullptr);
extern OVTK_API ISignalTrial& selectSamples(ISignalTrial& trial, size_t sampleStart, size_t sampleEnd, const ISignalTrial* srcTrial = nullptr);
extern OVTK_API ISignalTrial& selectTime(ISignalTrial& trial, uint64_t timeStart, uint64_t timeEnd, const ISignalTrial* srcTrial = nullptr);
extern OVTK_API ISignalTrial& removeSamples(ISignalTrial& trial, size_t sampleStart, size_t sampleEnd, const ISignalTrial* srcTrial = nullptr);
extern OVTK_API ISignalTrial& removeTime(ISignalTrial& trial, uint64_t timeStart, uint64_t timeEnd, const ISignalTrial* srcTrial = nullptr);
extern OVTK_API ISignalTrial& insertBufferSamples(ISignalTrial& trial, size_t sampleStart, size_t nSample, const double* buffer, const ISignalTrial* srcTrial = nullptr);
extern OVTK_API ISignalTrial& insertBufferTime(ISignalTrial& trial, uint64_t timeStart, size_t nSample, const double* buffer, const ISignalTrial* srcTrial = nullptr);
}  // namespace Toolkit
}  // namespace OpenViBE
