// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, not in upstream OpenViBE. See UG.md, 2.62. Tests the dominant frequency of the amplitude ruler's
// tooltip.

#include "SpectrumPeak.h"

#include <gtest/gtest.h>

#include <cmath>
#include <random>

using namespace OpenViBE::Plugins::SimpleVisualization;

namespace {
const double PI = std::acos(-1.0);

/// Four seconds at the given rate: the sum of the given sines, white noise of the given SD and a DC offset and drift.
std::vector<double> signal(const double sampling, const std::vector<std::pair<double, double>>& sines, const double noise,
                           const unsigned seed = 1)
{
    std::mt19937 random(seed);
    std::normal_distribution<double> gauss(0.0, noise > 0.0 ? noise : 1.0);
    std::vector<double> out(size_t(4.0 * sampling));
    for (size_t i = 0; i < out.size(); ++i)
    {
        const double t = double(i) / sampling;
        double v       = 2300.0 + 5.0 * t;  // offset and drift, which must not count
        for (const auto& [hz, amplitude] : sines) { v += amplitude * std::sin(2.0 * PI * hz * t); }
        if (noise > 0.0) { v += gauss(random); }
        out[i] = v;
    }
    return out;
}

/// Brown noise: integrated white noise, with the falling spectrum of EEG and no peak.
std::vector<double> brownNoise(const double sampling, const unsigned seed)
{
    std::mt19937 random(seed);
    std::normal_distribution<double> gauss(0.0, 1.0);
    std::vector<double> out(size_t(4.0 * sampling));
    double v = 0.0;
    for (double& x : out) { x = v += gauss(random); }
    return out;
}
}  // namespace

TEST(SpectrumPeak, findsAnAlphaRhythmBetweenBins)
{
    const auto peak = dominantFrequency(signal(512.0, { { 10.3, 20.0 } }, 5.0), 512.0);
    ASSERT_TRUE(peak.has_value());
    EXPECT_NEAR(*peak, 10.3, 0.15);
}

TEST(SpectrumPeak, takesTheHighestOfSeveralPeaks)
{
    const auto peak = dominantFrequency(signal(500.0, { { 12.0, 5.0 }, { 23.0, 20.0 }, { 40.0, 8.0 } }, 1.0), 500.0);
    ASSERT_TRUE(peak.has_value());
    EXPECT_NEAR(*peak, 23.0, 0.15);
}

TEST(SpectrumPeak, ignoresPeaksAtOrBelowSixHertz)
{
    // A strong 4 Hz wave and a weak 15 Hz one: the 15 Hz one is the peak above 6 Hz
    const auto peak = dominantFrequency(signal(256.0, { { 4.0, 100.0 }, { 15.0, 10.0 } }, 1.0), 256.0);
    ASSERT_TRUE(peak.has_value());
    EXPECT_NEAR(*peak, 15.0, 0.15);
    EXPECT_FALSE(dominantFrequency(signal(256.0, { { 4.0, 100.0 } }, 1.0), 256.0).has_value());
}

TEST(SpectrumPeak, findsNoneInNoise)
{
    for (unsigned seed = 1; seed <= 20; ++seed)
    {
        EXPECT_FALSE(dominantFrequency(signal(512.0, {}, 10.0, seed), 512.0).has_value()) << "white noise, seed " << seed;
        EXPECT_FALSE(dominantFrequency(brownNoise(512.0, seed), 512.0).has_value()) << "brown noise, seed " << seed;
    }
}

TEST(SpectrumPeak, noiseAlmostNeverPasses)
{
    // When this was written: 0 of 1000 white and 2 of 1000 brown
    size_t white = 0, brown = 0;
    for (unsigned seed = 1; seed <= 1000; ++seed)
    {
        white += dominantFrequency(signal(512.0, {}, 10.0, seed), 512.0).has_value() ? 1 : 0;
        brown += dominantFrequency(brownNoise(512.0, seed), 512.0).has_value() ? 1 : 0;
    }
    EXPECT_LE(white, 10U);
    EXPECT_LE(brown, 10U);
}

TEST(SpectrumPeak, findsAnAlphaRhythmInEEGLikeNoise)
{
    // Brown noise has the falling spectrum of EEG. An alpha rhythm of amplitude 4 on it has about ten times the noise's
    // power in its bin: dominant. One of amplitude 2, about two and a half times, is rightly not, though plain in the trace.
    for (unsigned seed = 1; seed <= 20; ++seed)
    {
        std::vector<double> eeg = brownNoise(512.0, seed);
        for (size_t i = 0; i < eeg.size(); ++i) { eeg[i] += 4.0 * std::sin(2.0 * PI * 10.6 * double(i) / 512.0); }
        const auto peak = dominantFrequency(eeg, 512.0);
        ASSERT_TRUE(peak.has_value()) << "seed " << seed;
        // The noise in the bins either side moves the estimate by up to 0.4 Hz: the tooltip shows whole hertz
        EXPECT_NEAR(*peak, 10.6, 0.5) << "seed " << seed;
    }
}

namespace {
/// Band-passed as the signal-display tutorial's Temporal Filter does, 4 to 40 Hz, order 4: by the magnitude of a
/// Butterworth low-pass and high-pass, in the frequency domain.
std::vector<double> bandPassed(const std::vector<double>& in, const double sampling)
{
    Eigen::FFT<double> fft;
    std::vector<std::complex<double>> spectrum;
    fft.fwd(spectrum, in);
    const size_t n = in.size();
    for (size_t k = 0; k < n; ++k)
    {
        const double f    = double(std::min(k, n - k)) * sampling / double(n);
        const double gain = f == 0.0 ? 0.0 : 1.0 / std::sqrt((1.0 + std::pow(f / 40.0, 8.0)) * (1.0 + std::pow(4.0 / f, 8.0)));
        spectrum[k] *= gain;
    }
    std::vector<double> out;
    fft.inv(out, spectrum);
    return out;
}
}  // namespace

TEST(SpectrumPeak, findsNoneInBandPassedNoise)
{
    // A band-pass bends the spectrum hard at its edges, which the background has to follow; noise alone, band-passed,
    // has no dominant frequency. When this was written: 1 of 400; with one curve through the whole range, 39.
    size_t found = 0;
    for (unsigned seed = 1; seed <= 200; ++seed)
    {
        found += dominantFrequency(bandPassed(brownNoise(512.0, seed), 512.0), 512.0).has_value() ? 1 : 0;
        found += dominantFrequency(bandPassed(signal(512.0, {}, 10.0, seed), 512.0), 512.0).has_value() ? 1 : 0;
    }
    EXPECT_LE(found, 4U);
}

TEST(SpectrumPeak, findsAnAlphaRhythmInBandPassedEEGLikeNoise)
{
    for (unsigned seed = 1; seed <= 20; ++seed)
    {
        std::vector<double> eeg = brownNoise(512.0, seed);
        for (size_t i = 0; i < eeg.size(); ++i) { eeg[i] += 4.0 * std::sin(2.0 * PI * 9.4 * double(i) / 512.0); }
        const auto peak = dominantFrequency(bandPassed(eeg, 512.0), 512.0);
        ASSERT_TRUE(peak.has_value()) << "seed " << seed;
        EXPECT_NEAR(*peak, 9.4, 0.5) << "seed " << seed;
    }
}

TEST(SpectrumPeak, looksNoHigherThanAHundredHertz)
{
    // A sine at 120 Hz alone is not looked at; with a weaker one at 95 Hz, that one is the peak
    EXPECT_FALSE(dominantFrequency(signal(1024.0, { { 120.0, 20.0 } }, 1.0), 1024.0).has_value());
    const auto peak = dominantFrequency(signal(1024.0, { { 120.0, 40.0 }, { 95.0, 20.0 } }, 1.0), 1024.0);
    ASSERT_TRUE(peak.has_value());
    EXPECT_NEAR(*peak, 95.0, 0.3);
}

TEST(SpectrumPeak, looksNoHigherThanFourFifthsOfNyquist)
{
    // At 128 Hz, Nyquist is 64 Hz: a sine at 58 Hz is past 0.8 of it, one at 45 Hz is not
    EXPECT_FALSE(dominantFrequency(signal(128.0, { { 58.0, 20.0 } }, 1.0), 128.0).has_value());
    const auto peak = dominantFrequency(signal(128.0, { { 45.0, 20.0 } }, 1.0), 128.0);
    ASSERT_TRUE(peak.has_value());
    EXPECT_NEAR(*peak, 45.0, 0.3);
}

TEST(SpectrumPeak, findsNoneWithoutAFullSegmentOrARate)
{
    EXPECT_FALSE(dominantFrequency(std::vector<double>(100, 1.0), 512.0).has_value());
    EXPECT_FALSE(dominantFrequency(signal(512.0, { { 10.0, 20.0 } }, 1.0), 0.0).has_value());
    EXPECT_FALSE(dominantFrequency({}, 512.0).has_value());
}

TEST(SpectrumPeak, findsNoneInAFlatSignal)
{
    EXPECT_FALSE(dominantFrequency(std::vector<double>(2048, 3.0), 512.0).has_value());
    // An offset and a ramp, which leave only rounding once the trend is out
    EXPECT_FALSE(dominantFrequency(signal(512.0, {}, 0.0), 512.0).has_value());
}
