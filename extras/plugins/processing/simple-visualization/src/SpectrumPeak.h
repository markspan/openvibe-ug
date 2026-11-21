// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, not in upstream OpenViBE. See UG.md, 2.62. The dominant frequency of a channel, for the amplitude
// ruler's tooltip. Plain C++ and Eigen, so that it can be tested without Qt.

#pragma once

#include <unsupported/Eigen/FFT>

#include <algorithm>
#include <cmath>
#include <complex>
#include <optional>
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

/// The span of signal the dominant frequency is looked for in: the newest samples, as far as the display keeps them.
constexpr double PEAK_WINDOW_SECONDS = 4.0;
/// A peak counts only above this frequency, clear of the slow waves and drift that hold most of the power of EEG.
constexpr double PEAK_MIN_HZ = 6.0;
/// A peak counts only when its power is at least this many times what the other frequencies have there.
constexpr double PEAK_MIN_RATIO = 3.0;
/// Length of the segments whose spectra are averaged (Welch): bins of 2 Hz, refined between bins. Fifteen half
/// overlapping segments in four seconds keep the bins steady enough that noise alone rarely passes PEAK_MIN_RATIO
/// (SpectrumPeakTest); with one second, seven segments, white noise passed one time in ten.
constexpr double PEAK_SEGMENT_SECONDS = 0.5;
/// A peak counts only up to this frequency: the range of EEG rhythms, below most of the mains' harmonics.
constexpr double PEAK_MAX_HZ = 100.0;
/// Nor above this part of the Nyquist frequency, at a low sampling rate: there, an amplifier's anti-aliasing filter and
/// the bend of the spectrum towards Nyquist would pass for peaks or hide them.
constexpr double PEAK_MAX_NYQUIST = 0.8;
/// Bins on either side of a peak that belong to it: a Hann window spreads a sine over five bins.
constexpr size_t PEAK_HALF_WIDTH_BINS = 2;
/// Bins on either side, beyond the peak's own, whose median is the nearby background.
constexpr size_t PEAK_NEAR_BINS = 4;

/** The strongest peak above PEAK_MIN_HZ: its frequency, and its power over the background of the other frequencies. */
struct SSpectrumPeak
{
    double hz    = 0.0;
    double ratio = 0.0;
};

/**
 * \brief The strongest peak above PEAK_MIN_HZ, whatever its ratio; dominantFrequency() keeps it when it is dominant.
 *
 * The spectrum is Welch's: segments of PEAK_SEGMENT_SECONDS, half overlapping, the last one ending at the newest sample,
 * each with its mean and linear trend taken out and a Hann window, their power spectra averaged.
 *
 * The candidates are the bins above PEAK_MIN_HZ, up to PEAK_MAX_HZ and PEAK_MAX_NYQUIST of the Nyquist frequency, that
 * are higher than both their neighbours; the highest is the peak. Its ratio is its power over "what the other
 * frequencies have there", the background, which may take in bins outside that range: the higher of two
 * estimates from the bins around it, leaving out its own PEAK_HALF_WIDTH_BINS on either side. One is a line through the
 * logarithm of their power against the logarithm of their frequency, from half the peak's frequency to twice it, both
 * octaves weighted alike: the 1/f slope on which EEG rhythms sit. The other is the median of the PEAK_NEAR_BINS nearest
 * bins on either side, which a band-pass's cliff next to the peak does not pull down, as it does the line.
 *
 * Simpler backgrounds failed SpectrumPeakTest. A plain average of the other frequencies made the lowest bins of any
 * falling spectrum a dominant peak, always. One line or parabola over the whole range found peaks in noise where the
 * spectrum bends: at the ends of the range one time in forty, and inside the edges of a band-pass one time in ten.
 *
 * Its frequency is refined between bins by a parabola through its height and its neighbours' above the background;
 * noise in those bins leaves it some 0.4 Hz uncertain.
 *
 * \param samples  The samples, oldest first, NaN and infinite ones left out
 * \param sampling Sampling rate in Hz
 * \return Nothing when there is no candidate, no full segment of samples, no sampling rate, or no signal but rounding.
 */
inline std::optional<SSpectrumPeak> strongestPeak(const std::vector<double>& samples, const double sampling)
{
    if (!(sampling > 0.0) || !std::isfinite(sampling)) { return std::nullopt; }
    const size_t n = size_t(std::lround(PEAK_SEGMENT_SECONDS * sampling));
    if (n < 8 || samples.size() < n) { return std::nullopt; }

    // Welch: half-overlapping segments, the last one ending at the newest sample
    const size_t step = n / 2;
    const size_t bins = n / 2 + 1;
    const double pi   = std::acos(-1.0);
    std::vector<double> power(bins, 0.0), hann(n), segment(n);
    for (size_t i = 0; i < n; ++i) { hann[i] = 0.5 - 0.5 * std::cos(2.0 * pi * double(i) / double(n)); }

    Eigen::FFT<double> fft;
    std::vector<std::complex<double>> spectrum;
    size_t segments = 0;
    double scale    = 0.0;  // the largest sample, against which rounding is judged
    for (const double v : samples) { scale = std::max(scale, std::abs(v)); }
    for (size_t end = samples.size(); end >= n; end -= step)
    {
        const size_t first = end - n;
        // Mean and linear trend out, by least squares against i - (n-1)/2
        double mean = 0.0, slopeNum = 0.0, slopeDen = 0.0;
        for (size_t i = 0; i < n; ++i) { mean += samples[first + i]; }
        mean /= double(n);
        for (size_t i = 0; i < n; ++i)
        {
            const double t = double(i) - 0.5 * double(n - 1);
            slopeNum += t * (samples[first + i] - mean);
            slopeDen += t * t;
        }
        const double slope = slopeDen > 0.0 ? slopeNum / slopeDen : 0.0;
        for (size_t i = 0; i < n; ++i)
        {
            const double t = double(i) - 0.5 * double(n - 1);
            segment[i]     = (samples[first + i] - mean - slope * t) * hann[i];
        }
        fft.fwd(spectrum, segment);
        for (size_t k = 0; k < bins; ++k) { power[k] += std::norm(spectrum[k]); }
        ++segments;
        if (end < n + step) { break; }
    }

    // The bins above PEAK_MIN_HZ, up to PEAK_MAX_HZ and PEAK_MAX_NYQUIST of the Nyquist frequency
    const double binHz = sampling / double(n);
    size_t lowest      = 1;
    while (lowest < bins && double(lowest) * binHz <= PEAK_MIN_HZ) { ++lowest; }
    const size_t highest = std::min({ bins - 2, size_t(PEAK_MAX_NYQUIST * double(bins - 1)), size_t(PEAK_MAX_HZ / binHz) });
    if (lowest + 2 > highest) { return std::nullopt; }  // too few bins for a peak between two neighbours

    // A signal that is flat but for its offset and trend leaves only rounding, whose bins may peak by chance. Anything
    // a converter records is far above a millionth of a millionth of its largest value.
    double above = 0.0;
    for (size_t k = lowest; k <= highest; ++k) { above += power[k]; }
    const double floor = 1e-12 * scale;
    if (!(above > floor * floor * double(n) * double(segments))) { return std::nullopt; }

    const double tiny = 1e-300;
    std::vector<double> logF(bins), logP(bins);
    for (size_t k = 1; k < bins; ++k)
    {
        logF[k] = std::log(double(k) * binHz);
        logP[k] = std::log(power[k] + tiny);
    }
    // The background at a peak: a line of logP over logF, by least squares through the bins from half the peak's
    // frequency to twice it, leaving out the peak's own, each weighted by 1/f so that both octaves count alike (bins are
    // evenly spaced in f). Local, because a spectrum bends: a band-pass makes it flat with a cliff at either edge, an
    // anti-aliasing filter drops it towards Nyquist, a noise floor flattens it. No one curve over the whole range
    // follows that; over an octave either side, a line does. x is centred on the peak for the arithmetic.
    double x0 = 0.0;
    const auto background = [&](const size_t peakBin, double c[2])
    {
        x0                 = logF[peakBin];
        const size_t first = std::max<size_t>(1, peakBin / 2);
        const size_t last  = std::min(bins - 1, 2 * peakBin);
        double s0 = 0.0, s1 = 0.0, s2 = 0.0, t0 = 0.0, t1 = 0.0;
        for (size_t k = first; k <= last; ++k)
        {
            if (k + PEAK_HALF_WIDTH_BINS >= peakBin && k <= peakBin + PEAK_HALF_WIDTH_BINS) { continue; }
            const double w = 1.0 / double(k), x = logF[k] - x0;
            s0 += w;
            s1 += w * x;
            s2 += w * x * x;
            t0 += w * logP[k];
            t1 += w * x * logP[k];
        }
        const double d = s0 * s2 - s1 * s1;
        c[1]           = d > 0.0 ? (s0 * t1 - s1 * t0) / d : 0.0;
        c[0]           = s0 > 0.0 ? (t0 - c[1] * s1) / s0 : logP[peakBin];
    };
    const auto aboveBackground = [&](const size_t k, const double c[2]) { return logP[k] - (c[0] + c[1] * (logF[k] - x0)); };

    // The highest candidate. Not the one that stands out most above the background: after a band-pass, a mains harmonic
    // far above the pass band stands out from the noise filtered with it, with next to no power.
    size_t peak = 0;
    for (size_t k = lowest; k <= highest; ++k)
    {
        if (power[k] > power[k - 1] && power[k] > power[k + 1] && (peak == 0 || power[k] > power[peak])) { peak = k; }
    }
    if (peak == 0) { return std::nullopt; }

    // Against the background of the other frequencies: the line's, or the median of the PEAK_NEAR_BINS nearest bins on
    // either side beyond the peak's own, whichever is higher. The line runs below a spectrum that bends down, as at the
    // edge of a band-pass, where the median does not; the median runs below a steep 1/f slope when the bins below the
    // peak are too few, where the line does not.
    double c[2];
    background(peak, c);
    std::vector<double> near;
    for (size_t d = PEAK_HALF_WIDTH_BINS + 1; d <= PEAK_HALF_WIDTH_BINS + PEAK_NEAR_BINS; ++d)
    {
        if (peak > d) { near.push_back(logP[peak - d]); }
        if (peak + d < bins) { near.push_back(logP[peak + d]); }
    }
    if (!near.empty())
    {
        std::sort(near.begin(), near.end());
        const size_t half   = near.size() / 2;
        const double median = near.size() % 2 ? near[half] : 0.5 * (near[half - 1] + near[half]);
        c[0]                = std::max(c[0], median);  // the background at the peak; the line's slope stays, for below
    }
    const double ratio = std::exp(aboveBackground(peak, c));

    // Between bins: the vertex of a parabola through the heights of the peak and its neighbours above the background.
    // Through their plain logarithms, the slope of the background would pull the vertex downhill, by a bin and more.
    const double left  = aboveBackground(peak - 1, c);
    const double mid   = aboveBackground(peak, c);
    const double right = aboveBackground(peak + 1, c);
    const double bend  = left - 2.0 * mid + right;
    const double shift = bend < 0.0 ? std::clamp(0.5 * (left - right) / bend, -0.5, 0.5) : 0.0;
    return SSpectrumPeak{ (double(peak) + shift) * binHz, ratio };
}

/**
 * \brief The frequency of the dominant peak above PEAK_MIN_HZ, if there is one: the strongestPeak() whose power is at
 *        least PEAK_MIN_RATIO times the background of the other frequencies there.
 *
 * \param samples  The samples, oldest first, NaN and infinite ones left out
 * \param sampling Sampling rate in Hz
 */
inline std::optional<double> dominantFrequency(const std::vector<double>& samples, const double sampling)
{
    const std::optional<SSpectrumPeak> peak = strongestPeak(samples, sampling);
    if (!peak || !(peak->ratio >= PEAK_MIN_RATIO)) { return std::nullopt; }
    return peak->hz;
}

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
