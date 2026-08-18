///-------------------------------------------------------------------------------------------------
///
/// \file spectrumTools.cpp
/// \brief Tools for spectrum computation - to be merged with connectivity measures
/// \author Arthur Desbois
/// \version 1.0
///
/// \copyright Copyright(C) 2021-2022 Inria
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
//#include <omp.h>

#include <Eigen/Dense>
#include <Eigen/Core>
#include <Eigen/LU>
#include <Eigen/StdVector>
#include <unsupported/Eigen/FFT>

#include <openvibe/ov_all.h>

#include "spectrumTools.hpp"


namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {


bool autoregressiveCoeffsAndPsd(const std::vector<Eigen::VectorXd>& samples, std::vector<Eigen::VectorXd>& arCoeffsOutput, std::vector<Eigen::VectorXd>& arPsd, const uint64_t order, const uint64_t psdSize, const bool detrend)
{
    const Eigen::Index nChannel          = samples.size();
    const Eigen::Index samplesPerChannel = samples[0].size();
    Eigen::FFT<double, Eigen::internal::kissfft_impl<double>> fft;
    double k = 0.0;

    // Compute the AR coefficients for each channel
    for (Eigen::Index chan = 0; chan < nChannel; ++chan) {
        // Initialization of all needed vectors
        Eigen::VectorXd errForwardPrediction  = Eigen::VectorXd::Zero(samplesPerChannel); // Error Forward prediction
        Eigen::VectorXd errBackwardPrediction = Eigen::VectorXd::Zero(samplesPerChannel); //Error Backward prediction

        Eigen::VectorXd errForward  = Eigen::VectorXd::Zero(samplesPerChannel); // Error Forward
        Eigen::VectorXd errBackward = Eigen::VectorXd::Zero(samplesPerChannel); // Error Backward

        Eigen::VectorXd arCoeffs = Eigen::VectorXd::Zero(order + 1); // Vector containing the AR coefficients for each channel, it will be our output vector
        Eigen::VectorXd errorVect = Eigen::VectorXd::Zero(order + 1); // Total error

        k = 0.0;
        arCoeffs(0) = 1.0;

        Eigen::VectorXd arReversed;
        arReversed = Eigen::VectorXd::Zero(order + 1);
        
        // Remove the DC component of each channel
        Eigen::VectorXd mySamples;
        if(detrend) {
            mySamples = samples[chan] - Eigen::VectorXd::Ones(samples[chan].size()) * samples[chan].mean();
        } else {
            mySamples = samples[chan];
        }

        errForward  = mySamples; // Error Forward is the input matrix at first
        errBackward = mySamples; //Error Backward is the input matrix at first
        errorVect(0) = mySamples.cwiseAbs2().sum() / samplesPerChannel;

        // iterate over the order
        for (Eigen::Index n = 1; n <= order; ++n) {
            const size_t length = samplesPerChannel - n;

            errForwardPrediction.resize(length);
            errBackwardPrediction.resize(length);

            errForwardPrediction  = errForward.tail(length);
            errBackwardPrediction = errBackward.head(length);

            const double num = -2.0 * errBackwardPrediction.dot(errForwardPrediction);
            const double den = (errForwardPrediction.dot(errForwardPrediction)) + (errBackwardPrediction.dot(errBackwardPrediction));

            k = num / den;

            // Update errors forward and backward vectors

            errForward  = errForwardPrediction + k * errBackwardPrediction;
            errBackward = errBackwardPrediction + k * errForwardPrediction;

            // Compute the AR coefficients
            for (Eigen::Index i = 1; i <= n; ++i) {
                arReversed(i) = arCoeffs(n-i);
            }
            arCoeffs = arCoeffs + k * arReversed;

            // Update Total Error
            errorVect(n) = (1 - k * k) * errorVect(n-1);
        }

        arCoeffsOutput[chan] = arCoeffs;

        // Compute the PSD from the AR coeffs
        Eigen::VectorXcd psdTemp = Eigen::RowVectorXcd::Zero(psdSize);
        Eigen::VectorXcd den = Eigen::RowVectorXcd::Zero(psdSize);
        for(Eigen::Index i = 0; i < arCoeffs.size(); i++) {
            den(i) = arCoeffs(i);
        }

        fft.fwd(psdTemp, den, psdSize);
        arPsd[chan] = psdTemp.cwiseAbs2().cwiseInverse();

        if(0) { // DEBUG LOGS
            std::cout << "-----AR PRE FFT : ";
            for(size_t i = 0; i < psdSize; i++) {
                std::cout << den(i) << " ";
            } std::cout << std::endl;

            std::cout << "-----AR POST FFT : ";
            for(size_t i = 0; i < psdSize; i++) {
                std::cout << psdTemp(i) << " ";
            } std::cout << std::endl;

            std::cout << "-----PSD : ";
            for(size_t i = 0; i < psdSize; i++) {
                std::cout << arPsd[chan](i) << " ";
            } std::cout << std::endl << std::endl;
        }

    }

    return true;
}

Eigen::MatrixXcd generateExpMat(const uint64_t order, const uint64_t psdSize, const uint64_t sampRate)
{
    double factor =  double(sampRate) /(double(psdSize));
    std::complex<double> z(0.0, 2*M_PI/ double(sampRate) );
    std::complex<double> complexFreq = std::complex<double>(0.0, 0.0);

    Eigen::MatrixXcd expMat = Eigen::MatrixXcd::Zero(psdSize, order+1); // psd x (order+1)
    for(size_t i = 0; i < psdSize-1; i++) {
        complexFreq = i * factor * z;
        for (size_t k = 0; k < order+1; k++) {
            expMat(i,k) = std::exp(std::complex<double>(k, 0.0) * complexFreq);
        }
    }
    return expMat;
}

bool mvarSpectralEstimation(const Eigen::VectorXd& samples1, const Eigen::VectorXd& samples2, std::vector<std::vector<Eigen::VectorXcd>>& mvarMatrix, 
    const uint64_t order, const uint64_t psdSize, const uint64_t sampRate, const bool detrend, const Eigen::MatrixXcd& expMat)
{
    Eigen::Matrix2cd PEf = Eigen::MatrixXcd::Zero(2, 2);;   // Error Forward matrix
    Eigen::Matrix2cd PEb = Eigen::MatrixXcd::Zero(2, 2);;   // Error Backward matrix
    //std::vector<Eigen::Matrix2cd> PE; // Residual error variance
    std::vector<Eigen::Matrix2cd, Eigen::aligned_allocator<Eigen::Matrix2cd>> ARf;  // AutoRegressive Coefficents forward
    std::vector<Eigen::Matrix2cd, Eigen::aligned_allocator<Eigen::Matrix2cd>> ARb;  // AutoRegressive Coefficents backward
    std::vector<Eigen::Matrix2cd, Eigen::aligned_allocator<Eigen::Matrix2cd>> C;     // covariance matrix
    ARb.resize(order);
    ARf.resize(order);
    C.resize(order+1);
    
    const Eigen::Index M = 2; // M = nb channels
    const Eigen::Index N = samples1.size(); // N = data length

    // TODO : only 2 channels for now?
    if(M != 2) {
        std::cout << "Nb of Channels in MVAR algo must be 2 (here " << M << ")" << std::endl;
        return false;
    }

    // Putting the signals vectors in a matrix...
    Eigen::MatrixXd signalsMat(M, N);
    if(detrend) {
        signalsMat.row(0) = samples1 - Eigen::VectorXd::Ones(N) * samples1.mean();
        signalsMat.row(1) = samples2 - Eigen::VectorXd::Ones(N) * samples2.mean();
    } else {
        signalsMat.row(0) = samples1;
        signalsMat.row(1) = samples2;
    }

    // To save time, we compute the adjoint matrix once and for all
    Eigen::MatrixXd signalsMatAdjoint = signalsMat.adjoint();

    // Compute covariance matrix... 
    Eigen::Matrix2cd cov = (signalsMat * signalsMatAdjoint) / double(N);

    // Setting PE(0) as cov(X,Y), normalize by N
    C[0] = cov;
    //PE.push_back(cov / N); // useless

    PEf = cov;
    PEb = cov;

    Eigen::Matrix2d I = Eigen::Matrix2d::Identity();
    Eigen::Matrix2cd tempARf;

    for (Eigen::Index K = 1; K < order + 1; ++K) {

        cov = (signalsMat.block(0, K, M, N-K) * signalsMatAdjoint.block(0, 0, N-K, M)) / double(N-K); // D in Matlab code

        C[K] = cov;
        for (Eigen::Index L = 1; L < K; ++L) {
            cov -= ARf[L-1] * C[K-L]; // still D
        }
        // AR coeffs
        ARf[K-1] = cov * PEb.inverse();
        ARb[K-1] = cov.adjoint() * PEf.inverse();

        for (Eigen::Index L = 1; L < K; ++L) {
            tempARf = ARf[L-1]-ARf[K-1] * ARb[K-L-1];
            ARb[K-L-1] = ARb[K-L-1]-ARb[K-1] * ARf[L-1];
            ARf[L-1] = tempARf;
        }
        PEf = (I-ARf[K-1] * ARb[K-1])*PEf;
        PEb = (I-ARb[K-1] * ARf[K-1])*PEb;

        //PE.push_back(PEf); // useless
    }

    // Compute S matrix from AR coeffs
//#pragma omp parallel for
    for (int n = 0; n < psdSize; ++n) {
        Eigen::Matrix2cd atmp = I;
        for (int k = 1; k < order + 1; ++k) {
            atmp -= ARf[k-1] * expMat(n, k);
        }

        Eigen::Matrix2cd h = atmp.inverse();
        Eigen::Matrix2cd S = h*h.adjoint();

        mvarMatrix[0][0](n) = S(0, 0);
        mvarMatrix[1][0](n) = S(1, 0);
        mvarMatrix[0][1](n) = S(0, 1);
        mvarMatrix[1][1](n) = S(1, 1);
    }

    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE

