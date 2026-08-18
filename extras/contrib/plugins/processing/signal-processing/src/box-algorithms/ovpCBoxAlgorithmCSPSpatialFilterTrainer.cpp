#if defined TARGET_HAS_ThirdPartyITPP

#include "ovpCBoxAlgorithmCSPSpatialFilterTrainer.h"

#include <complex>
#include <cstdio>
#include <map>
#include <math.h>
#include <iostream>

#include <itpp/base/algebra/eigen.h>
#include <itpp/base/algebra/inv.h>
#include <itpp/stat/misc_stat.h>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

// Taken from http://techlogbook.wordpress.com/2009/08/12/adding-generalized-eigenvalue-functions-to-it
//            http://techlogbook.wordpress.com/2009/08/12/calling-lapack-functions-from-c-codes
//            http://sourceforge.net/projects/itpp/forums/forum/115656/topic/3363490?message=7557038
//
// http://icl.cs.utk.edu/projectsfiles/f2j/javadoc/org/netlib/lapack/DSYGV.html
// http://www.lassp.cornell.edu/sethna/GeneDynamics/NetworkCodeDocumentation/lapack_8h.html#a17

namespace {
extern "C" {
// This symbol comes from LAPACK
/*
        void zggev_(char *jobvl, char *jobvr, int *n, std::complex<double> *a,
            int *lda, std::complex<double> *b, int *ldb, std::complex<double> *alpha,
            std::complex<double> *beta, std::complex<double> *vl,
            int *ldvl, std::complex<double> *vr, int *ldvr,
            std::complex<double> *work, int *lwork, double *rwork, int *info);
*/
int dsygv_(int* itype, char* jobz, char* uplo, int* n, double* a,
           int* lda, double* b, int* ldb, double* w, double* work, int* lwork, int* info);
}
}  // namespace

namespace itppextcsp {
itpp::mat convert(const CMatrix& matrix)
{
    itpp::mat res(matrix.getDimensionSize(1), matrix.getDimensionSize(0));
    if (matrix.getBufferElementCount() != 0) { memcpy(res._data(), matrix.getBuffer(), matrix.getBufferElementCount() * sizeof(double)); }
    return res.transpose();
}

itpp::mat cov(const itpp::mat& matrix)
{
    itpp::mat centered = repmat(sum(matrix, 2), 1, matrix.cols(), false);
    centered           = centered / double(matrix.cols());
    centered           = matrix - centered;
    itpp::mat res      = centered * centered.transpose();
    res                = res / double(matrix.cols() - 1);
    res                = res / trace(res);
    return res;
}
}  // namespace itppextcsp

bool CBoxAlgorithmCSPSpatialFilterTrainer::initialize()
{
    m_stimDecoder = new Toolkit::TStimulationDecoder<CBoxAlgorithmCSPSpatialFilterTrainer>();
    m_stimDecoder->initialize(*this, 0);

    m_signalDecoderCondition1 = new Toolkit::TSignalDecoder<CBoxAlgorithmCSPSpatialFilterTrainer>();
    m_signalDecoderCondition1->initialize(*this, 1);

    m_signalDecoderCondition2 = new Toolkit::TSignalDecoder<CBoxAlgorithmCSPSpatialFilterTrainer>();
    m_signalDecoderCondition2->initialize(*this, 2);

    m_encoder.initialize(*this, 0);

    m_stimID                      = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_spatialFilterConfigFilename = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    m_filterDimension             = uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2));
    m_saveAsBoxConfig             = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);

    return true;
}

bool CBoxAlgorithmCSPSpatialFilterTrainer::uninitialize()
{
    m_signalDecoderCondition1->uninitialize();
    delete m_signalDecoderCondition1;
    m_signalDecoderCondition2->uninitialize();
    delete m_signalDecoderCondition2;
    m_stimDecoder->uninitialize();
    delete m_stimDecoder;

    m_encoder.uninitialize();

    return true;
}

bool CBoxAlgorithmCSPSpatialFilterTrainer::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmCSPSpatialFilterTrainer::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    bool shouldTrain = false;
    uint64_t date    = 0, startTime = 0, endTime = 0;

    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i)
    {
        m_stimDecoder->decode(i);
        if (m_stimDecoder->isHeaderReceived())
        {
            m_encoder.encodeHeader();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }
        if (m_stimDecoder->isBufferReceived())
        {
            Kernel::TParameterHandler<CStimulationSet*> op_stimSet(m_stimDecoder->getOutputStimulationSet());
            for (size_t j = 0; j < op_stimSet->size(); ++j) { shouldTrain |= (op_stimSet->getId(j) == m_stimID); }
            if (shouldTrain)
            {
                date      = op_stimSet->getDate(op_stimSet->size() - 1);
                startTime = boxContext.getInputChunkStartTime(0, i);
                endTime   = boxContext.getInputChunkEndTime(0, i);
            }
        }
        if (m_stimDecoder->isEndReceived()) { m_encoder.encodeEnd(); }
        boxContext.markInputAsDeprecated(0, i);
    }

    if (shouldTrain)
    {
        this->getLogManager() << Kernel::LogLevel_Info << "Received train stimulation - be patient\n";

        this->getLogManager() << Kernel::LogLevel_Trace << "Estimating cov for condition 1...\n";

        itpp::mat covarianceMatrixCondition1;
        int nCondition1Trials   = 0;
        int condition1ChunkSize = 0;
        int condition2ChunkSize = 0;
        for (size_t i = 0; i < boxContext.getInputChunkCount(1); ++i)
        {
            m_signalDecoderCondition1->decode(i);
            if (m_signalDecoderCondition1->isHeaderReceived())
            {
                Kernel::TParameterHandler<CMatrix*> ip_matrix(m_signalDecoderCondition1->getOutputMatrix());
                covarianceMatrixCondition1.set_size(ip_matrix->getDimensionSize(0), ip_matrix->getDimensionSize(0));
                covarianceMatrixCondition1.zeros();

                condition1ChunkSize = ip_matrix->getDimensionSize(1);
                this->getLogManager() << Kernel::LogLevel_Debug << "Cov matrix size for condition 1 is [" << ip_matrix->getDimensionSize(0) << "x"
                        << ip_matrix->getDimensionSize(0) << "], chunk size is " << condition1ChunkSize << " samples\n";
            }
            if (m_signalDecoderCondition1->isBufferReceived())
            {
                Kernel::TParameterHandler<CMatrix*> ip_matrix(m_signalDecoderCondition1->getOutputMatrix());
                itpp::mat matrix = itppextcsp::convert(*ip_matrix);
                covarianceMatrixCondition1 += itppextcsp::cov(matrix);
                nCondition1Trials++;
            }
            if (m_signalDecoderCondition1->isEndReceived()) { }
            boxContext.markInputAsDeprecated(1, i);
        }
        covarianceMatrixCondition1 = covarianceMatrixCondition1 / double(nCondition1Trials);
        this->getLogManager() << Kernel::LogLevel_Trace << "Number of chunks for condition 1: " << nCondition1Trials << "\n";
        this->getLogManager() << Kernel::LogLevel_Trace << "Estimating cov for condition 2...\n";

        itpp::mat covarianceMatrixCondition2;
        int nCondition2Trials = 0;
        for (size_t i = 0; i < boxContext.getInputChunkCount(2); ++i)
        {
            m_signalDecoderCondition2->decode(i);
            if (m_signalDecoderCondition2->isHeaderReceived())
            {
                Kernel::TParameterHandler<CMatrix*> ip_matrix(m_signalDecoderCondition2->getOutputMatrix());
                covarianceMatrixCondition2.set_size(ip_matrix->getDimensionSize(0), ip_matrix->getDimensionSize(0));
                covarianceMatrixCondition2.zeros();

                condition2ChunkSize = ip_matrix->getDimensionSize(1);
                this->getLogManager() << Kernel::LogLevel_Debug << "Cov matrix size for condition 2 is [" << ip_matrix->getDimensionSize(0) << "x"
                        << ip_matrix->getDimensionSize(0) << "], chunk size is " << condition2ChunkSize << " samples\n";
            }
            if (m_signalDecoderCondition2->isBufferReceived())
            {
                Kernel::TParameterHandler<CMatrix*> ip_matrix(m_signalDecoderCondition2->getOutputMatrix());
                itpp::mat matrix = itppextcsp::convert(*ip_matrix);
                covarianceMatrixCondition2 += itppextcsp::cov(matrix);
                nCondition2Trials++;
            }
            if (m_signalDecoderCondition2->isEndReceived()) { }
            boxContext.markInputAsDeprecated(2, i);
        }
        covarianceMatrixCondition2 = covarianceMatrixCondition2 / double(nCondition2Trials);

        if (covarianceMatrixCondition1.cols() != covarianceMatrixCondition2.cols())
        {
            this->getLogManager() << Kernel::LogLevel_Error << "The two inputs do not seem to have the same number of channels, "
                    << covarianceMatrixCondition1.cols() << " vs " << covarianceMatrixCondition2.cols() << "\n";
            return false;
        }

        this->getLogManager() << Kernel::LogLevel_Info << "Data covariance dims are [" << covarianceMatrixCondition1.rows() << "x" << covarianceMatrixCondition1.cols()
                << "]. Number of samples per condition : \n";
        this->getLogManager() << Kernel::LogLevel_Info << "  cond1 = " << nCondition1Trials << " chunks, sized " << condition1ChunkSize << " -> "
                << nCondition1Trials * condition1ChunkSize << " samples\n";
        this->getLogManager() << Kernel::LogLevel_Info << "  cond2 = " << nCondition2Trials << " chunks, sized " << condition2ChunkSize << " -> "
                << nCondition2Trials * condition2ChunkSize << " samples\n";

        if (nCondition1Trials == 0 || nCondition2Trials == 0)
        {
            this->getLogManager() << Kernel::LogLevel_Error << "No signal received... Can't continue\n";
            return true;
        }

        this->getLogManager() << Kernel::LogLevel_Trace << "Computing eigen vector decomposition...\n";

        itpp::cmat eigenVector;
        itpp::cvec eigenValue;
        size_t nChannel = covarianceMatrixCondition1.rows();

        if (eig(inv(covarianceMatrixCondition2) * covarianceMatrixCondition1, eigenValue, eigenVector))
        {
            std::map<double, itpp::vec> vEigenVector;
            for (size_t i = 0; i < nChannel; ++i)
            {
                itpp::cvec v                            = eigenVector.get_col(i);
                vEigenVector[itpp::real(eigenValue)[i]] = itpp::real(v);
            }

            // Collect the output vectors here
            CMatrix outputVectors(m_filterDimension, nChannel);

            size_t steps = 0, cnt = 0;
            this->getLogManager() << Kernel::LogLevel_Debug << "lowest eigenvalues: " << "\n";
            for (auto it = vEigenVector.begin(); it != vEigenVector.end() && steps < ceil(m_filterDimension / 2.0); ++it, steps++)
            {
                this->getLogManager() << Kernel::LogLevel_Debug << it->first << ", ";
                for (size_t j = 0; j < nChannel; ++j) { outputVectors.getBuffer()[cnt++] = it->second[j]; }
            }
            this->getLogManager() << Kernel::LogLevel_Debug << "\n";
            this->getLogManager() << Kernel::LogLevel_Debug << "highest eigenvalues: " << "\n";
            steps = 0;
            for (auto it = vEigenVector.rbegin(); it != vEigenVector.rend() && steps < floor(m_filterDimension / 2.0); ++it, steps++)
            {
                this->getLogManager() << Kernel::LogLevel_Debug << it->first << ", ";
                for (size_t j = 0; j < nChannel; ++j) { outputVectors.getBuffer()[cnt++] = it->second[j]; }
            }
            this->getLogManager() << Kernel::LogLevel_Debug << "\n";

            if (m_saveAsBoxConfig)
            {
                FILE* file = fopen(m_spatialFilterConfigFilename.toASCIIString(), "wb");
                if (!file)
                {
                    this->getLogManager() << Kernel::LogLevel_Error << "The file [" << m_spatialFilterConfigFilename <<
                            "] could not be opened for writing...\n";
                    return false;
                }

                fprintf(file, "<OpenViBE-SettingsOverride>\n");
                fprintf(file, "\t<SettingValue>");

                cnt = 0;
                for (size_t i = 0; i < m_filterDimension; ++i)
                {
                    for (size_t j = 0; j < nChannel; ++j) { fprintf(file, "%e ", outputVectors.getBuffer()[cnt++]); }
                }

                fprintf(file, "</SettingValue>\n");
                fprintf(file, "\t<SettingValue>%zu</SettingValue>\n", m_filterDimension);
                fprintf(file, "\t<SettingValue>%zu</SettingValue>\n", nChannel);
                fprintf(file, "\t<SettingValue></SettingValue>\n");
                fprintf(file, "</OpenViBE-SettingsOverride>\n");
                fclose(file);
            }
            else
            {
                if (!Toolkit::Matrix::saveToTextFile(outputVectors, m_spatialFilterConfigFilename))
                {
                    this->getLogManager() << Kernel::LogLevel_Error << "Unable to save to [" << m_spatialFilterConfigFilename << "\n";
                    return false;
                }
            }
        }
        else
        {
            this->getLogManager() << Kernel::LogLevel_ImportantWarning << "Eigen vector decomposition failed...\n";
            return true;
        }

        this->getLogManager() << Kernel::LogLevel_Info << "CSP Spatial filter trained successfully.\n";

        m_encoder.getInputStimulationSet()->clear();
        m_encoder.getInputStimulationSet()->push_back(OVTK_StimulationId_TrainCompleted, date, 0);
        m_encoder.encodeBuffer();

        boxContext.markOutputAsReadyToSend(0, startTime, endTime);
    }

    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyITPP
