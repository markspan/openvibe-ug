#pragma once

#include <vector>
#include <algorithm>
#include <cassert>
#include <cstdint>

#include <r8brain/CDSPResampler.h>

namespace Common {
namespace Resampler {

enum class EResamplerStoreModes { ChannelWise, SampleWise };

template <class TFloat, EResamplerStoreModes TStoreMode>
class TResampler
{
public:

    class ICallback
    {
    public:

        virtual ~ICallback() { }
        virtual void processResampler(const TFloat* pSample, const size_t nChannel) const = 0;
    };

private:

    TResampler(const TResampler<TFloat, TStoreMode>&) = default;

public:

    /// <summary> Constructor, with default values for real-time processing. </summary>
    TResampler()
    {
        this->clear();
        switch (TStoreMode)
        {
            case EResamplerStoreModes::ChannelWise:
                m_fpResample = &TResampler<TFloat, TStoreMode>::resampleChannelWise;
                m_fpResampleDirect = &TResampler<TFloat, TStoreMode>::resampleChannelWise;
                break;
            case EResamplerStoreModes::SampleWise:
                m_fpResample = &TResampler<TFloat, TStoreMode>::resampleSampleWise;
                m_fpResampleDirect = &TResampler<TFloat, TStoreMode>::resampleSampleWise;
                break;
            default:
                assert(false);
        }
    }

    virtual ~TResampler() { this->clear(); }

    void clear()
    {
        for (size_t j = 0; j < m_resamplers.size(); ++j) { delete m_resamplers[j]; }
        m_resamplers.clear();

        m_nChannel                     = 0;
        m_iSampling                    = 0;
        m_oSampling                    = 0;
        m_nFractionalDelayFilterSample = 6;
        m_iMaxNSampleIn                = 1024;
        m_transitionBandPercent        = 45;
        m_stopBandAttenuation          = 49;
    }

    /// <summary> Specifies the number of samples (taps) each fractional delay filter should have.\n
    /// 
    /// This must be an even value.To achieve a higher resampling precision,
    /// the oversampling should be used in the first place instead of using a higher FractionalDelayFilterSampleCount value.
    /// The lower this value is the lower the signal - to - noise performance of the interpolator will be.
    /// Each FractionalDelayFilterSampleCount decrease by 2 decreases SNR by approximately 12 to 14 decibels. </summary>
    /// <param name="n">The fractional delay (Between 6 and 30, and default = 6).</param>
    /// <returns> <c>true</c> if even and between 6 and 30, <c>false</c> otherwise. </returns>
     /// <remarks>For real-time processing, n = 6. </remarks>
    bool setFractionalDelayFilterSampleCount(const int n = 6)
    {
        if (n < 6 || 30 < n || n % 2 == 1) { return false; }    // false if odd value
        m_nFractionalDelayFilterSample = n;
        return true;
    }

    /// <summary> Maximal planned length of the input buffer (in samples) that will be passed to the resampler.
    /// Input buffers longer than this value should never be supplied to the resampler.
    /// The resampler relies on this value as it allocates intermediate buffers. </summary>
    /// <param name="n">The maximal input sample count.</param>
    /// <returns> <c>true</c> if between 8 and 2048, <c>false</c> otherwise. </returns>
    /// <remarks> Note that the resampler may use the input buffer itself for intermediate sample data storage. </remarks>
    bool setMaxNSampleIn(const int n = 8)
    {
        if (n < 8 || 2048 < n) { return false; }
        m_iMaxNSampleIn = n;
        return true;
    }

    /// <summary> Transition band, in percent of the spectral space of the input signal
    /// (or the output signal if downsampling is performed) between filter's -3 dB point and the Nyquist frequency. </summary>
    /// <param name="n">The Transition band (Between 0.5% and 45%, and default = 10.).</param>
    /// <returns> <c>true</c> if between 0.5 and 45, <c>false</c> otherwise. </returns>
    /// <remarks>For real-time processing, n = 45. </remarks>
    bool setTransitionBand(const double n = 10)
    {
        if (n < r8b::CDSPFIRFilter::getLPMinTransBand() || r8b::CDSPFIRFilter::getLPMaxTransBand() < n) { return false; }
        m_transitionBandPercent = n;
        return true;
    }

    /// <summary> Stop-band attenuation in decibel.\n
    /// 
    /// The general formula is 6.02 * Bits + 40, where "Bits" is the bit resolution(e.g. 16, 24),
    /// "40" is an added resolution for stationary signals, this value can be decreased to 20 to 10
    /// if the signal being resampled is mostly non - stationary(e.g.impulse response). </summary>
    /// <param name="n">The Stop-band attenuation in decibel (Between 49 and 218, and default = 49 is given by the general formula).</param>
    /// <returns> <c>true</c> if between 49 and 218, <c>false</c> otherwise. </returns>
   /// <remarks>For real-time processing, n = 49. </remarks>
    bool setStopBandAttenuation(const double n = 49)
    {
        if (n < r8b::CDSPFIRFilter::getLPMinAtten() || r8b::CDSPFIRFilter::getLPMaxAtten() < n) { return false; }
        m_stopBandAttenuation = n;
        return true;
    }

    /// <summary> This fonction initializes the vector of Resampler, using the number of channels, the input and the output sampling rates. </summary>
    /// <param name="nChannel"> The number of channel. </param>
    /// <param name="iSampling"> The input sampling. </param>
    /// <param name="oSampling"> The output sampling. </param>
    /// <returns> <c>true</c> if success, <c>false</c> otherwise. </returns>
    bool reset(const size_t nChannel, const size_t iSampling, const size_t oSampling)
    {
        if (nChannel == 0 || iSampling == 0 || oSampling == 0) { return false; }
        m_iSampling = iSampling;
        m_oSampling = oSampling;

        m_nChannel = nChannel;
        for (size_t i = 0; i < m_resamplers.size(); ++i) { delete m_resamplers[i]; }
        m_resamplers.clear();
        m_resamplers.resize(m_nChannel);

        const double in                  = double(iSampling), out = double(oSampling);
        const double stopBandAttenuation = m_stopBandAttenuation == 0
                                               ? std::min(6.02 * m_nFractionalDelayFilterSample + 40, r8b::CDSPFIRFilter::getLPMaxAtten())
                                               : m_stopBandAttenuation;
#define NEW_SAMPLER(Len, Fracs)\
    new r8b::CDSPResampler<r8b::CDSPFracInterpolator<Len, Fracs>>(in, out, m_iMaxNSampleIn,\
        m_transitionBandPercent, stopBandAttenuation,r8b::EDSPFilterPhaseResponse(0), false)

        for (size_t j = 0; j < m_nChannel; ++j)
        {
            switch (m_nFractionalDelayFilterSample) // it defines iFractionalDelayPositionCount 
            {
                case 6:
                    m_resamplers[j] = NEW_SAMPLER(6, 11);
                    break;

                case 8:
                    m_resamplers[j] = NEW_SAMPLER(8, 17);
                    break;

                case 10:
                    m_resamplers[j] = NEW_SAMPLER(10, 23);
                    break;

                case 12:
                    m_resamplers[j] = NEW_SAMPLER(12, 41);
                    break;

                case 14:
                    //stopBandAttenuation = 109.56;
                    m_resamplers[j] = NEW_SAMPLER(14, 67);
                    break;

                case 16:
                    m_resamplers[j] = NEW_SAMPLER(16, 97);
                    break;

                case 18:
                    //stopBandAttenuation = 136.45;
                    m_resamplers[j] = NEW_SAMPLER(18, 137);
                    break;

                case 20:
                    m_resamplers[j] = NEW_SAMPLER(20, 211);
                    break;

                case 22:
                    m_resamplers[j] = NEW_SAMPLER(22, 353);
                    break;

                case 24:
                    //stopBandAttenuation = 180.15;
                    m_resamplers[j] = NEW_SAMPLER(24, 673);
                    break;

                case 26:
                    m_resamplers[j] = NEW_SAMPLER(26, 1051);
                    break;

                case 28:
                    m_resamplers[j] = NEW_SAMPLER(28, 1733);
                    break;

                case 30:
                    m_resamplers[j] = NEW_SAMPLER(30, 2833);
                    break;

                default:
                    return false;
            }
        }

#undef NEW_SAMPLER

        return true;
    }

    /// <summary> Gets the latency.\n
    /// 
    /// This value is usually zero if the DSP processor "consumes" the latency automatically. (from CDSPProcessor.h) </summary>
    /// <returns> Latency. </returns>
    virtual int getLatency() const { return m_resamplers[0]->getLatency(); }

    /// <summary> Gets the Fractional latency.
    ///
    /// Fractional latency, in samples, which is present in the output signal.
    /// This value is usually zero if a linear - phase filtering is used.
    /// With minimum - phase filters in use, this value can be non - zero even if the getLatency() function returns zero. (from CDSPProcessor.h)
    /// </summary>
    /// <returns></returns>
    /// <returns> Fractionnal Latency. </returns>
    virtual double getLatencyFrac() const { return m_resamplers[0]->getLatencyFrac(); }

    /// <summary> Gets the cumulative number of samples that should be passed to *this object before the actual output starts.
    /// This value includes latencies induced by all processors which run after *this processor in chain. </summary>
    /// <param name="nextInLen"> The number of input samples required before the output starts on the next resampling step. (from CDSPProcessor.h) </param>
    /// <returns>  the cumulative number of samples. </returns>
    virtual int getInLenBeforeOutStart(const int nextInLen) const { return m_resamplers[0]->getInLenBeforeOutStart(nextInLen); }

    /// <summary> Gets the maximal length of the output buffer required when processing the "maxInLen" number of input samples. </summary>
    /// <param name="maxInLen"> The number of samples planned to process at once, at most. (from CDSPProcessor.h). </param>
    /// <returns> The length. </returns>
    virtual int getMaxOutLen(const int maxInLen) const { return m_resamplers[0]->getMaxOutLen(maxInLen); }

    double getBuiltInLatency() const { return (m_iSampling != 0) ? (1.0 * m_resamplers[0]->getInLenBeforeOutStart(0) / m_iSampling) : 0.0; }

    size_t resample(const ICallback& callback, const TFloat* iSample, const size_t nInSample) { return (this->*m_fpResample)(callback, iSample, nInSample); }

    size_t downsample(const ICallback& callback, const TFloat* iSample, const size_t nInSample) { return (this->*m_fpResample)(callback, iSample, nInSample); }

private:

    /// <summary> This function resamples the signal assuming the input samples are ordered this way :
    /// - sample 1 of channel 1, sample 1 of channel 2, ..., sample 1 of channel nChannel,
    /// - sample 2 of channel 1, sample 2 of channel 2, ..., sample 2 of channel nChannel,
    /// - ...
    /// - sample nInSample of channel 1, sample nInSample of channel 2, ..., sample nInSample of channel nChannel,\n
    /// 
    /// This is convenient for resampling at the acquisition level. </summary>
    /// <param name="callback">The callback.</param>
    /// <param name="iSample">The input sample.</param>
    /// <param name="nInSample">The n input sample.</param>
    /// <returns> the number of sample process in last channel. </returns>
    size_t resampleChannelWise(const ICallback& callback, const TFloat* iSample, const size_t nInSample)
    {
        int nI              = 0;
        bool isFirstChannel = true;

        std::vector<double> iBuffers(nInSample);
        std::vector<TFloat> oBuffers;

        for (size_t j = 0; j < m_nChannel; ++j)
        {
            for (size_t k = 0; k < nInSample; ++k) { iBuffers[k] = double(iSample[k * m_nChannel + j]); }

            double* resamplerOutputBuffer;
            nI = m_resamplers[j]->process(&iBuffers[0], int(nInSample), resamplerOutputBuffer);

            if (isFirstChannel)
            {
                oBuffers.resize(nI * m_nChannel);
                isFirstChannel = false;
            }

            for (int k = 0; k < nI; ++k) { oBuffers[k * m_nChannel + j] = TFloat(resamplerOutputBuffer[k]); }
        }

        for (int k = 0; k < nI; ++k) { callback.processResampler(&oBuffers[k * m_nChannel], m_nChannel); }

        return nI;
    }

    /// <summary> This function resamples the signal assuming the input samples are ordered this way :
    /// - sample 1 of channel 1, sample 2 of channel 1, ..., sample nInSample of channel 1,
    /// - sample 1 of channel 2, sample 2 of channel 2, ..., sample nInSample of channel 2,
    /// - ...
    /// - sample 1 of channel nChannel, sample 2 of channel nChannel, ..., sample nInSample of channel nChannel,
    /// 
    /// This is convenient for resampling at the signal-processing level. </summary>
    /// <param name="callback">The callback.</param>
    /// <param name="iSample">The input sample.</param>
    /// <param name="nInSample">The n input sample.</param>
    /// <returns> the number of sample process in last channel. </returns>
    size_t resampleSampleWise(const ICallback& callback, const TFloat* iSample, const size_t nInSample)
    {
        int nI              = 0;
        bool isFirstChannel = true;

        std::vector<double> iBuffers(nInSample);
        std::vector<TFloat> oBuffers;

        for (size_t j = 0; j < m_nChannel; ++j)
        {
            for (size_t k = 0; k < nInSample; ++k) { iBuffers[k] = double(iSample[j * nInSample + k]); }

            double* resamplerOutputBuffer;
            nI = m_resamplers[j]->process(&iBuffers[0], int(nInSample), resamplerOutputBuffer);

            if (isFirstChannel)
            {
                oBuffers.resize(nI * m_nChannel);
                isFirstChannel = false;
            }

            for (int k = 0; k < nI; ++k) { oBuffers[k * m_nChannel + j] = TFloat(resamplerOutputBuffer[k]); }
        }

        for (int k = 0; k < nI; ++k) { callback.processResampler(&oBuffers[k * m_nChannel], m_nChannel); }

        return nI;
    }

protected:

    /// <summary> Trivial channel wise callback implementation. </summary>
    class CCallbackChannelWise final : public ICallback
    {
    public:
        CCallbackChannelWise(TFloat* oSample) : m_OutputSample(oSample) { }

        void processResampler(const TFloat* sample, const size_t nChannel) const override
        {
            for (size_t i = 0; i < nChannel; ++i)
            {
                *m_OutputSample = *sample;
                ++m_OutputSample;
                ++sample;
            }
        }

        mutable TFloat* m_OutputSample;
    };

    /// <summary> Trivial sample wise callback implementation. </summary>
    class CCallbackSampleWise final : public ICallback
    {
    public:
        CCallbackSampleWise(TFloat* oSample, const size_t nOutSample) : m_OutputSample(oSample), m_NOutputSample(nOutSample) { }

        void processResampler(const TFloat* sample, const size_t nChannel) const override
        {
            for (size_t i = 0; i < nChannel; ++i) { m_OutputSample[i * m_NOutputSample + m_OutputSampleIdx] = sample[i]; }
            m_OutputSampleIdx++;
            m_OutputSampleIdx %= m_NOutputSample;
        }

        TFloat* const m_OutputSample;
        size_t m_NOutputSample           = 0;
        mutable size_t m_OutputSampleIdx = 0;
    };

    size_t resampleChannelWise(TFloat* oSample, const TFloat* iSample, const size_t nInSample, const size_t /*nOutSample*/)
    {
        return this->resample(CCallbackChannelWise(oSample), iSample, nInSample);
    }

    size_t resampleSampleWise(TFloat* oSample, const TFloat* iSample, const size_t nInSample, const size_t nOutSample)
    {
        return this->resample(CCallbackSampleWise(oSample, nOutSample), iSample, nInSample);
    }

public:

    size_t resample(TFloat* oSample, const TFloat* iSample, const size_t nInSample, const size_t nOutSample = 1)
    {
        return (this->*m_fpResampleDirect)(oSample, iSample, nInSample, nOutSample);
    }

    size_t downsample(TFloat* oSample, const TFloat* iSample, const size_t nInSample, const size_t nOutSample = 1)
    {
        return resample(oSample, iSample, nInSample, nOutSample);
    }

protected:

    size_t m_nChannel  = 0;
    size_t m_iSampling = 0;
    size_t m_oSampling = 0;

    int m_nFractionalDelayFilterSample = 6;
    int m_iMaxNSampleIn                = 1024;
    double m_transitionBandPercent     = 45;
    double m_stopBandAttenuation       = 49;

    std::vector<r8b::CDSPProcessor*> m_resamplers;

    size_t (TResampler<TFloat, TStoreMode>::*m_fpResample)(const ICallback& callback, const TFloat* iSample, const size_t nInSample);
    size_t (TResampler<TFloat, TStoreMode>::*m_fpResampleDirect)(TFloat* oSample, const TFloat* iSample, const size_t nInSample, const size_t nOutSample);
};

typedef TResampler<float, EResamplerStoreModes::SampleWise> CResamplerSf;
typedef TResampler<float, EResamplerStoreModes::ChannelWise> CResamplerCf;
typedef TResampler<double, EResamplerStoreModes::SampleWise> CResamplerSd;
typedef TResampler<double, EResamplerStoreModes::ChannelWise> CResamplerCd;

typedef TResampler<float, EResamplerStoreModes::SampleWise> CDownsamplerSf;
typedef TResampler<float, EResamplerStoreModes::ChannelWise> CDownsamplerCf;
typedef TResampler<double, EResamplerStoreModes::SampleWise> CDownsamplerSd;
typedef TResampler<double, EResamplerStoreModes::ChannelWise> CDownsamplerCd;

}  // namespace Resampler
}  // namespace Common
