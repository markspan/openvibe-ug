#include "ovpCAlgorithmARBurgMethod.h"

#include <Eigen/Dense>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CAlgorithmARBurgMethod::initialize()
{
    ip_pMatrix.initialize(this->getInputParameter(OVP_Algorithm_ARBurgMethod_InputParameterId_Matrix));
    op_pMatrix.initialize(this->getOutputParameter(OVP_Algorithm_ARBurgMethod_OutputParameterId_Matrix));
    ip_Order.initialize(this->getInputParameter(OVP_Algorithm_ARBurgMethod_InputParameterId_UInteger));

    return true;
}

bool CAlgorithmARBurgMethod::uninitialize()
{
    op_pMatrix.uninitialize();
    ip_pMatrix.uninitialize();
    ip_Order.uninitialize();

    return true;
}

bool CAlgorithmARBurgMethod::process()
{
    m_order = size_t(ip_Order);

    const Eigen::Index nChannel          = Eigen::Index(ip_pMatrix->getDimensionSize(0));
    const Eigen::Index samplesPerChannel = Eigen::Index(ip_pMatrix->getDimensionSize(1));

    CMatrix* iMatrix      = ip_pMatrix;
    CMatrix* oMatrix      = op_pMatrix;
    const double* iBuffer = iMatrix->getBuffer();

    if (this->isInputTriggerActive(OVP_Algorithm_ARBurgMethod_InputTriggerId_Initialize)) {
        if (iMatrix->getDimensionCount() != 2) {
            this->getLogManager() << Kernel::LogLevel_Error << "The input matrix must have 2 dimensions";
            return false;
        }

        if (iMatrix->getDimensionSize(1) < 2 * m_order) {
            this->getLogManager() << Kernel::LogLevel_Error << "The input vector must be greater than twice the order";
            return false;
        }

        // Setting size of output

        oMatrix->resize(nChannel, m_order + 1); // The number of coefficients per channel is equal to the order + 1

        for (size_t i = 0; i < nChannel; ++i) {
            const std::string label = "Channel " + std::to_string(i + 1);
            oMatrix->setDimensionLabel(0, i, label);
        }
        for (size_t i = 0; i < (m_order + 1); ++i) {
            const std::string label = "ARCoeff " + std::to_string(i + 1);
            oMatrix->setDimensionLabel(1, i, label);
        }
    }


    if (this->isInputTriggerActive(OVP_Algorithm_ARBurgMethod_InputTriggerId_Process)) {
        // Compute the coefficients for each channel
        for (Eigen::Index j = 0; j < nChannel; ++j) {
            // Initialization of all needed vectors 

            m_errForwardPrediction  = Eigen::RowVectorXd::Zero(samplesPerChannel); // Error Forward prediction
            m_errBackwardPrediction = Eigen::RowVectorXd::Zero(samplesPerChannel); //Error Backward prediction

            m_errForward  = Eigen::RowVectorXd::Zero(samplesPerChannel); // Error Forward 
            m_errBackward = Eigen::RowVectorXd::Zero(samplesPerChannel); // Error Backward

            m_arCoefs =
                    Eigen::RowVectorXd::Zero(Eigen::Index(m_order + 1)); // Vector containing the AR coefficients for each channel, it will be our output vector
            m_error = Eigen::RowVectorXd::Zero(Eigen::Index(m_order + 1)); // Total error 

            m_k          = 0.0;
            m_arCoefs(0) = 1.0;

            Eigen::VectorXd arReversed;
            arReversed = Eigen::VectorXd::Zero(Eigen::Index(m_order + 1));

            // Retrieving input datas
            for (Eigen::Index i = 0; i < samplesPerChannel; ++i) {
                m_errForward(i)  = iBuffer[i + j * (samplesPerChannel)]; // Error Forward is the input matrix at first
                m_errBackward(i) = iBuffer[i + j * (samplesPerChannel)]; //Error Backward is the input matrix at first

                m_error(0) += (iBuffer[i + j * (samplesPerChannel)] * iBuffer[i + j * (samplesPerChannel)]) / double(samplesPerChannel);
            }

            // we iterate over the order
            for (Eigen::Index n = 1; n <= Eigen::Index(m_order); ++n) {
                const Eigen::Index length = samplesPerChannel - n;

                m_errForwardPrediction.resize(length);
                m_errBackwardPrediction.resize(length);

                m_errForwardPrediction  = m_errForward.tail(length);
                m_errBackwardPrediction = m_errBackward.head(length);

                const double num = -2.0 * m_errBackwardPrediction.dot(m_errForwardPrediction);
                const double den = (m_errForwardPrediction.dot(m_errForwardPrediction)) + (m_errBackwardPrediction.dot(m_errBackwardPrediction));

                m_k = num / den;

                // Update errors forward and backward vectors

                m_errForward  = m_errForwardPrediction + m_k * m_errBackwardPrediction;
                m_errBackward = m_errBackwardPrediction + m_k * m_errForwardPrediction;

                // Compute the AR coefficients

                for (Eigen::Index i = 1; i <= n; ++i) { arReversed(i) = m_arCoefs(n - i); }

                m_arCoefs = m_arCoefs + m_k * arReversed;

                // Update Total Error
                m_error(n) = (1 - m_k * m_k) * m_error(n - 1);
            }
            for (Eigen::Index i = 0; i <= Eigen::Index(m_order); ++i) { oMatrix->getBuffer()[i + j * (m_order + 1)] = m_arCoefs(i); }
        }
        this->activateOutputTrigger(OVP_Algorithm_ARBurgMethod_OutputTriggerId_ProcessDone, true);
    }
    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
