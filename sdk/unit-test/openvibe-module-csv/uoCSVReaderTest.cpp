///-------------------------------------------------------------------------------------------------
/// 
/// \file uoCSVReaderTest.cpp
/// \copyright Copyright (C) 2022 Inria
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
/// 
///-------------------------------------------------------------------------------------------------

#include <tuple>
#include <numeric>

#include "gtest/gtest.h"

#include "csv/ovICSV.h"

struct SSignalFile
{
    std::vector<std::string> m_ChannelNames;
    size_t m_Sampling;
    size_t m_nSamplePerBuffer;
    std::vector<std::pair<std::pair<double, double>, std::vector<double>>> m_data;
};

namespace {
std::string dataDirectory = "";

const struct SSignalFile SIMPLE_SIGNAL_FILE = {
    { "Time Signal" }, 32, 8,
    {
        { { 0.00000, 0.25000 }, { 0.00000, 0.03125, 0.06250, 0.09375, 0.12500, 0.15625, 0.18750, 0.21875 } },
        { { 0.25000, 0.50000 }, { 0.25000, 0.28125, 0.31250, 0.34375, 0.37500, 0.40625, 0.43750, 0.46875 } },
        { { 0.50000, 0.75000 }, { 0.50000, 0.53125, 0.56250, 0.59375, 0.62500, 0.65625, 0.68750, 0.71875 } },
    }
};

void compareChunks(const std::pair<std::pair<double, double>, std::vector<double>>& expected, const OpenViBE::CSV::SMatrixChunk& actual)
{
    ASSERT_EQ(expected.first.first, actual.startTime);
    ASSERT_EQ(expected.first.second, actual.endTime);
    ASSERT_EQ(expected.second.size(), actual.matrix.size());
    for (size_t sample = 0; sample < expected.second.size(); ++sample) { ASSERT_EQ(expected.second[sample], actual.matrix[sample]); }
}
}  // namespace

TEST(CSV_Reader_Test_Case, signalReaderUNIXEndlines)
{
    OpenViBE::CSV::ICSVHandler* csv = OpenViBE::CSV::createCSVHandler();
    const std::string filepath      = dataDirectory + "/testCSVSignalUNIXEndlines.csv";

    ASSERT_TRUE(csv->openFile(filepath, OpenViBE::CSV::EFileAccessMode::Read));
    std::vector<std::string> channelNames;
    size_t sampling;
    size_t nSamplePerBuffer;
    std::vector<OpenViBE::CSV::SMatrixChunk> chunks;
    std::vector<OpenViBE::CSV::SStimulationChunk> stimulations;

    ASSERT_TRUE(csv->parseHeader());
    ASSERT_EQ(csv->getFormatType(), OpenViBE::CSV::EStreamType::Signal);
    ASSERT_TRUE(csv->getSignalInformation(channelNames, sampling, nSamplePerBuffer));
    ASSERT_TRUE(csv->readSamplesAndEventsFromFile(1, chunks, stimulations));
    ASSERT_EQ(1, chunks.size());
    compareChunks(SIMPLE_SIGNAL_FILE.m_data[0], chunks[0]);
    ASSERT_TRUE(csv->readSamplesAndEventsFromFile(2, chunks, stimulations));
    ASSERT_EQ(2, chunks.size());
    compareChunks(SIMPLE_SIGNAL_FILE.m_data[1], chunks[0]);
    compareChunks(SIMPLE_SIGNAL_FILE.m_data[2], chunks[1]);

    ASSERT_TRUE(csv->closeFile());
    releaseCSVHandler(csv);
}

TEST(CSV_Reader_Test_Case, signalReaderWindowsEndlines)
{
    OpenViBE::CSV::ICSVHandler* csv = OpenViBE::CSV::createCSVHandler();
    const std::string filepath      = dataDirectory + "/testCSVSignalWindowsEndlines.csv";

    ASSERT_TRUE(csv->openFile(filepath, OpenViBE::CSV::EFileAccessMode::Read));
    std::vector<std::string> channelNames;
    size_t sampling;
    size_t nSamplePerBuffer;
    std::vector<OpenViBE::CSV::SMatrixChunk> chunks;
    std::vector<OpenViBE::CSV::SStimulationChunk> stimulations;

    ASSERT_TRUE(csv->parseHeader());
    ASSERT_EQ(csv->getFormatType(), OpenViBE::CSV::EStreamType::Signal);
    ASSERT_TRUE(csv->getSignalInformation(channelNames, sampling, nSamplePerBuffer));
    ASSERT_TRUE(csv->readSamplesAndEventsFromFile(1, chunks, stimulations));
    ASSERT_EQ(1, chunks.size());
    compareChunks(SIMPLE_SIGNAL_FILE.m_data[0], chunks[0]);
    ASSERT_TRUE(csv->readSamplesAndEventsFromFile(2, chunks, stimulations));
    ASSERT_EQ(2, chunks.size());
    compareChunks(SIMPLE_SIGNAL_FILE.m_data[1], chunks[0]);
    compareChunks(SIMPLE_SIGNAL_FILE.m_data[2], chunks[1]);

    ASSERT_TRUE(csv->closeFile());
    releaseCSVHandler(csv);
}

TEST(CSV_Reader_Test_Case, signalReaderNormalGoodSignal)
{
    OpenViBE::CSV::ICSVHandler* signalReaderTest = OpenViBE::CSV::createCSVHandler();
    const std::string filepath                   = dataDirectory + "testCSVSignalReader01.csv";

    ASSERT_TRUE(signalReaderTest->openFile(filepath, OpenViBE::CSV::EFileAccessMode::Read));

    std::vector<OpenViBE::CSV::SMatrixChunk> chunks;
    std::vector<OpenViBE::CSV::SStimulationChunk> stimulations;
    std::vector<std::string> channelNames;
    const std::vector<std::string> expectedChannels = { "O1", "O2", "Pz", "P1", "P2" };
    size_t sampling;
    size_t nSamplePerBuffer;

    ASSERT_TRUE(signalReaderTest->parseHeader());
    ASSERT_EQ(signalReaderTest->getFormatType(), OpenViBE::CSV::EStreamType::Signal);
    ASSERT_TRUE(signalReaderTest->getSignalInformation(channelNames, sampling, nSamplePerBuffer));
    ASSERT_TRUE(signalReaderTest->readSamplesAndEventsFromFile(3, chunks, stimulations));
    ASSERT_EQ(chunks.size(), 3);

    ASSERT_TRUE(!chunks.empty());
    ASSERT_TRUE(!stimulations.empty());
    ASSERT_EQ(channelNames, expectedChannels);
    ASSERT_EQ(sampling, 8U);
    ASSERT_EQ(nSamplePerBuffer, 4);
    ASSERT_TRUE(signalReaderTest->closeFile());
    releaseCSVHandler(signalReaderTest);
}

TEST(CSV_Reader_Test_Case, signalReaderNotEnoughChunk)
{
    OpenViBE::CSV::ICSVHandler* signalReaderTest = OpenViBE::CSV::createCSVHandler();
    const std::string filepath                   = dataDirectory + "testCSVSignalReader01.csv";

    ASSERT_TRUE(signalReaderTest->openFile(filepath, OpenViBE::CSV::EFileAccessMode::Read));

    std::vector<OpenViBE::CSV::SMatrixChunk> chunks;
    std::vector<OpenViBE::CSV::SStimulationChunk> stimulations;
    std::vector<std::string> channelNames;
    const std::vector<std::string> expectedChannels = { "O1", "O2", "Pz", "P1", "P2" };
    size_t sampling;
    size_t nSamplePerBuffer;

    ASSERT_TRUE(signalReaderTest->parseHeader());
    ASSERT_EQ(signalReaderTest->getFormatType(), OpenViBE::CSV::EStreamType::Signal);
    ASSERT_TRUE(signalReaderTest->getSignalInformation(channelNames, sampling, nSamplePerBuffer));

    ASSERT_TRUE(signalReaderTest->readSamplesAndEventsFromFile(3, chunks, stimulations));
    ASSERT_EQ(chunks.size(), size_t(3));
    ASSERT_EQ(stimulations.size(), size_t(3));
    ASSERT_EQ(channelNames, expectedChannels);
    ASSERT_EQ(sampling, 8U);
    ASSERT_EQ(nSamplePerBuffer, 4);
    ASSERT_TRUE(signalReaderTest->closeFile());
    releaseCSVHandler(signalReaderTest);
}

TEST(CSV_Reader_Test_Case, SignalReaderEmptyFile)
{
    OpenViBE::CSV::ICSVHandler* signalReaderTest = OpenViBE::CSV::createCSVHandler();
    const std::string filepath                   = dataDirectory + "testCSVSignalEmptyFile.csv";
    ASSERT_TRUE(signalReaderTest->openFile(filepath, OpenViBE::CSV::EFileAccessMode::Read));
    releaseCSVHandler(signalReaderTest);
}

TEST(CSV_Reader_Test_Case, SignalReaderWrongHeader)
{
    OpenViBE::CSV::ICSVHandler* signalReaderTest = OpenViBE::CSV::createCSVHandler();
    const std::string filepath                   = dataDirectory + "testCSVSignalWrongHeader.csv";
    ASSERT_TRUE(signalReaderTest->openFile(filepath, OpenViBE::CSV::EFileAccessMode::Read));
    std::vector<std::string> channelNames;
    size_t sampling;
    size_t nSamplePerBuffer;

    ASSERT_FALSE(signalReaderTest->parseHeader());
    ASSERT_EQ(signalReaderTest->getFormatType(), OpenViBE::CSV::EStreamType::Undefined);
    ASSERT_FALSE(signalReaderTest->getSignalInformation(channelNames, sampling, nSamplePerBuffer));

    ASSERT_TRUE(signalReaderTest->closeFile());
    releaseCSVHandler(signalReaderTest);
}

TEST(CSV_Reader_Test_Case, spectrumReaderNormalGoodSignal)
{
    OpenViBE::CSV::ICSVHandler* spectrumReaderTest = OpenViBE::CSV::createCSVHandler();
    std::string filepath                           = dataDirectory + "testCSVSpectrumReader01.csv";
    ASSERT_TRUE(spectrumReaderTest->openFile(filepath, OpenViBE::CSV::EFileAccessMode::Read));
    std::vector<OpenViBE::CSV::SMatrixChunk> chunks;
    std::vector<OpenViBE::CSV::SStimulationChunk> stimulations;
    std::vector<std::string> channelNames;
    std::vector<std::string> expectedChannels = { "O1", "O2" };
    std::vector<double> expectedData(128);
    std::iota(expectedData.begin(), expectedData.begin() + 64, 0);
    std::iota(expectedData.begin() + 64, expectedData.end(), 0);

    std::vector<double> frequencyAbscissa;
    size_t originalSampling;

    ASSERT_TRUE(spectrumReaderTest->parseHeader());
    ASSERT_EQ(spectrumReaderTest->getFormatType(), OpenViBE::CSV::EStreamType::Spectrum);
    ASSERT_TRUE(spectrumReaderTest->getSpectrumInformation(channelNames, frequencyAbscissa, originalSampling));

    ASSERT_TRUE(spectrumReaderTest->readSamplesAndEventsFromFile(3, chunks, stimulations));
    ASSERT_EQ(chunks.size(), 3);
    ASSERT_EQ(chunks[0].matrix, expectedData);
    ASSERT_EQ(chunks[1].matrix, expectedData);
    ASSERT_EQ(chunks[2].matrix, expectedData);
    ASSERT_EQ(chunks[0].startTime, 0);
    ASSERT_EQ(chunks[0].endTime, 1);
    ASSERT_EQ(chunks[1].startTime, 0.125);
    ASSERT_EQ(chunks[1].endTime, 1.125);
    ASSERT_EQ(chunks[2].startTime, 0.25);
    ASSERT_EQ(chunks[2].endTime, 1.25);

    ASSERT_TRUE(!chunks.empty());
    ASSERT_EQ(channelNames, expectedChannels);
    ASSERT_EQ(originalSampling, 128);
    ASSERT_TRUE(spectrumReaderTest->closeFile());
    releaseCSVHandler(spectrumReaderTest);
}

TEST(CSV_Reader_Test_Case, spectrumReaderNotEnoughChunk)
{
    OpenViBE::CSV::ICSVHandler* spectrumReaderTest = OpenViBE::CSV::createCSVHandler();
    const std::string filepath                     = dataDirectory + "testCSVSpectrumReader01.csv";
    ASSERT_TRUE(spectrumReaderTest->openFile(filepath, OpenViBE::CSV::EFileAccessMode::Read));

    std::vector<OpenViBE::CSV::SMatrixChunk> chunks;
    std::vector<OpenViBE::CSV::SStimulationChunk> stimulations;
    std::vector<std::string> channelNames;
    const std::vector<std::string> expectedChannels = { "O1", "O2" };
    std::vector<double> frequencyAbscissa;
    size_t originalSampling;

    ASSERT_TRUE(spectrumReaderTest->parseHeader());
    ASSERT_EQ(spectrumReaderTest->getFormatType(), OpenViBE::CSV::EStreamType::Spectrum);
    ASSERT_TRUE(spectrumReaderTest->getSpectrumInformation(channelNames, frequencyAbscissa, originalSampling));

    ASSERT_TRUE(spectrumReaderTest->readSamplesAndEventsFromFile(13, chunks, stimulations));
    ASSERT_NE(chunks.size(), 13);

    ASSERT_TRUE(!chunks.empty());
    ASSERT_EQ(channelNames, expectedChannels);
    ASSERT_EQ(originalSampling, 128);
    ASSERT_NE(4, chunks.size());
    ASSERT_TRUE(spectrumReaderTest->closeFile());
    releaseCSVHandler(spectrumReaderTest);
}

TEST(CSV_Reader_Test_Case, spectrumReaderWrongHeader)
{
    OpenViBE::CSV::ICSVHandler* spectrumReaderTest = OpenViBE::CSV::createCSVHandler();
    const std::string filepath                     = dataDirectory + "testCSVSpectrumWrongHeader.csv";
    ASSERT_TRUE(spectrumReaderTest->openFile(filepath, OpenViBE::CSV::EFileAccessMode::Read));

    std::vector<std::string> channelNames;
    std::vector<double> frequencyAbscissa;
    size_t originalSampling;

    ASSERT_FALSE(spectrumReaderTest->parseHeader());
    ASSERT_NE(spectrumReaderTest->getFormatType(), OpenViBE::CSV::EStreamType::Spectrum);
    ASSERT_FALSE(spectrumReaderTest->getSpectrumInformation(channelNames, frequencyAbscissa, originalSampling));

    ASSERT_TRUE(spectrumReaderTest->closeFile());
    releaseCSVHandler(spectrumReaderTest);
}

TEST(CSV_Reader_Test_Case, matrixReaderNormalGoodSignal)
{
    OpenViBE::CSV::ICSVHandler* matrixReaderTest = OpenViBE::CSV::createCSVHandler();
    const std::string filepath                   = dataDirectory + "testCSVMatrixReader01.csv";
    ASSERT_TRUE(matrixReaderTest->openFile(filepath, OpenViBE::CSV::EFileAccessMode::Read));

    std::vector<OpenViBE::CSV::SMatrixChunk> chunks;
    std::vector<OpenViBE::CSV::SStimulationChunk> stimulations;
    std::vector<size_t> dimensionSizes;
    std::vector<std::string> labels;
    const std::vector<std::string> expectedLabels = { "", "", "", "", "", "" };
    const std::vector<size_t> goodDimensionsSizes = { 2, 2, 2 };

    ASSERT_TRUE(matrixReaderTest->parseHeader());
    ASSERT_EQ(matrixReaderTest->getFormatType(), OpenViBE::CSV::EStreamType::StreamedMatrix);
    ASSERT_TRUE(matrixReaderTest->getStreamedMatrixInformation(dimensionSizes, labels));
    ASSERT_TRUE(matrixReaderTest->readSamplesAndEventsFromFile(10, chunks, stimulations));
    ASSERT_EQ(chunks.size(), 10);

    ASSERT_EQ(dimensionSizes, goodDimensionsSizes);
    ASSERT_EQ(labels, expectedLabels);

    ASSERT_TRUE(matrixReaderTest->closeFile());
    releaseCSVHandler(matrixReaderTest);
}


TEST(CSV_Reader_Test_Case, matrixReaderWrongHeader)
{
    OpenViBE::CSV::ICSVHandler* matrixReaderTest = OpenViBE::CSV::createCSVHandler();
    const std::string filepath                   = dataDirectory + "testCSVMatrixWrongHeader.csv";
    ASSERT_TRUE(matrixReaderTest->openFile(filepath, OpenViBE::CSV::EFileAccessMode::Read));

    std::vector<size_t> dimensionSizes;
    std::vector<std::string> labels;

    ASSERT_FALSE(matrixReaderTest->parseHeader());
    ASSERT_EQ(matrixReaderTest->getFormatType(), OpenViBE::CSV::EStreamType::Undefined);

    ASSERT_TRUE(matrixReaderTest->closeFile());
    releaseCSVHandler(matrixReaderTest);
}

TEST(CSV_Reader_Test_Case, matrixReaderTooManyLabels)
{
    OpenViBE::CSV::ICSVHandler* matrixReaderTest = OpenViBE::CSV::createCSVHandler();
    const std::string filepath                   = dataDirectory + "testCSVMatrixTooManyLabels.csv";
    ASSERT_TRUE(matrixReaderTest->openFile(filepath, OpenViBE::CSV::EFileAccessMode::Read));
    matrixReaderTest->setFormatType(OpenViBE::CSV::EStreamType::StreamedMatrix);

    std::vector<size_t> dimensionSizes;
    std::vector<std::string> labels;

    ASSERT_FALSE(matrixReaderTest->parseHeader());
    ASSERT_EQ(matrixReaderTest->getFormatType(), OpenViBE::CSV::EStreamType::Undefined);

    ASSERT_TRUE(matrixReaderTest->closeFile());
    releaseCSVHandler(matrixReaderTest);
}

TEST(CSV_Reader_Test_Case, matrixReaderWrongStimulation)
{
    OpenViBE::CSV::ICSVHandler* matrixReaderTest = OpenViBE::CSV::createCSVHandler();
    const std::string filepath                   = dataDirectory + "testCSVMatrixWrongStimulation.csv";
    ASSERT_TRUE(matrixReaderTest->openFile(filepath, OpenViBE::CSV::EFileAccessMode::Read));
    matrixReaderTest->setFormatType(OpenViBE::CSV::EStreamType::StreamedMatrix);

    std::vector<size_t> dimensionSizes;
    std::vector<std::string> labels;

    ASSERT_TRUE(matrixReaderTest->parseHeader());
    ASSERT_EQ(matrixReaderTest->getFormatType(), OpenViBE::CSV::EStreamType::StreamedMatrix);
    ASSERT_TRUE(matrixReaderTest->getStreamedMatrixInformation(dimensionSizes, labels));

    std::vector<OpenViBE::CSV::SMatrixChunk> chunks;
    std::vector<OpenViBE::CSV::SStimulationChunk> stimulations;

    ASSERT_FALSE(matrixReaderTest->readSamplesAndEventsFromFile(1, chunks, stimulations));

    ASSERT_TRUE(matrixReaderTest->closeFile());
    releaseCSVHandler(matrixReaderTest);
}


TEST(CSV_Reader_Test_Case, covarianceMatrixReaderNormalGoodSignal)
{
    OpenViBE::CSV::ICSVHandler* matrixReaderTest = OpenViBE::CSV::createCSVHandler();
    const std::string filepath                   = dataDirectory + "testCSVCovarMatrixReader01.csv";
    ASSERT_TRUE(matrixReaderTest->openFile(filepath, OpenViBE::CSV::EFileAccessMode::Read));

    std::vector<OpenViBE::CSV::SMatrixChunk> chunks;
    std::vector<OpenViBE::CSV::SStimulationChunk> stimulations;
    std::vector<size_t> dimensionSizes;
    std::vector<std::string> labels;
    const std::vector<std::string> expectedLabels = { "X", "Y", "X", "Y", "Z1", "Z2", "Z3", "Z4", "Z5" };
    const std::vector<size_t> goodDimensionsSizes = { 2, 2, 5 };


    ASSERT_TRUE(matrixReaderTest->parseHeader());
    ASSERT_EQ(matrixReaderTest->getFormatType(), OpenViBE::CSV::EStreamType::StreamedMatrix);
    ASSERT_TRUE(matrixReaderTest->getStreamedMatrixInformation(dimensionSizes, labels));
    ASSERT_TRUE(matrixReaderTest->readSamplesAndEventsFromFile(3, chunks, stimulations)) << matrixReaderTest->getLastLogError() << ".Details: " <<
 matrixReaderTest->getLastErrorString();
    ASSERT_EQ(chunks.size(), 3);

    ASSERT_EQ(dimensionSizes, goodDimensionsSizes);
    ASSERT_EQ(labels, expectedLabels);

    ASSERT_TRUE(matrixReaderTest->closeFile());
    releaseCSVHandler(matrixReaderTest);
}

TEST(CSV_Reader_Test_Case, covarianceMatrixReaderWrongHeader)
{
    OpenViBE::CSV::ICSVHandler* matrixReaderTest = OpenViBE::CSV::createCSVHandler();
    const std::string filepath                   = dataDirectory + "testCSVCovarMatrixWrongHeader.csv";
    ASSERT_TRUE(matrixReaderTest->openFile(filepath, OpenViBE::CSV::EFileAccessMode::Read));
    std::vector<size_t> dimensionSizes;
    std::vector<std::string> labels;

    ASSERT_FALSE(matrixReaderTest->parseHeader());
    ASSERT_EQ(matrixReaderTest->getFormatType(), OpenViBE::CSV::EStreamType::Undefined);
    ASSERT_FALSE(matrixReaderTest->getStreamedMatrixInformation(dimensionSizes, labels));

    ASSERT_TRUE(matrixReaderTest->closeFile());
    releaseCSVHandler(matrixReaderTest);
}

TEST(CSV_Reader_Test_Case, covarianceMatrixReaderTooManyLabels)
{
    OpenViBE::CSV::ICSVHandler* matrixReaderTest = OpenViBE::CSV::createCSVHandler();
    const std::string filepath                   = dataDirectory + "testCSVCovarMatrixTooManyLabels.csv";
    ASSERT_TRUE(matrixReaderTest->openFile(filepath, OpenViBE::CSV::EFileAccessMode::Read));

    std::vector<size_t> dimensionSizes;
    std::vector<std::string> labels;

    ASSERT_FALSE(matrixReaderTest->parseHeader());
    ASSERT_EQ(matrixReaderTest->getFormatType(), OpenViBE::CSV::EStreamType::Undefined);

    ASSERT_TRUE(matrixReaderTest->closeFile());
    releaseCSVHandler(matrixReaderTest);
}

TEST(CSV_Reader_Test_Case, stimulationsNormalGoodStims)
{
    OpenViBE::CSV::ICSVHandler* stimReaderTest = OpenViBE::CSV::createCSVHandler();
    const std::string filepath                 = dataDirectory + "testCSVStimulationsReader01.csv";
    ASSERT_TRUE(stimReaderTest->openFile(filepath, OpenViBE::CSV::EFileAccessMode::Read));
    std::vector<OpenViBE::CSV::SStimulationChunk> stimulations;

    ASSERT_TRUE(stimReaderTest->parseHeader());
    ASSERT_EQ(stimReaderTest->getFormatType(), OpenViBE::CSV::EStreamType::Stimulations);
    ASSERT_TRUE(stimReaderTest->readEventsFromFile(5, stimulations))
    << stimReaderTest->getLastLogError() << ".Details: " << stimReaderTest->getLastErrorString();

    ASSERT_EQ(stimulations.size(), 3); //Only 3 stims in the file
    ASSERT_TRUE(stimReaderTest->closeFile());
    releaseCSVHandler(stimReaderTest);
}

TEST(CSV_Reader_Test_Case, stimulationsNormalWrongStim)
{
    OpenViBE::CSV::ICSVHandler* stimReaderTest = OpenViBE::CSV::createCSVHandler();
    const std::string filepath                 = dataDirectory + "testCSVStimulationsWrongStimulation.csv";
    ASSERT_TRUE(stimReaderTest->openFile(filepath, OpenViBE::CSV::EFileAccessMode::Read));
    std::vector<OpenViBE::CSV::SStimulationChunk> stimulations;

    ASSERT_TRUE(stimReaderTest->parseHeader());
    ASSERT_EQ(stimReaderTest->getFormatType(), OpenViBE::CSV::EStreamType::Stimulations);
    ASSERT_FALSE(stimReaderTest->readEventsFromFile(5, stimulations))
    << stimReaderTest->getLastLogError() << ".Details: " << stimReaderTest->getLastErrorString();

    ASSERT_TRUE(stimReaderTest->closeFile());
    releaseCSVHandler(stimReaderTest);
}

TEST(CSV_Reader_Test_Case, stimulationsNormalWrongHeader)
{
    OpenViBE::CSV::ICSVHandler* stimReaderTest = OpenViBE::CSV::createCSVHandler();
    const std::string filepath                 = dataDirectory + "testCSVStimulationsWrongHeader.csv";
    ASSERT_TRUE(stimReaderTest->openFile(filepath, OpenViBE::CSV::EFileAccessMode::Read));

    ASSERT_FALSE(stimReaderTest->parseHeader());
    ASSERT_EQ(stimReaderTest->getFormatType(), OpenViBE::CSV::EStreamType::Undefined);

    ASSERT_TRUE(stimReaderTest->closeFile());
    releaseCSVHandler(stimReaderTest);
}


int uoCSVReaderTest(int argc, char* argv[])
{
    if (argv[1] != nullptr) { dataDirectory = argv[1]; }
    testing::InitGoogleTest(&argc, argv);
    ::testing::GTEST_FLAG(filter) = "CSV_Reader_Test_Case.*";
    return RUN_ALL_TESTS();
}
