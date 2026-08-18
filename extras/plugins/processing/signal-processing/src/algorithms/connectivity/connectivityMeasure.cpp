///-------------------------------------------------------------------------------------------------
///
/// \file connectivityMeasure.cpp
/// \brief All connectivity measure metrics.
/// \author Arthur Desbois (Inria).
/// \version 1.0
/// \date 30/10/2020
///
/// \copyright Copyright(C) 2020-2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
///-------------------------------------------------------------------------------------------------

#include <cmath>
#include <complex>
#include <iostream>

#include <Eigen/Dense>
#include <unsupported/Eigen/FFT>

#include "connectivityMeasure.hpp"
#include "spectrumTools.hpp"
#include <openvibe/ov_all.h>
#include <windowFunctions.hpp>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

//************************************************************************
//******* Connectivity measurements & associated helper functions ********
//************************************************************************
bool ConnectivityMeasure::initializeWelch(const EConnectMetric metric, uint64_t sampRate, const size_t nbChannels, const size_t fftSize, const bool dcRemoval,
                                          const EConnectWindowMethod windowMethod, const int windowLength, const int windowOverlap)
{
    initialize(metric, sampRate, nbChannels, fftSize, dcRemoval);
    
    // Welch specific
    m_psdMode = EPsdMode::Welch;
    m_windowMethod = windowMethod;
    m_windowLength = windowLength; // size of one welch window (samples)
    m_windowOverlap = windowOverlap; // overlap btw windows (%)
    m_windowOverlapSamples = int(std::floor(double(m_windowLength * windowOverlap) / 100.0));
        
    m_window = Eigen::VectorXd::Zero(m_windowLength);
    switch (m_windowMethod) {
        case EConnectWindowMethod::Hamming: WindowFunctions::hamming(m_window, m_windowLength);
            break;
        case EConnectWindowMethod::Hann: WindowFunctions::hann(m_window, m_windowLength);
            break;
        case EConnectWindowMethod::Welch: WindowFunctions::welch(m_window, m_windowLength);
            break;
    }
    // window normalization constant
    m_u = 0;
    for (Eigen::Index i = 0; i < m_window.size(); ++i) { 
        m_u += std::pow(m_window(i), 2); 
    }

    // Instantiate the big spectra & x-spectra arrays
    m_dft.resize(m_nbChannels); // vector of channels x fftsize
    m_psd.resize(m_nbChannels); // vector of channels x fftsize
    m_cpsd.resize(m_nbChannels); // matrix of channels x channels x fftsize
    for (size_t chan = 0; chan < m_nbChannels; chan++)  {
        m_cpsd[chan].resize(m_nbChannels); 
    }

    return true;
}

bool ConnectivityMeasure::initializeBurg(const EConnectMetric metric, uint64_t sampRate, const size_t nbChannels, const size_t fftSize, const bool dcRemoval,
                                         const int autoRegOrder)
{
    initialize(metric, sampRate, nbChannels, fftSize, dcRemoval);
    
    // Burg specific
    m_psdMode = EPsdMode::Burg;
    m_autoRegOrder = autoRegOrder;      
    m_expMat = generateExpMat(m_autoRegOrder, m_fftSize, m_sampRate);

    // chan1 x chan2 x 2 x 2 x nfft cdoubles
    m_mvarCoeffsMatrix.resize(m_nbChannels);
    for(size_t chan1 = 0; chan1 < m_nbChannels; chan1++) {
        m_mvarCoeffsMatrix[chan1].resize(m_nbChannels);
        for (size_t chan2 = 0; chan2 < m_nbChannels; chan2++) {
            m_mvarCoeffsMatrix[chan1][chan2].resize(2);
            m_mvarCoeffsMatrix[chan1][chan2][0].resize(2);
            m_mvarCoeffsMatrix[chan1][chan2][1].resize(2);
            m_mvarCoeffsMatrix[chan1][chan2][0][0].resize(m_fftSize);
            m_mvarCoeffsMatrix[chan1][chan2][0][1].resize(m_fftSize);
            m_mvarCoeffsMatrix[chan1][chan2][1][0].resize(m_fftSize);
            m_mvarCoeffsMatrix[chan1][chan2][1][1].resize(m_fftSize);
        }
    }
    return true;
}

bool ConnectivityMeasure::initialize(const EConnectMetric metric, uint64_t sampRate, const size_t nbChannels, const size_t fftSize, const bool dcRemoval)
{
    m_metric = metric;
    m_sampRate = sampRate;
    m_nbChannels = nbChannels;
    m_fftSize = fftSize;
    m_dcRemoval = dcRemoval;

    return true;
}

bool ConnectivityMeasure::process(const std::vector<Eigen::VectorXd>& samples, std::vector<Eigen::MatrixXd>& connectivityMatrix, const EPsdMode mode, const std::vector<size_t>& vLookup)
{
    switch(m_psdMode) {
        case EPsdMode::Welch: return processWelch(samples, connectivityMatrix, vLookup);
        case EPsdMode::Burg:  return processBurg(samples, connectivityMatrix, vLookup);
    }
}

bool ConnectivityMeasure::processWelch(const std::vector<Eigen::VectorXd>& samples, std::vector<Eigen::MatrixXd>& connectivityMatrix, const std::vector<size_t>& vLookup)
{
    // Windowing
    const size_t windowShift = m_window.size() - m_windowOverlapSamples;
    const size_t nbWindows   = size_t(std::floor(double(samples[0].size() - windowShift) / double(windowShift)));

    // Remove the DC component of each channel
    std::vector<Eigen::VectorXd> mySamples;
    if (m_dcRemoval) {
        for (size_t chan = 0; chan < m_nbChannels; chan++) {
            Eigen::VectorXd temp = samples[chan] - Eigen::VectorXd::Ones(samples[chan].size()) * samples[chan].mean();
            mySamples.push_back(temp);
        }
    }
    else { mySamples = samples; }

    // TODO : don't recompute the whole set of DFTs, because connectivity measures overlap !
    // Periodigrams (DFTs)
    for (size_t chan = 0; chan < m_nbChannels; chan++) { 
        periodogram(mySamples[chan], m_dft[chan], nbWindows); 
    }

    // PSDs
    for (size_t chan = 0; chan < m_nbChannels; chan++) { 
        powerSpectralDensity(m_dft[chan], m_psd[chan], nbWindows); 
    }

    // Cross-SPECTRA
    for (size_t chan1 = 0; chan1 < vLookup.size(); chan1++) {
        for (size_t chan2 = 0; chan2 < m_nbChannels; chan2++) { 
            if (vLookup[chan1] != chan2) {
                crossSpectralDensity(m_dft[vLookup[chan1]], m_dft[chan2], m_cpsd[vLookup[chan1]][chan2], nbWindows);
            }
        }
    }

    // Use PSDs & x-spectra to compute the connectivity matrix
    std::vector<std::vector<std::vector<std::vector<Eigen::VectorXcd>>>> placeHolder; // empty vector, to use same fcts...
    switch (m_metric) {
        case EConnectMetric::Coherence: return coherence(placeHolder, connectivityMatrix, vLookup);
        case EConnectMetric::MagnitudeSquaredCoherence: return magnitudeSquaredCoherence(placeHolder, connectivityMatrix, vLookup);
        case EConnectMetric::ImaginaryCoherence: return imaginaryCoherence(placeHolder, connectivityMatrix, vLookup);
        case EConnectMetric::AbsImaginaryCoherence: return absImaginaryCoherence(placeHolder, connectivityMatrix, vLookup);
        default: return coherence(placeHolder, connectivityMatrix, vLookup);
    }
}

bool ConnectivityMeasure::processBurg(const std::vector<Eigen::VectorXd>& samples, std::vector<Eigen::MatrixXd>& connectivityMatrix, const std::vector<size_t>& vLookup)
{
    // NOT MULTITHREADED VERSION - for reference / debugging purpose
    /*for (size_t chan1 = 0; chan1 < m_nbChannels; chan1++) {
        for (size_t chan2 = chan1+1; chan2 < m_nbChannels; chan2++) {           
            if (!mvarSpectralEstimation(samples[chan1], samples[chan2], m_mvarCoeffsMatrix[chan1][chan2], m_autoRegOrder, m_fftSize, m_sampRate, m_dcRemoval, m_expMat)) { 
                return false; 
            }           
        }
    }*/

    // THREAD POOL (MAX THREADS / STANDALONE without init)
    for (size_t chan1 = 0; chan1 < vLookup.size(); chan1++) {
        size_t chan1idx = vLookup[chan1];
        size_t startingIdxChan2 = 0;

        // if we consider all channels, override this channel idx nonsense
        if (vLookup.size() == m_nbChannels) {
            startingIdxChan2 = chan1 + 1;
            chan1idx = chan1;
        }

        for (size_t chan2 = startingIdxChan2; chan2 < m_nbChannels; chan2++) {

            if (chan1idx != chan2) {
                m_threadPoolMax.QueueJob([&, chan1idx, chan2] {
                    // No need to use mutexes : samples data can be read from multiple threads without issue.
                    // and writing occurs to indices of m_mvarCoeffsMatrix that are independent for every threads
                    mvarSpectralEstimation(samples[chan1idx], samples[chan2], m_mvarCoeffsMatrix[chan1idx][chan2], m_autoRegOrder, m_fftSize, m_sampRate, m_dcRemoval, m_expMat);
                    });
            }
        }

    }
    // Wait until all jobs have been processed
    m_threadPoolMax.waitUntilCompleted();

    // Use MVAR results to compute the connectivity matrix
    switch (m_metric) {
        case EConnectMetric::Coherence: return coherence(m_mvarCoeffsMatrix, connectivityMatrix, vLookup);
        case EConnectMetric::MagnitudeSquaredCoherence: return magnitudeSquaredCoherence(m_mvarCoeffsMatrix, connectivityMatrix, vLookup);
        case EConnectMetric::ImaginaryCoherence: return imaginaryCoherence(m_mvarCoeffsMatrix, connectivityMatrix, vLookup);
        case EConnectMetric::AbsImaginaryCoherence: return absImaginaryCoherence(m_mvarCoeffsMatrix, connectivityMatrix, vLookup);
        default: return coherence(m_mvarCoeffsMatrix, connectivityMatrix, vLookup);
    }
}


bool ConnectivityMeasure::periodogram(const Eigen::VectorXd& input, Eigen::MatrixXcd& periodograms, const size_t& nSegments)
{
    periodograms = Eigen::MatrixXcd::Zero(Eigen::Index(m_fftSize), Eigen::Index(nSegments));

    // Cut input vector into segments, and apply window to each segment
    for (Eigen::Index k = 0; k < Eigen::Index(nSegments); ++k) {
        Eigen::VectorXd segment = Eigen::VectorXd::Zero(Eigen::Index(m_fftSize));

        for (Eigen::Index i = 0; i < Eigen::Index(m_windowLength); ++i) { 
            segment(i) = input(i + k * m_windowOverlapSamples) * m_window(i); 
        }

        Eigen::VectorXcd dft = Eigen::VectorXcd::Zero(Eigen::Index(m_fftSize));
        m_fft.fwd(dft, segment, Eigen::Index(m_fftSize));
        periodograms.col(k) = dft;

    }
    return true;
}


bool ConnectivityMeasure::powerSpectralDensity(const Eigen::MatrixXcd& dft, Eigen::VectorXd& output, const size_t& nSegments)
{
    // output(i) will be the power for the band i across segments (time) as summed from the periodogram
    output = Eigen::VectorXd::Zero(Eigen::Index(m_fftSize));

    for (Eigen::Index k = 0; k < Eigen::Index(nSegments); ++k) { 
        output += dft.col(k).cwiseAbs2(); 
    }
    const double factor = double(nSegments) * m_u;
    output /= factor;
    return true;
}


bool ConnectivityMeasure::crossSpectralDensity(const Eigen::MatrixXcd& dft1, const Eigen::MatrixXcd& dft2, Eigen::VectorXcd& output, const size_t& nSegments)
{
    output = Eigen::VectorXcd::Zero(Eigen::Index(m_fftSize));

    for (Eigen::Index k = 0; k < Eigen::Index(nSegments); ++k) { 
        output += dft2.col(k).cwiseProduct(dft1.col(k).conjugate()); 
    }
    const double factor = double(nSegments) * m_u;
    output /= factor;
    return true;
}

bool ConnectivityMeasure::coherence(const std::vector<std::vector<std::vector<std::vector<Eigen::VectorXcd>>>>& mvarMatrix, std::vector<Eigen::MatrixXd>& connectivityMatrix, const std::vector<size_t>& vLookup)
{
    // TODO : coherence is a complex matrix. Here we also used MSC... but with sqrt in the denominator.

    if(m_psdMode == EPsdMode::Welch) {

        // PSDs and CPSD are directly available as class members m_psd and m_cpsd
        for (size_t chan1 = 0; chan1 < vLookup.size(); chan1++) {
            size_t chan1idx = vLookup[chan1];
            size_t startingIdxChan2 = 0;
            if (vLookup.size() == m_nbChannels) {
                chan1idx = chan1;
                startingIdxChan2 = chan1 + 1;
            }

            connectivityMatrix[chan1idx].row(chan1) = Eigen::VectorXd::Zero(m_psd[0].size());

            for (size_t chan2 = startingIdxChan2; chan2 < m_nbChannels; chan2++) {
                if (chan1idx != chan2) {
                    connectivityMatrix[chan1].row(chan2) = m_cpsd[chan1idx][chan2].cwiseAbs2().cwiseQuotient(m_psd[chan1idx].cwiseProduct(m_psd[chan2])).cwiseSqrt();
                    if (vLookup.size() == m_nbChannels) {
                        connectivityMatrix[chan2].row(chan1) = connectivityMatrix[chan1].row(chan2); // diagonal matrix
                    }
                }
            }
        }

    }
    else { // m_psdMode == EPsdMode::Burg

     // For each channel pair : Coh = S(0,1) / sqrt(S(0,0).*S(1,1))
     // Input matrix dimensions : chan1 x chan2 x nfft x Matrix2cd (S11,S12),(S21,S22))
     // mvarMatrix[chan1][chan2] is matrix S
     // mvarMatrix[chan1][chan2][0][0] is psd of chan1
     // mvarMatrix[chan1][chan2][1][1] is psd of chan2
     // mvarMatrix[chan1][chan2][0][1] is cpsd of pair of channels

        for (size_t chan1 = 0; chan1 < vLookup.size(); chan1++) {

            size_t chan1idx = vLookup[chan1];
            size_t startingIdxChan2 = 0;
            if (vLookup.size() == m_nbChannels) {
                chan1idx = chan1;
                startingIdxChan2 = chan1 + 1;
            }

            connectivityMatrix[chan1].row(chan1) = Eigen::VectorXd::Zero(m_fftSize);

            for (size_t chan2 = startingIdxChan2; chan2 < m_nbChannels; chan2++) {
                {
                    if (chan1idx != chan2) {
                        connectivityMatrix[chan1].row(chan2) = mvarMatrix[chan1idx][chan2][0][1].cwiseAbs2().cwiseQuotient(mvarMatrix[chan1idx][chan2][0][0].real().cwiseProduct(mvarMatrix[chan1idx][chan2][1][1].real())).cwiseSqrt();
                        if (vLookup.size() == m_nbChannels) {
                            connectivityMatrix[chan2].row(chan1) = connectivityMatrix[chan1].row(chan2); // diagonal matrix
                        }
                    }
                }
            }
        }
    }

    return true;
}

bool ConnectivityMeasure::magnitudeSquaredCoherence(const std::vector<std::vector<std::vector<std::vector<Eigen::VectorXcd>>>>& mvarMatrix, std::vector<Eigen::MatrixXd>& connectivityMatrix, const std::vector<size_t>& vLookup)
{
    if(m_psdMode == EPsdMode::Welch) {

        // PSDs and CPSD are directly available as class members m_psd and m_cpsd
        for (size_t chan1 = 0; chan1 < vLookup.size(); chan1++) {
            size_t chan1idx = vLookup[chan1];
            size_t startingIdxChan2 = 0;
            if (vLookup.size() == m_nbChannels) {
                chan1idx = chan1;
                startingIdxChan2 = chan1 + 1;
            }
            connectivityMatrix[chan1].row(chan1) = Eigen::VectorXd::Zero(m_psd[0].size());
            for (size_t chan2 = startingIdxChan2; chan2 < m_nbChannels; chan2++) {
                if (chan1 != chan2) {
                    connectivityMatrix[chan1].row(chan2) = m_cpsd[chan1idx][chan2].cwiseAbs2().cwiseQuotient(m_psd[chan1idx].cwiseProduct(m_psd[chan2]));
                    if (vLookup.size() == m_nbChannels) {
                        connectivityMatrix[chan2].row(chan1) = connectivityMatrix[chan1].row(chan2); // diagonal matrix
                    }
                }
            }
        }

    } else { // m_psdMode == EPsdMode::Burg

        // For each channel pair : MSC = abs(S(0,1)).^2 / (S(0,0).*S(1,1))
        // Input matrix dimensions : chan1 x chan2 x 2 x 2 x nfft complexdoubles
        // mvarMatrix[chan1][chan2] is matrix S
        // mvarMatrix[chan1][chan2][0][0] is psd of chan1
        // mvarMatrix[chan1][chan2][1][1] is psd of chan2
        // mvarMatrix[chan1][chan2][0][1] is cpsd of pair of channels
        for (size_t chan1 = 0; chan1 < vLookup.size(); chan1++) {
            size_t chan1idx = vLookup[chan1];
            size_t startingIdxChan2 = 0;
            if (vLookup.size() == m_nbChannels) {
                chan1idx = chan1;
                startingIdxChan2 = chan1 + 1;
            }

            connectivityMatrix[chan1].row(chan1) = Eigen::VectorXd::Zero(m_fftSize);
            for (size_t chan2 = startingIdxChan2; chan2 < m_nbChannels; chan2++) {
                if (chan1idx != chan2) {
                    connectivityMatrix[chan1].row(chan2) = mvarMatrix[chan1idx][chan2][0][1].cwiseAbs2().cwiseQuotient(mvarMatrix[chan1idx][chan2][0][0].real().cwiseProduct(mvarMatrix[chan1idx][chan2][1][1].real()));
                    if (vLookup.size() == m_nbChannels) {
                        connectivityMatrix[chan2].row(chan1) = connectivityMatrix[chan1].row(chan2); // diagonal matrix
                    }
                }
            }
        }

        
    }

    return true;
}

bool ConnectivityMeasure::imaginaryCoherence(const std::vector<std::vector<std::vector<std::vector<Eigen::VectorXcd>>>>& mvarMatrix, std::vector<Eigen::MatrixXd>& connectivityMatrix, const std::vector<size_t>& vLookup)
{
    if(m_psdMode == EPsdMode::Welch) {

        // PSDs and CPSD are directly available as class members m_psd and m_cpsd
        for (size_t chan1 = 0; chan1 < vLookup.size(); chan1++) {
            size_t chan1idx = vLookup[chan1];
            size_t startingIdxChan2 = 0;
            if (vLookup.size() == m_nbChannels) {
                chan1idx = chan1;
                startingIdxChan2 = chan1 + 1;
            }

            connectivityMatrix[chan1idx].row(chan1) = Eigen::VectorXd::Zero(m_psd[0].size());

            for (size_t chan2 = startingIdxChan2; chan2 < m_nbChannels; chan2++) {
                if (chan1idx != chan2) {
                    connectivityMatrix[chan1].row(chan2) = m_cpsd[chan1idx][chan2].imag().cwiseQuotient(m_psd[chan1idx].cwiseProduct(m_psd[chan2]).cwiseSqrt());
                    if (vLookup.size() == m_nbChannels) {
                        connectivityMatrix[chan2].row(chan1) = connectivityMatrix[chan1].row(chan2); // diagonal matrix
                    }
                }
            }
        }

    } else { // m_psdMode == EPsdMode::Burg

        // For each channel pair : imCoh = im(S(0,1)) / sqrt(S(0,0).*S(1,1))
        // Input matrix dimensions : chan1 x chan2 x nfft x Matrix2cd (S11,S12),(S21,S22))
        // mvarMatrix[chan1][chan2] is matrix S
        // mvarMatrix[chan1][chan2][0][0] is psd of chan1
        // mvarMatrix[chan1][chan2][1][1] is psd of chan2
        // mvarMatrix[chan1][chan2][0][1] is cpsd of pair of channels

        for (size_t chan1 = 0; chan1 < vLookup.size(); chan1++) {
            size_t chan1idx = vLookup[chan1];
            size_t startingIdxChan2 = 0;
            if (vLookup.size() == m_nbChannels) {
                chan1idx = chan1;
                startingIdxChan2 = chan1 + 1;
            }

            connectivityMatrix[chan1].row(chan1) = Eigen::VectorXd::Zero(m_fftSize);
            for (size_t chan2 = startingIdxChan2; chan2 < m_nbChannels; chan2++) {
                if (chan1idx != chan2) {
                    connectivityMatrix[chan1].row(chan2) = mvarMatrix[chan1idx][chan2][0][1].imag().cwiseQuotient(mvarMatrix[chan1idx][chan2][0][0].real().cwiseProduct(mvarMatrix[chan1idx][chan2][1][1].real()).cwiseSqrt());
                    if (vLookup.size() == m_nbChannels) {
                        connectivityMatrix[chan2].row(chan1) = connectivityMatrix[chan1].row(chan2); // diagonal matrix
                    }
                }
            }
        }
    }

    return true;
}

bool ConnectivityMeasure::absImaginaryCoherence(const std::vector<std::vector<std::vector<std::vector<Eigen::VectorXcd>>>>& mvarMatrix, std::vector<Eigen::MatrixXd>& connectivityMatrix, const std::vector<size_t>& vLookup)
{
    if(m_psdMode == EPsdMode::Welch) {

        // PSDs and CPSD are directly available as class members m_psd and m_cpsd
        for (size_t chan1 = 0; chan1 < vLookup.size(); chan1++) {
            size_t chan1idx = vLookup[chan1];
            size_t startingIdxChan2 = 0;
            if (vLookup.size() == m_nbChannels) {
                chan1idx = chan1;
                startingIdxChan2 = chan1 + 1;
            }

            connectivityMatrix[chan1idx].row(chan1) = Eigen::VectorXd::Zero(m_psd[0].size());

            for (size_t chan2 = startingIdxChan2; chan2 < m_nbChannels; chan2++) {
                if (chan1idx != chan2) {
                    connectivityMatrix[chan1].row(chan2) = m_cpsd[chan1idx][chan2].imag().cwiseAbs().cwiseQuotient(m_psd[chan1idx].cwiseProduct(m_psd[chan2]).cwiseSqrt());
                    if (vLookup.size() == m_nbChannels) {
                        connectivityMatrix[chan2].row(chan1) = connectivityMatrix[chan1].row(chan2); // diagonal matrix
                    }
                }
            }
        }

    } else { // m_psdMode == EPsdMode::Burg

        // For each channel pair : MSC = abs(im(S(0,1))) / sqrt(S(0,0).*S(1,1))
        // Input matrix dimensions : chan1 x chan2 x nfft x Matrix2cd (S11,S12),(S21,S22))
        // mvarMatrix[chan1][chan2] is matrix S
        // mvarMatrix[chan1][chan2][0][0] is psd of chan1
        // mvarMatrix[chan1][chan2][1][1] is psd of chan2
        // mvarMatrix[chan1][chan2][0][1] is cpsd of pair of channels

        for (size_t chan1 = 0; chan1 < vLookup.size(); chan1++) {
            size_t chan1idx = vLookup[chan1];
            size_t startingIdxChan2 = 0;
            if (vLookup.size() == m_nbChannels) {
                chan1idx = chan1;
                startingIdxChan2 = chan1 + 1;
            }

            connectivityMatrix[chan1].row(chan1) = Eigen::VectorXd::Zero(m_fftSize);
            for (size_t chan2 = startingIdxChan2; chan2 < m_nbChannels; chan2++) {
                if (chan1idx != chan2) {
                    connectivityMatrix[chan1].row(chan2) = mvarMatrix[chan1idx][chan2][0][1].imag().cwiseAbs().cwiseQuotient(mvarMatrix[chan1idx][chan2][0][0].real().cwiseProduct(mvarMatrix[chan1idx][chan2][1][1].real()).cwiseSqrt());
                    if (vLookup.size() == m_nbChannels) {
                        connectivityMatrix[chan2].row(chan1) = connectivityMatrix[chan1].row(chan2); // diagonal matrix
                    }
                }
            }
        }
    }
    return true;
}


}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
