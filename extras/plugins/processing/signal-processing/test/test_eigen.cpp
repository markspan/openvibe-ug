/*
 * \author Jussi T. Lindgren / Inria
 *
 * This is currently rather a simple benchmark than a test. It can be used to see how Eigen loads the cores,
 * simulating spatial filter (matrix multiplication use) with large matrices. Basically the core load should be
 * smaller during the stream simulation test than the burn test, unless 1) OpenMP/Eigen is busy-waiting the cores
 * 2) the matrix sizes really demand all the computational power available.
 *
 * The behavior may also depend on the matrix sizes.
 *
 * \date 24.02.2016
 */

#include <iostream>

#include <Eigen/Dense>

#if defined(TARGET_OS_Windows)
#include <Windows.h>
#endif

#include <system/ovCTime.h>
#include <openvibe/CTime.hpp>

typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> MatrixXdRowMajor;

int main(int /*argc*/, char* /*argv*/[])
{
    int returnValue = 0;

#if defined(TARGET_OS_Windows)
    // Set the clock precision to 1ms (default on Win7: 15ms)
    timeBeginPeriod(1);
#endif

    const size_t nChannelsIn    = 256;
    const size_t nChannelsOut   = 128;
    const size_t chunkSize      = 32;
    const uint64_t samplingFreq = 1000;

    Eigen::MatrixXd filterMatrix, dataMatrix;
    filterMatrix.resize(nChannelsOut, nChannelsIn);
    dataMatrix.resize(nChannelsIn, chunkSize);

    filterMatrix.setRandom();
    dataMatrix.setRandom();

#if EIGEN_MAJOR_VERSION >= 2 && EIGEN_MINOR_VERSION >= 8
    //Eigen::setNbThreads(1);
    //Eigen::initParallel();

    std::cout << "Eigen is using " << Eigen::nbThreads() << " threads\n";
#endif

    const double chunksPerSec    = samplingFreq / double(chunkSize);
    const uint64_t chunkDuration = OpenViBE::CTime(samplingFreq, chunkSize).time();

    std::cout << "Filter is " << nChannelsOut << "x" << nChannelsIn << ", data chunk is " << nChannelsIn << "x" << chunkSize << "\n";

    uint64_t before = System::Time::zgetTime();
    uint64_t after  = before;

    if (true) {
        uint64_t matricesProcessed = 0;
        const uint64_t timeOut     = 20;
        std::cout << "Running multiplication burn test\n";
        while (after - before < (timeOut << 32)) {
            Eigen::MatrixXd output = filterMatrix * dataMatrix; // Output useless but Time test
            matricesProcessed++;
            after = System::Time::zgetTime();
        }
        std::cout << "Managed to do " << matricesProcessed << " multiplications in " << timeOut << "s, " << double(matricesProcessed) / double(timeOut) <<
                " per sec.\n";
    }

    std::cout << "Running scheduler simulator\n";

    std::cout << "Using " << samplingFreq << "Hz sampling frequency, requires " << chunksPerSec << " chunks per sec\n";
    std::cout << "Time available for each chunk is " << OpenViBE::CTime(chunkDuration * 1000).toSeconds() << "ms.\n";

    uint64_t timeUsed          = 0;
    const uint64_t totalChunks = uint64_t(30 * chunksPerSec);

    for (size_t i = 0; i < totalChunks; ++i) {
        before                 = System::Time::zgetTime();
        Eigen::MatrixXd output = filterMatrix * dataMatrix; // Output useless but Time test
        after                  = System::Time::zgetTime();
        timeUsed += (after - before);

        // busy wait, this is the OV scheduler default
        while (after - before < chunkDuration) { after = System::Time::zgetTime(); }
    }

    std::cout << "Avg chunk process time was " << OpenViBE::CTime(timeUsed / totalChunks * 1000).toSeconds() << "ms.\n";

    if (timeUsed / totalChunks > chunkDuration) {
        std::cout << "Error: Cannot reach realtime with these parameters\n";
        returnValue = 1;
    }
    std::cout << "All tests completed\n";

#if defined(TARGET_OS_Windows)
    timeEndPeriod(1);
#endif

    return returnValue;
}
