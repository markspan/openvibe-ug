#include "ovpCHilbertTransform.h"
#include <complex>
#include <Eigen/Dense>
#include <unsupported/Eigen/FFT>

bool HilbertTransform::transform(const Eigen::VectorXcd& in, Eigen::VectorXcd& out)
{
    const Eigen::Index nSamples = Eigen::Index(in.size());

    // Resize our buffers if input size has changed
    if (m_signalFourier.size() != nSamples) {
        m_signalFourier = Eigen::VectorXcd::Zero(nSamples);
        m_hilbert       = Eigen::VectorXcd::Zero(nSamples);

        //Initialization of vector h used to compute analytic signal
        m_hilbert(0) = 1.0;

        if (nSamples % 2 == 0) {
            m_hilbert(nSamples / 2) = 1.0;
            m_hilbert.segment(1, (nSamples / 2) - 1).setOnes();
            m_hilbert.segment(1, (nSamples / 2) - 1) *= 2.0;
            m_hilbert.tail((nSamples / 2) + 1).setZero();
        }
        else {
            m_hilbert((nSamples + 1) / 2) = 1.0;
            m_hilbert.segment(1, (nSamples / 2)).setOnes();
            m_hilbert.segment(1, (nSamples / 2)) *= 2.0;
            m_hilbert.tail(((nSamples + 1) / 2) + 1).setZero();
        }
    }

    // Always resize output for safety
    out.resize(nSamples);

    //Fast Fourier Transform of input signal
    m_fft.fwd(m_signalFourier, in);

    //Apply Hilbert transform by element-wise multiplying fft vector by h
    m_signalFourier = m_signalFourier.cwiseProduct(m_hilbert);

    //Inverse Fast Fourier transform
    m_fft.inv(out, m_signalFourier); // m_vecXcdSignalBuffer is now the analytical signal of the initial input signal

    return true;
}

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CAlgorithmHilbertTransform::initialize()
{
    ip_matrix.initialize(this->getInputParameter(OVP_Algorithm_HilbertTransform_InputParameterId_Matrix));
    op_hilbertMatrix.initialize(this->getOutputParameter(OVP_Algorithm_HilbertTransform_OutputParameterId_HilbertMatrix));
    op_envelopeMatrix.initialize(this->getOutputParameter(OVP_Algorithm_HilbertTransform_OutputParameterId_EnvelopeMatrix));
    op_phaseMatrix.initialize(this->getOutputParameter(OVP_Algorithm_HilbertTransform_OutputParameterId_PhaseMatrix));

    return true;
}

bool CAlgorithmHilbertTransform::uninitialize()
{
    op_hilbertMatrix.uninitialize();
    op_envelopeMatrix.uninitialize();
    op_phaseMatrix.uninitialize();
    ip_matrix.uninitialize();

    return true;
}

bool CAlgorithmHilbertTransform::process()
{
    const size_t nChannel          = ip_matrix->getDimensionSize(0);
    const size_t samplesPerChannel = ip_matrix->getDimensionSize(1);

    CMatrix* matrix   = ip_matrix;
    CMatrix* hilbert  = op_hilbertMatrix;
    CMatrix* envelope = op_envelopeMatrix;
    CMatrix* phase    = op_phaseMatrix;

    if (this->isInputTriggerActive(OVP_Algorithm_HilbertTransform_InputTriggerId_Initialize)) //Check if the input is correct
    {
        if (matrix->getDimensionCount() != 2) {
            this->getLogManager() << Kernel::LogLevel_Error << "The input matrix must have 2 dimensions, here the dimension is " << matrix->getDimensionCount()
                    << "\n";
            return false;
        }

        if (matrix->getDimensionSize(1) < 2) {
            this->getLogManager() << Kernel::LogLevel_Error << "Can't compute Hilbert transform on data length " << matrix->getDimensionSize(1) << "\n";
            return false;
        }

        //Setting size of outputs

        hilbert->resize(nChannel, samplesPerChannel);
        envelope->resize(nChannel, samplesPerChannel);
        phase->resize(nChannel, samplesPerChannel);

        for (size_t i = 0; i < nChannel; ++i) {
            hilbert->setDimensionLabel(0, i, matrix->getDimensionLabel(0, i));
            envelope->setDimensionLabel(0, i, matrix->getDimensionLabel(0, i));
            phase->setDimensionLabel(0, i, matrix->getDimensionLabel(0, i));
        }
    }

    if (this->isInputTriggerActive(OVP_Algorithm_HilbertTransform_InputTriggerId_Process)) {
        //Compute Hilbert transform for each channel separately
        for (size_t c = 0; c < nChannel; ++c) {
            // We cannot do a simple ptr assignment here as we need to convert real input to a complex vector
            Eigen::VectorXcd vecXcdSingleChannel = Eigen::VectorXcd::Zero(Eigen::Index(samplesPerChannel));
            const double* buffer                 = &matrix->getBuffer()[c * samplesPerChannel];
            for (Eigen::Index samples = 0; samples < Eigen::Index(samplesPerChannel); ++samples) {
                vecXcdSingleChannel(samples) = buffer[samples];
                vecXcdSingleChannel(samples).imag(0.0);
            }

            Eigen::VectorXcd vecXcdSingleChannelTransformed;
            m_hilbert.transform(vecXcdSingleChannel, vecXcdSingleChannelTransformed);

            //Compute envelope and phase and pass them to the corresponding outputs
            for (Eigen::Index s = 0; s < Eigen::Index(samplesPerChannel); ++s) {
                hilbert->getBuffer()[s + c * samplesPerChannel]  = vecXcdSingleChannelTransformed(s).imag();
                envelope->getBuffer()[s + c * samplesPerChannel] = abs(vecXcdSingleChannelTransformed(s));
                phase->getBuffer()[s + c * samplesPerChannel]    = arg(vecXcdSingleChannelTransformed(s));
            }
        }
    }
    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
