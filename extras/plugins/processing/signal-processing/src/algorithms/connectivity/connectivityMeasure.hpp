///-------------------------------------------------------------------------------------------------
///
/// \file connectivityMeasure.hpp
/// \brief All connectivity measure metrics.
/// \author Arthur Desbois (Inria).
/// \version 1.0
/// \date 30/10/2020
///
/// \copyright Copyright (C) 2020-2022 Inria
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

#pragma once

#include <string>

#include <Eigen/Dense>
#include <unsupported/Eigen/FFT>

#include <openvibe/ov_all.h>
#include "threadPool.hpp"


namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

enum class EPsdMode {Welch, Burg};

/// \brief  Convert Psd algo to string.
/// \param mode The Psd algo.
/// \return The PSD algo as human readable string
inline std::string toString(const EPsdMode mode)
{
    switch (mode) {
        case EPsdMode::Welch: return "Welch";
        case EPsdMode::Burg: return "Burg";
        default: return "Undefined";
    }
}

/// \brief  Convert string to Psd Mode.
/// \param mode The psd mode as a string
/// \return \ref EPsdMode
inline EPsdMode StringToMode(const std::string& mode)
{
    if (mode == "Welch") {return EPsdMode::Welch;}
    if (mode == "Burg") {return EPsdMode::Burg;}
    return EPsdMode::Burg; // default
}

/// \brief  Enumeration of metrics.
enum class EConnectMetric { Coherence, MagnitudeSquaredCoherence, ImaginaryCoherence, AbsImaginaryCoherence };

/// \brief  Convert Metrics to string.
/// \param metric The metric.
/// \return The metric as human readable string
inline std::string toString(const EConnectMetric metric)
{
    switch (metric) {
        case EConnectMetric::Coherence: return "Coherence";
        case EConnectMetric::MagnitudeSquaredCoherence: return "MagnitudeSquaredCoherence";
        case EConnectMetric::ImaginaryCoherence: return "ImaginaryCoherence";
        case EConnectMetric::AbsImaginaryCoherence: return "AbsImaginaryCoherence";
        default: return "Invalid";
    }
}

/// \brief  Convert string to Metric.
/// \param metric The metric as a string
/// \return \ref EConnectMetric
inline EConnectMetric StringToMetric(const std::string& metric)
{
    if (metric == "Coherence") { return EConnectMetric::Coherence; }
    if (metric == "MagnitudeSquaredCoherence") { return EConnectMetric::MagnitudeSquaredCoherence; }
    if (metric == "ImaginaryCoherence") { return EConnectMetric::ImaginaryCoherence; }
    if (metric == "AbsImaginaryCoherence") { return EConnectMetric::AbsImaginaryCoherence; }
    return EConnectMetric::Coherence; // default
}

/// \brief  Enumeration of windowing methods
enum class EConnectWindowMethod { Hamming, Hann, Welch };

/// \brief  Convert Window method to string.
/// \param method The windowing method
/// \return the window method as human readable string
inline std::string toString(const EConnectWindowMethod method)
{
    switch (method) {
        case EConnectWindowMethod::Hamming: return "Hamming";
        case EConnectWindowMethod::Hann: return "Hann";
        case EConnectWindowMethod::Welch: return "Welch";
    }
}

class ConnectivityMeasure
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    ConnectivityMeasure() {m_threadPoolMax.Start(std::thread::hardware_concurrency()); }
    ~ConnectivityMeasure(){m_threadPoolMax.Stop();}

    /// \brief Initialize the connectivity measurement class, Welch specific parameters
    /// \param metric The chosen metric (coherence, imaginary part of coherence...)
    /// \param sampRate The signal's sampling rate
    /// \param nbChannels The number of processed channels
    /// \param fftSize The size of used FFT / output PSD
    /// \param dcRemoval Activate Detrending/DC Removal
    /// \param windowsMethod The windowing method (Welch)
    /// \param windowLength The length of one window of processing (Welch)
    /// \param windowOverlap The overlap btw windows (Welch)
    bool initializeWelch(const EConnectMetric metric, uint64_t sampling, const size_t nbChannels, const size_t fftSize, const bool dcRemoval, 
        const EConnectWindowMethod windowMethod, const int windowLength, const int windowOverlap);

    /// \brief Initialize the connectivity measurement class, Burg specific parameters
    /// \param metric The chosen metric (coherence, imaginary part of coherence...)
    /// \param sampRate The signal's sampling rate
    /// \param nbChannels The number of processed channels
    /// \param fftSize The size of used FFT / output PSD
    /// \param dcRemoval Activate Detrending/DC Removal
    /// \param autoRegOrder The MVAR model order (Burg)
    bool initializeBurg(const EConnectMetric metric, uint64_t sampling, const size_t nbChannels, const size_t fftSize, const bool dcRemoval, 
        const int autoRegOrder);

    /// \brief Select the function to call for the connectivity measurement.
    /// \param samples The input data set \f$x\f$. With \f$ nChan \f$ channels and \f$ nSamp \f$ samples per channel
    /// \param connectivityMatrix The connectivity matrix
    bool process(const std::vector<Eigen::VectorXd>& samples, std::vector<Eigen::MatrixXd>& connectivityMatrix, const EPsdMode mode, const std::vector<size_t>& vLookup);

protected:
    bool processWelch(const std::vector<Eigen::VectorXd>& samples,  std::vector<Eigen::MatrixXd>& connectivityMatrix, const std::vector<size_t>& vLookup);
    bool processBurg(const std::vector<Eigen::VectorXd>& samples, std::vector<Eigen::MatrixXd>& connectivityMatrix, const std::vector<size_t>& vLookup);

private:
    /// \brief Initialize the connectivity measurement class, with common parameters
    /// \param metric The chosen metric (coherence, imaginary part of coherence...)
    /// \param sampRate The signal's sampling rate
    /// \param nbChannels The number of processed channels
    /// \param fftSize The size of used FFT / output PSD
    /// \param dcRemoval Activate Detrending/DC Removal
    bool initialize(const EConnectMetric metric, uint64_t sampling, const size_t nbChannels, const size_t fftSize, const bool dcRemoval);

    /// \brief Calculation of the Coherence
    /// \f$ Pxx = PSD of x \f$ // \f$ Pyy = PSD of y \f$ // \f$ Sxy = cross spectral density of x and y \f$
    /// \f$ MSC = \frac{\left| Sxy \right|}{sqrt{(Pxx.Pyy)} } \f$
    ///
    /// The connectivity matrix is symmetrical, so we only need to compute its top part
    /// and discard the diagonal (no need to compute connectivity of a single channel...)
    /// eg [0 1 2 3] :
    /// [   01 02 03]
    /// [      12 13]
    /// [         23]
    /// [           ]
    /// \param mvarMatrix The MVAR coefficients, vector of size nbChannels x nbChannels x 2 x 2 x frequency_taps. Can be empty if not using Burg/MVAR (but still initialized!)
    /// \param connectivityMatrix The connectivity matrix as a vector of 2D Matrices of size nbChannels x (nbChannels x frequency_taps)
    /// \return True if it succeeds, false if it fails.
    /// \todo don't recompute the whole table, because connectivity measures overlap !
    bool coherence(const std::vector<std::vector<std::vector<std::vector<Eigen::VectorXcd>>>>& mvarMatrix, std::vector<Eigen::MatrixXd>& connectivityMatrix, const std::vector<size_t>& vLookup);

    /// \brief Calculation of the Magnitude Squared Coherence
    /// \f$ Pxx = PSD of x \f$ // \f$ Pyy = PSD of y \f$ // \f$ Sxy = cross spectral density of x and y \f$
    /// \f$ MSC = \frac{\left| Sxy \right|^2}{(Pxx.Pyy)} \f$
    ///
    /// The connectivity matrix is symmetrical, so we only need to compute its top part
    /// and discard the diagonal (no need to compute connectivity of a single channel...)
    /// eg [0 1 2 3] :
    /// [   01 02 03]
    /// [      12 13]
    /// [         23]
    /// [           ]
    /// \param mvarMatrix The MVAR coefficients, vector of size nbChannels x nbChannels x 2 x 2 x frequency_taps. Can be empty if not using Burg/MVAR (but still initialized!)
    /// \param connectivityMatrix The connectivity matrix as a vector of 2D Matrices of size nbChannels x (nbChannels x frequency_taps)
    /// \return True if it succeeds, false if it fails.
    /// \todo don't recompute the whole table, because connectivity measures overlap !
    bool magnitudeSquaredCoherence(const std::vector<std::vector<std::vector<std::vector<Eigen::VectorXcd>>>>& mvarMatrix, std::vector<Eigen::MatrixXd>& connectivityMatrix, const std::vector<size_t>& vLookup);

    /// \brief Calculation of the Imaginary part of the coherence
    /// \f$ Pxx = PSD of x \f$ // \f$ Pyy = PSD of y \f$ // \f$ Sxy = cross spectral density of x and y \f$
    /// \f$ ImC = \frac{Im(Sxy)}{sqrt{Pxx.Pyy} } \f$
    /// The connectivity matrix is symmetrical, so we only need to compute
    /// its top part, and discard the diagonal (no need to compute connectivity of a single channel...)
    /// eg [0 1 2 3] :
    /// [   01 02 03]
    /// [      12 13]
    /// [         23]
    /// [           ]
    /// \param mvarMatrix The MVAR coefficients, vector of size nbChannels x nbChannels x 2 x 2 x frequency_taps. Can be empty if not using Burg/MVAR (but still initialized!)
    /// \param connectivityMatrix The connectivity matrix as a vector of 2D Matrices of size nbChannels x (nbChannels x frequency_taps)
    /// \return True if it succeeds, false if it fails.
    /// \todo don't recompute the whole table, because connectivity measures overlap !
    bool imaginaryCoherence(const std::vector<std::vector<std::vector<std::vector<Eigen::VectorXcd>>>>& mvarMatrix, std::vector<Eigen::MatrixXd>& connectivityMatrix, const std::vector<size_t>& vLookup);

    /// \brief Calculation of the absolute value of the Imaginary part of the coherence
    /// \f$ Pxx = PSD of x \f$ // \f$ Pyy = PSD of y \f$ // \f$ Sxy = cross spectral density of x and y \f$
    /// \f$ ImC = \frac{\left| Im(Sxy) \right|}{sqrt{Pxx.Pyy} } \f$
    /// The connectivity matrix is symmetrical, so we only need to compute
    /// its top part, and discard the diagonal (no need to compute connectivity of a single channel...)
    /// eg [0 1 2 3] :
    /// [   01 02 03]
    /// [      12 13]
    /// [         23]
    /// [           ]
    /// \param mvarMatrix The MVAR coefficients, vector of size nbChannels x nbChannels x 2 x 2 x frequency_taps. Can be empty if not using Burg/MVAR (but still initialized!)
    /// \param connectivityMatrix The connectivity matrix as a vector of 2D Matrices of size nbChannels x (nbChannels x frequency_taps)
    /// \return True if it succeeds, false if it fails.
    /// \todo don't recompute the whole table, because connectivity measures overlap !
    bool absImaginaryCoherence(const std::vector<std::vector<std::vector<std::vector<Eigen::VectorXcd>>>>& mvarMatrix, std::vector<Eigen::MatrixXd>& connectivityMatrix, const std::vector<size_t>& vLookup);

    /// \brief Generates periodigram of signal
    /// \param input The signal
    /// \param periodograms The generated periodigram
    /// \param nSegments Number of segments
    /// \return True on succes, false otherwise
    bool periodogram(const Eigen::VectorXd& input, Eigen::MatrixXcd& periodograms, const size_t& nSegments);

    /// \brief Computes the spectral density of a spectrum
    /// \param dft The discrete fourier transform
    /// \param output The generated spectral density
    /// \param nSegments Number of segments
    /// \return True on success, false otherwise
    bool powerSpectralDensity(const Eigen::MatrixXcd& dft, Eigen::VectorXd& output, const size_t& nSegments);

    /// \brief Computes cross spectral density of 2 spectra
    /// Only computes the top diagonal of the CPSD matrix
    /// eg [0 1 2 3] :
    /// [00 01 02 03]
    /// [   11 12 13]
    /// [      22 23]
    /// [         33]
    /// \param dft1 The first discrete fourier transform
    /// \param dft2 The second discrete fourier transform
    /// \param output The generated cross spectral density matrix
    /// \param nSegments Number of segments
    /// \return True on success, false otherwise
    ///
    /// \todo : don't compute only the top part, because some connectivity measurements are not symmetrical ?
    bool crossSpectralDensity(const Eigen::MatrixXcd& dft1, const Eigen::MatrixXcd& dft2, Eigen::VectorXcd& output, const size_t& nSegments);

    // Parameters / Members
    EPsdMode m_psdMode = EPsdMode::Welch;
    EConnectMetric m_metric = EConnectMetric::Coherence;
    uint64_t m_sampRate = 500;
    size_t m_fftSize = 256;
    size_t m_nbChannels = 1;
    bool m_dcRemoval = false;
    Eigen::FFT<double, Eigen::internal::kissfft_impl<double>> m_fft; // Instance of the fft transform
    std::vector<Eigen::MatrixXcd> m_dft; // Discrete Fourier Transform
    std::vector<Eigen::VectorXd> m_psd; // Power Spectral Density
    std::vector<std::vector<Eigen::VectorXcd>> m_cpsd; // Cross Power Spectral Density

    // Welch specific...
    EConnectWindowMethod m_windowMethod = EConnectWindowMethod::Hann;
    Eigen::VectorXd m_window; // Window used for Welch method
    double m_u = 0; // Window normalization factor
    int m_windowLength = 128; // size of one Welch window (samples)
    int m_windowOverlap = 50; // overlap btw windows (%)
    int m_windowOverlapSamples = 64;

    // Burg Specific...
    int m_autoRegOrder = 11;
    Eigen::MatrixXcd m_expMat;
    std::vector<std::vector<std::vector<std::vector<Eigen::VectorXcd>>>> m_mvarCoeffsMatrix;

    // Thread Pool, for more multithreading
    ThreadPool m_threadPoolMax;

};

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
