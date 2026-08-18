///-------------------------------------------------------------------------------------------------
/// 
/// \file uoCSVWriterTest.cpp
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

#include "gtest/gtest.h"

#include "csv/ovICSV.h"

#include <string>
#include <fstream>
#include <streambuf>
#include <numeric>

static std::string directoryPath = "";

TEST(CSV_Writer_Test_Case, signalWriterNormalGoodSignal)
{
    OpenViBE::CSV::ICSVHandler* handler = OpenViBE::CSV::createCSVHandler();
    const std::string filename          = directoryPath + "testCSVSignalWriter01.csv";

    ASSERT_TRUE(handler->openFile(filename, OpenViBE::CSV::EFileAccessMode::Write));
    handler->setFormatType(OpenViBE::CSV::EStreamType::Signal);

    ASSERT_TRUE(handler->setSignalInformation({ "O1", "O2", "Pz", "P1", "P2" }, 8, 8));

    double index = 0.0;
    while (index < 1.2) {
        const double epoch = index / 0.5;
        ASSERT_TRUE(handler->addSample({ index, index + 0.125, { -10.10, -5.05, 0.00, 5.05, 10.10 }, size_t(epoch) }));
        if (index == 0.25 || index == 0.75) { ASSERT_TRUE(handler->addEvent(35000, index, 0.0)); }
        index += 0.125;
    }

    ASSERT_TRUE(handler->writeHeaderToFile());
    ASSERT_TRUE(handler->writeDataToFile());
    ASSERT_TRUE(handler->closeFile());
    releaseCSVHandler(handler);
}

TEST(CSV_Writer_Test_Case, signalWriterNoStimulations)
{
    OpenViBE::CSV::ICSVHandler* handler = OpenViBE::CSV::createCSVHandler();
    const std::string filename          = directoryPath + "testCSVSignalWriter02.csv";

    ASSERT_TRUE(handler->openFile(filename, OpenViBE::CSV::EFileAccessMode::Write));
    handler->setFormatType(OpenViBE::CSV::EStreamType::Signal);

    ASSERT_TRUE(handler->setSignalInformation({ "O1", "O2", "Pz", "P1", "P2" }, 8, 8));
    ASSERT_TRUE(handler->noEventsUntilDate(2.0));

    double index = 0.0;
    while (index < 1.2) {
        const double epoch = index / 0.5;
        ASSERT_TRUE(handler->addSample({ index, index + 0.125, { -10.10, -5.05, 0.00, 5.05, 10.10 }, size_t(epoch) }));
        index += 0.125;
    }

    ASSERT_TRUE(handler->writeHeaderToFile());
    ASSERT_TRUE(handler->writeDataToFile());
    ASSERT_TRUE(handler->closeFile());
    releaseCSVHandler(handler);
}

TEST(CSV_Writer_Test_Case, signalWriterNoFileOpen)
{
    OpenViBE::CSV::ICSVHandler* handler = OpenViBE::CSV::createCSVHandler();
    handler->setFormatType(OpenViBE::CSV::EStreamType::Signal);
    ASSERT_TRUE(handler->setSignalInformation({ "O1", "O2", "Pz", "P1", "P2" }, 8, 8));

    double index = 0.0;
    while (index < 1.2) {
        const double epoch = index / 0.5;
        ASSERT_TRUE(handler->addSample({ index, index + 0.125, { -10.10, -5.05, 0.00, 5.05, 10.10 }, size_t(epoch) }));
        index += 0.125;
    }

    ASSERT_FALSE(handler->writeHeaderToFile());
    ASSERT_TRUE(handler->closeFile());
    releaseCSVHandler(handler);
}

TEST(CSV_Writer_Test_Case, signalWriterWrongInputType)
{
    OpenViBE::CSV::ICSVHandler* handler = OpenViBE::CSV::createCSVHandler();
    handler->setFormatType(OpenViBE::CSV::EStreamType::Spectrum);
    ASSERT_FALSE(handler->setSignalInformation({ "O1", "O2", "Pz", "P1", "P2" }, 8, 8));
    releaseCSVHandler(handler);
}

TEST(CSV_Writer_Test_Case, signalWriterWrongMatrixSize)
{
    OpenViBE::CSV::ICSVHandler* handler = OpenViBE::CSV::createCSVHandler();
    const std::string filename          = directoryPath + "testCSVSignalWriter05.csv";

    ASSERT_TRUE(handler->openFile(filename, OpenViBE::CSV::EFileAccessMode::Write));
    handler->setFormatType(OpenViBE::CSV::EStreamType::Signal);
    ASSERT_TRUE(handler->setSignalInformation({ "O1", "O2", "Pz", "P1", "P2" }, 8, 8));
    ASSERT_FALSE(handler->addSample({ 0, 0.125, { -20.20, -10.10, -5.05, 5.05, 10.10, 20.20 }, 0 }));
    ASSERT_TRUE(handler->closeFile());
    releaseCSVHandler(handler);
}

// should have nothing in the file
TEST(CSV_Writer_Test_Case, signalWriterTonsOfSignalWithoutSetNoEventsUntilDate)
{
    OpenViBE::CSV::ICSVHandler* handler = OpenViBE::CSV::createCSVHandler();
    const std::string filename          = directoryPath + "testCSVSignalWriter06.csv";

    ASSERT_TRUE(handler->openFile(filename, OpenViBE::CSV::EFileAccessMode::Write));
    handler->setFormatType(OpenViBE::CSV::EStreamType::Signal);
    ASSERT_TRUE(handler->setSignalInformation({ "O1", "O2", "Pz", "P1", "P2" }, 8, 8));

    double time = 0.0;
    while (time < 100.0) {
        ASSERT_TRUE(handler->addSample({ time, time + 0.125, { -20.20, -10.10, 0.0, 10.10, 20.20 }, size_t(time / 0.125) }));
        time += 0.125;
    }

    ASSERT_TRUE(handler->writeHeaderToFile());
    ASSERT_TRUE(handler->writeDataToFile());
    releaseCSVHandler(handler);
}

// file should be full
TEST(CSV_Writer_Test_Case, signalWriterTonsOfSignalWithSetNoEventsUntilDate)
{
    OpenViBE::CSV::ICSVHandler* handler = OpenViBE::CSV::createCSVHandler();
    const std::string filename          = directoryPath + "testCSVSignalWriter07.csv";

    ASSERT_TRUE(handler->openFile(filename, OpenViBE::CSV::EFileAccessMode::Write));
    handler->setFormatType(OpenViBE::CSV::EStreamType::Signal);
    ASSERT_TRUE(handler->setSignalInformation({ "O1", "O2", "Pz", "P1", "P2" }, 8, 8));
    ASSERT_TRUE(handler->noEventsUntilDate(100.001));

    double time = 0.0;
    while (time < 100.0) {
        ASSERT_TRUE(handler->addSample({ time, time + 0.125, { -20.20, -10.10, 0.0, 10.10, 20.20 }, size_t(time / 0.5) }));
        time += 0.125;
    }

    ASSERT_TRUE(handler->writeHeaderToFile());
    ASSERT_TRUE(handler->writeDataToFile());
    ASSERT_TRUE(handler->writeAllDataToFile());
    releaseCSVHandler(handler);
}

TEST(CSV_Writer_Test_Case, signalWriterOnlyLastMatrix)
{
    OpenViBE::CSV::ICSVHandler* handler = OpenViBE::CSV::createCSVHandler();
    handler->setLastMatrixOnlyMode(true);
    const std::string filename = directoryPath + "testCSVSignalWriter08.csv";
    const std::string expectedFileContent(
        "Time:8Hz,Epoch,O1,O2,Pz,P1,P2,Event Id,Event Date,Event Duration\n1.0000000000,2,-10.1000000000,-5.0500000000,0.0000000000,5.0500000000,10.1000000000,35000,1.0000000000,0.0000000000\n1.1250000000,2,-10.1000000000,-5.0500000000,0.0000000000,5.0500000000,10.1000000000,,,\n");
    ASSERT_TRUE(handler->openFile(filename, OpenViBE::CSV::EFileAccessMode::Write));
    handler->setFormatType(OpenViBE::CSV::EStreamType::Signal);

    ASSERT_TRUE(handler->setSignalInformation({ "O1", "O2", "Pz", "P1", "P2" }, 8, 8));

    double index = 0.0;
    while (index < 1.2) {
        const double epoch = index / 0.5;
        ASSERT_TRUE(handler->addSample({ index, index + 0.125, { -10.10, -5.05, 0.00, 5.05, 10.10 }, size_t(epoch) }));
        if (index == 0.25 || index == 0.75 || index == 1.0) { ASSERT_TRUE(handler->addEvent(35000, index, 0.0)); }
        index += 0.125;
    }

    ASSERT_TRUE(handler->writeHeaderToFile());
    ASSERT_TRUE(handler->writeAllDataToFile());
    ASSERT_TRUE(handler->closeFile());
    releaseCSVHandler(handler);

    std::ifstream t(filename);
    const std::string out((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

    ASSERT_STREQ(out.c_str(), expectedFileContent.c_str());
}

TEST(CSV_Writer_Test_Case, spectrumWriterNormalGoodSignal)
{
    OpenViBE::CSV::ICSVHandler* handler = OpenViBE::CSV::createCSVHandler();
    const std::string filename          = directoryPath + "testCSVSpectrumWriter01.csv";

    ASSERT_TRUE(handler->openFile(filename, OpenViBE::CSV::EFileAccessMode::Write));
    handler->setFormatType(OpenViBE::CSV::EStreamType::Spectrum);

    std::vector<double> frequencyAbscissa(64);
    std::iota(frequencyAbscissa.begin(), frequencyAbscissa.end(), 0.0);

    ASSERT_TRUE(handler->setSpectrumInformation({ "O1", "O2" }, frequencyAbscissa, 256));
    double time = 0;
    for (size_t i = 0; i < 10; ++i) {
        const size_t epoch = i / 4;
        std::vector<double> sample(128);
        std::iota(sample.begin(), sample.end(), -64);

        ASSERT_TRUE(handler->addSample({ time, time + 1.0, sample, epoch }));

        time += 0.125;
        if (i == 5 || i == 3 || i == 7) { ASSERT_TRUE(handler->addEvent(35001, time, 1.0)); }
    }

    ASSERT_TRUE(handler->writeHeaderToFile());
    ASSERT_TRUE(handler->writeDataToFile());
    ASSERT_TRUE(handler->closeFile());
    releaseCSVHandler(handler);
}

TEST(CSV_Writer_Test_Case, spectrumWriterWrongInputType)
{
    OpenViBE::CSV::ICSVHandler* handler = OpenViBE::CSV::createCSVHandler();
    const std::string filename          = directoryPath + "testCSVSpectrumWriter02.csv";

    ASSERT_TRUE(handler->openFile(filename, OpenViBE::CSV::EFileAccessMode::Write));
    handler->setFormatType(OpenViBE::CSV::EStreamType::Signal);
    std::vector<double> frequencyAbscissa(64);
    std::iota(frequencyAbscissa.begin(), frequencyAbscissa.end(), 0.0);

    ASSERT_FALSE(handler->setSpectrumInformation({ "O1", "O2" }, frequencyAbscissa, 256));

    ASSERT_TRUE(handler->closeFile());
    releaseCSVHandler(handler);
}

TEST(CSV_Writer_Test_Case, spectrumWriterWrongMatrixSize)
{
    OpenViBE::CSV::ICSVHandler* handler = OpenViBE::CSV::createCSVHandler();
    const std::string filename          = directoryPath + "testCSVSpectrumWriter03.csv";

    ASSERT_TRUE(handler->openFile(filename, OpenViBE::CSV::EFileAccessMode::Write));
    handler->setFormatType(OpenViBE::CSV::EStreamType::Spectrum);
    std::vector<double> frequencyAbscissa(64);
    std::iota(frequencyAbscissa.begin(), frequencyAbscissa.end(), 0.0);
    ASSERT_TRUE(handler->setSpectrumInformation({ "O1", "O2" }, frequencyAbscissa, 256));

    ASSERT_FALSE(handler->addSample({ 0, 1, { -20.20, -10.10, 0.0, 10.10, 20.20 }, 0 }));

    ASSERT_TRUE(handler->closeFile());
    releaseCSVHandler(handler);
}

TEST(CSV_Writer_Test_Case, matrixWriterNormalGoodSignal)
{
    OpenViBE::CSV::ICSVHandler* handler = OpenViBE::CSV::createCSVHandler();
    const std::string filename          = directoryPath + "testCSVMatrixWriter01.csv";

    ASSERT_TRUE(handler->openFile(filename, OpenViBE::CSV::EFileAccessMode::Write));
    handler->setFormatType(OpenViBE::CSV::EStreamType::StreamedMatrix);

    ASSERT_TRUE(handler->setStreamedMatrixInformation({ 2, 2, 2 }, { "LA", "LB", "1", "2", "X", "Y" }));

    for (size_t i = 0; i < 50; ++i) {
        const size_t epoch = i / 10;
        ASSERT_TRUE(handler->addSample({ double(i), double(i)+1.0, { -20.20, -15.15, -10.10, -5.05, 5.05, 10.10, 15.15, 20.20 }, epoch }));

        if (i == 5 || i == 3 || i == 7) { ASSERT_TRUE(handler->addEvent(35001, double(i)+3.5, 1.0)); }
    }

    ASSERT_TRUE(handler->writeHeaderToFile());
    ASSERT_TRUE(handler->writeDataToFile());
    ASSERT_TRUE(handler->closeFile());
    releaseCSVHandler(handler);
}

TEST(CSV_Writer_Test_Case, matrixWriterEmptyLabels)
{
    OpenViBE::CSV::ICSVHandler* handler = OpenViBE::CSV::createCSVHandler();
    const std::string filename          = directoryPath + "testCSVMatrixWriter02.csv";

    ASSERT_TRUE(handler->openFile(filename, OpenViBE::CSV::EFileAccessMode::Write));
    handler->setFormatType(OpenViBE::CSV::EStreamType::StreamedMatrix);

    ASSERT_TRUE(handler->setStreamedMatrixInformation({ 2, 2, 2 }, { "", "", "", "", "", "" }));

    for (size_t i = 0; i < 50; ++i) {
        const size_t epoch = i / 10;
        ASSERT_TRUE(handler->addSample({ double(i), double(i)+1.0, { -20.20, -15.15, -10.10, -5.05, 5.05, 10.10, 15.15, 20.20 }, epoch }));

        if (i == 5 || i == 3 || i == 7) { ASSERT_TRUE(handler->addEvent(35001, double(i)+3.5, 1.0)); }
    }

    ASSERT_TRUE(handler->writeHeaderToFile());
    ASSERT_TRUE(handler->writeDataToFile());
    ASSERT_TRUE(handler->closeFile());
    releaseCSVHandler(handler);
}

TEST(CSV_Writer_Test_Case, matrixWithDifferentsDimensionSizes)
{
    OpenViBE::CSV::ICSVHandler* handler = OpenViBE::CSV::createCSVHandler();
    const std::string filename          = directoryPath + "testCSVMatrixWriter03.csv";

    ASSERT_TRUE(handler->openFile(filename, OpenViBE::CSV::EFileAccessMode::Write));
    handler->setFormatType(OpenViBE::CSV::EStreamType::StreamedMatrix);

    ASSERT_TRUE(handler->setStreamedMatrixInformation({ 1, 4 }, { "L1", "A", "B", "C", "D" }));

    for (size_t i = 0; i < 50; ++i) {
        const size_t epoch = i / 10;
        ASSERT_TRUE(handler->addSample({ double(i), double(i)+1.0, { -20.20, -10.10, 10.10, 20.20 }, epoch }));

        if (i == 5 || i == 3 || i == 7) { ASSERT_TRUE(handler->addEvent(35001, double(i)+3.5, 1.0)); }
    }

    ASSERT_TRUE(handler->writeHeaderToFile());
    ASSERT_TRUE(handler->writeDataToFile());
    ASSERT_TRUE(handler->closeFile());
    releaseCSVHandler(handler);
}

TEST(CSV_Writer_Test_Case, matrixWriterWrongMatrixSize)
{
    OpenViBE::CSV::ICSVHandler* handler = OpenViBE::CSV::createCSVHandler();
    const std::string filename          = directoryPath + "testCSVMatrixWriter04.csv";

    ASSERT_TRUE(handler->openFile(filename, OpenViBE::CSV::EFileAccessMode::Write));
    handler->setFormatType(OpenViBE::CSV::EStreamType::StreamedMatrix);
    ASSERT_TRUE(handler->setStreamedMatrixInformation({ 2, 2, 2 }, { "", "", "", "", "", "" }));

    ASSERT_FALSE(handler->addSample({ 0, 1.0, { -25.25, -20.20, -15.15, -10.10, -5.05, 5.05, 10.10, 15.15, 20.20, 25.25 }, 0 }));

    ASSERT_TRUE(handler->closeFile());
    releaseCSVHandler(handler);
}

TEST(CSV_Writer_Test_Case, matrixWithDifferentsDimensionSizes2)
{
    OpenViBE::CSV::ICSVHandler* handler = OpenViBE::CSV::createCSVHandler();
    const std::string filename          = directoryPath + "testCSVMatrixWriter05.csv";

    ASSERT_TRUE(handler->openFile(filename, OpenViBE::CSV::EFileAccessMode::Write));
    handler->setFormatType(OpenViBE::CSV::EStreamType::StreamedMatrix);

    ASSERT_TRUE(
        handler->setStreamedMatrixInformation({ 6, 8, 2 }, { "L1", "L2", "L3", "L4", "L5", "L6", "A1", "B2", "C3", "D4", "E5", "F6", "G7", "H8", "X", "Y" }));

    std::vector<double> values(96);
    std::iota(values.begin(), values.end(), 0.0);

    for (size_t i = 0; i < 50; ++i) {
        const size_t epoch = i / 10;
        ASSERT_TRUE(handler->addSample({ double(i), double(i)+1.0, values, epoch }));

        if (i == 5 || i == 3 || i == 7) { ASSERT_TRUE(handler->addEvent(35001, double(i)+3.5, 1.0)); }
    }

    ASSERT_TRUE(handler->writeHeaderToFile());
    ASSERT_TRUE(handler->writeDataToFile());
    ASSERT_TRUE(handler->closeFile());
    releaseCSVHandler(handler);
}

TEST(CSV_Writer_Test_Case, matrixWithDifferentsDimensionSizes3)
{
    OpenViBE::CSV::ICSVHandler* handler = OpenViBE::CSV::createCSVHandler();
    const std::string filename          = directoryPath + "testCSVMatrixWriter06.csv";

    ASSERT_TRUE(handler->openFile(filename, OpenViBE::CSV::EFileAccessMode::Write));
    handler->setFormatType(OpenViBE::CSV::EStreamType::StreamedMatrix);

    ASSERT_TRUE(handler->setStreamedMatrixInformation({ 4, 1, 4 }, { "L1", "L2", "L3", "L4", "X", "R1", "R2", "R3", "R4" }));

    std::vector<double> values(16);
    std::iota(values.begin(), values.end(), 0.0);

    for (size_t i = 0; i < 50; ++i) {
        const size_t epoch = i / 10;
        ASSERT_TRUE(handler->addSample({ double(i), double(i)+1.0, values, epoch }));

        if (i == 5 || i == 3 || i == 7) { ASSERT_TRUE(handler->addEvent(35001, double(i)+3.5, 1.0)); }
    }

    ASSERT_TRUE(handler->writeHeaderToFile());
    ASSERT_TRUE(handler->writeDataToFile());
    ASSERT_TRUE(handler->closeFile());
    releaseCSVHandler(handler);
}

// As of 10/01/2017 (commit a11210cf1c3fd81bb52095c7c9c6006c760218a2), this is valid for
TEST(CSV_Writer_Test_Case, matrixWriterWithInvalidTime)
{
    OpenViBE::CSV::ICSVHandler* handler = OpenViBE::CSV::createCSVHandler();
    const std::string filename          = directoryPath + "testCSVMatrixWriter07.csv";

    ASSERT_TRUE(handler->openFile(filename, OpenViBE::CSV::EFileAccessMode::Write));
    handler->setFormatType(OpenViBE::CSV::EStreamType::StreamedMatrix);

    ASSERT_TRUE(handler->setStreamedMatrixInformation({ 1, 1, 1 }, { "X", "Y", "Z" }));

    ASSERT_FALSE(handler->addSample({ 1.0, 0, { -20.20, -15.15, -10.10 }, 0 }));
    ASSERT_FALSE(handler->addSample({ -1.0, 0, { -20.20, -15.15, -10.10 }, 1 }));
    ASSERT_FALSE(handler->addSample({ -1.0, -0.5, { -20.20, -15.15, -10.10 }, 2 }));
    ASSERT_FALSE(handler->addSample({ 1.0, -1.0, { -20.20, -15.15, -10.10 }, 3 }));

    ASSERT_TRUE(handler->closeFile());

    releaseCSVHandler(handler);
}

TEST(CSV_Writer_Test_Case, matrixWriterOnlyLastMatrix)
{
    OpenViBE::CSV::ICSVHandler* handler = OpenViBE::CSV::createCSVHandler();
    handler->setLastMatrixOnlyMode(true);
    const std::string filename = directoryPath + "testCSVMatrixWriter08.csv";
    const std::string expectedFileContent(
        "Time:2x2x2,End Time,LA:1:X,LA:1:Y,LA:2:X,LA:2:Y,LB:1:X,LB:1:Y,LB:2:X,LB:2:Y,Event Id,Event Date,Event Duration\n49.0000000000,50.0000000000,49.0000000000,1.0000000000,2.0000000000,3.0000000000,4.0000000000,5.0000000000,6.0000000000,7.0000000000,,,\n");
    ASSERT_TRUE(handler->openFile(filename, OpenViBE::CSV::EFileAccessMode::Write));
    handler->setFormatType(OpenViBE::CSV::EStreamType::StreamedMatrix);

    ASSERT_TRUE(handler->setStreamedMatrixInformation({ 2, 2, 2 }, { "LA", "LB", "1", "2", "X", "Y" }));
    ASSERT_TRUE(handler->writeHeaderToFile());

    for (size_t i = 0; i < 50; ++i) {
        const size_t epoch = i / 10;
        ASSERT_TRUE(handler->addSample({ double(i), double(i) + 1.0, { double(i), 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0 }, epoch }));

        if (i == 3 || i == 5 || i == 7) { ASSERT_TRUE(handler->addEvent(35001, double(i) + 3.5, 1.0)); }
    }

    ASSERT_TRUE(handler->writeAllDataToFile());
    ASSERT_TRUE(handler->closeFile());
    releaseCSVHandler(handler);

    std::ifstream t(filename);
    const std::string out((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

    ASSERT_STREQ(out.c_str(), expectedFileContent.c_str());
}


TEST(CSV_Writer_Test_Case, featureVectorNormalGoodSignal)
{
    OpenViBE::CSV::ICSVHandler* handler = OpenViBE::CSV::createCSVHandler();
    const std::string filename          = directoryPath + "testCSVFeatureVectorWriter01.csv";

    ASSERT_TRUE(handler->openFile(filename, OpenViBE::CSV::EFileAccessMode::Write));
    handler->setFormatType(OpenViBE::CSV::EStreamType::FeatureVector);

    ASSERT_TRUE(handler->setFeatureVectorInformation({ "F1", "F2", "F3" }));

    for (size_t i = 0; i < 50; ++i) {
        const size_t epoch = i / 10;
        ASSERT_TRUE(handler->addSample({ double(i), double(i)+1.0, { -20.20, -15.15, -10.10 }, epoch }));

        if (i == 5 || i == 3 || i == 7) { ASSERT_TRUE(handler->addEvent(35001, double(i)+3.5, 1.0)); }
    }

    ASSERT_TRUE(handler->writeHeaderToFile());
    ASSERT_TRUE(handler->writeDataToFile());
    ASSERT_TRUE(handler->closeFile());

    releaseCSVHandler(handler);
}

TEST(CSV_Writer_Test_Case, featureVectorEmptyLabels)
{
    OpenViBE::CSV::ICSVHandler* handler = OpenViBE::CSV::createCSVHandler();
    const std::string filename          = directoryPath + "testCSVFeatureVectorWriter02.csv";

    ASSERT_TRUE(handler->openFile(filename, OpenViBE::CSV::EFileAccessMode::Write));
    handler->setFormatType(OpenViBE::CSV::EStreamType::FeatureVector);

    ASSERT_TRUE(handler->setFeatureVectorInformation({ "", "", "" }));

    for (size_t i = 0; i < 50; ++i) {
        const size_t epoch = i / 10;
        ASSERT_TRUE(handler->addSample({ double(i), double(i)+1.0, { -20.20, -15.15, -10.10 }, epoch }));
    }

    ASSERT_TRUE(handler->writeHeaderToFile());
    ASSERT_TRUE(handler->writeDataToFile());
    ASSERT_TRUE(handler->closeFile());

    releaseCSVHandler(handler);
}

TEST(CSV_Writer_Test_Case, featureVectorWrongVectorSize)
{
    OpenViBE::CSV::ICSVHandler* handler = OpenViBE::CSV::createCSVHandler();
    const std::string filename          = directoryPath + "testCSVFeatureVectorWriter03.csv";

    ASSERT_TRUE(handler->openFile(filename, OpenViBE::CSV::EFileAccessMode::Write));
    handler->setFormatType(OpenViBE::CSV::EStreamType::FeatureVector);

    ASSERT_TRUE(handler->setFeatureVectorInformation({ "F1", "F2", "F3" }));

    ASSERT_FALSE(handler->addSample({ 0, 1.0, { -20.20, -15.15, -10.10, 12 }, 0 }));
    ASSERT_FALSE(handler->addSample({ 1.0, 2.0, { -20.20, -15.15 }, 1 }));

    ASSERT_TRUE(handler->closeFile());

    releaseCSVHandler(handler);
}


TEST(CSV_Writer_Test_Case, covarianceMatrixWriterNormalGoodSignal)
{
    OpenViBE::CSV::ICSVHandler* handler = OpenViBE::CSV::createCSVHandler();
    const std::string filename          = directoryPath + "testCSVCovarMatrixWriter01.csv";

    ASSERT_TRUE(handler->openFile(filename, OpenViBE::CSV::EFileAccessMode::Write));
    handler->setFormatType(OpenViBE::CSV::EStreamType::CovarianceMatrix);

    ASSERT_TRUE(handler->setStreamedMatrixInformation({ 2, 2, 2 }, { "C1", "C2", "C1", "C2", "Matrix 1", "Matrix 2" }));

    for (size_t i = 0; i < 50; ++i) {
        const size_t epoch = i / 10;
        ASSERT_TRUE(handler->addSample({ double(i), double(i)+1.0, { -20.20, -15.15, -10.10, -5.05, 5.05, 10.10, 15.15, 20.20 }, epoch }));
    }
    handler->noEventsUntilDate(20.0);

    ASSERT_TRUE(handler->writeHeaderToFile());
    ASSERT_TRUE(handler->writeAllDataToFile());
    ASSERT_TRUE(handler->closeFile());
    releaseCSVHandler(handler);
}

TEST(CSV_Writer_Test_Case, covarianceMatrixWriterEmptyLabels)
{
    OpenViBE::CSV::ICSVHandler* handler = OpenViBE::CSV::createCSVHandler();
    const std::string filename          = directoryPath + "testCSVCovarMatrixWriter02.csv";

    ASSERT_TRUE(handler->openFile(filename, OpenViBE::CSV::EFileAccessMode::Write));
    handler->setFormatType(OpenViBE::CSV::EStreamType::CovarianceMatrix);

    ASSERT_TRUE(handler->setStreamedMatrixInformation({ 2, 2, 2 }, { "", "", "", "", "", "" }));

    for (size_t i = 0; i < 50; ++i) {
        const size_t epoch = i / 10;
        ASSERT_TRUE(handler->addSample({ double(i), double(i)+1.0, { -20.20, -15.15, -10.10, -5.05, 5.05, 10.10, 15.15, 20.20 }, epoch }));
    }

    ASSERT_TRUE(handler->writeHeaderToFile());
    ASSERT_TRUE(handler->writeAllDataToFile());
    ASSERT_TRUE(handler->closeFile());
    releaseCSVHandler(handler);
}


TEST(CSV_Writer_Test_Case, covarianceMatrixWriterWrongMatrixSize)
{
    OpenViBE::CSV::ICSVHandler* handler = OpenViBE::CSV::createCSVHandler();
    const std::string filename          = directoryPath + "testCSVCovarMatrixWriter04.csv";

    ASSERT_TRUE(handler->openFile(filename, OpenViBE::CSV::EFileAccessMode::Write));
    handler->setFormatType(OpenViBE::CSV::EStreamType::CovarianceMatrix);
    ASSERT_TRUE(handler->setStreamedMatrixInformation({ 2, 2, 2 }, { "", "", "", "", "", "" }));

    ASSERT_FALSE(handler->addSample({ 0, 1.0, { -25.25, -20.20, -15.15, -10.10, -5.05, 5.05, 10.10, 15.15, 20.20, 25.25 }, 0 }));

    ASSERT_TRUE(handler->closeFile());
    releaseCSVHandler(handler);
}

TEST(CSV_Writer_Test_Case, stimulationsOnlyWriterHeader)
{
    OpenViBE::CSV::ICSVHandler* handler   = OpenViBE::CSV::createCSVHandler();
    const std::string filename            = directoryPath + "testCSVStimulationsWriter01.csv";
    const std::string expectedFileContent = "Event Id,Event Date,Event Duration";
    ASSERT_TRUE(handler->openFile(filename, OpenViBE::CSV::EFileAccessMode::Write));
    handler->setFormatType(OpenViBE::CSV::EStreamType::Stimulations);

    ASSERT_TRUE(handler->writeHeaderToFile());
    ASSERT_TRUE(handler->closeFile());
    releaseCSVHandler(handler);

    // Verification
    std::ifstream ifs(filename);
    std::string line;
    ASSERT_TRUE(std::getline(ifs, line));
    ifs.close();

    ASSERT_STREQ(line.c_str(), expectedFileContent.c_str());
}


TEST(CSV_Writer_Test_Case, stimulationsOnlyWriterGoodStims)
{
    OpenViBE::CSV::ICSVHandler* handler          = OpenViBE::CSV::createCSVHandler();
    const std::string filename                   = directoryPath + "testCSVStimulations02.csv";
    const std::vector<uint64_t> stimCodes        = { 33025, 33026, 33027 };
    const std::vector<std::string> expectedStims = {
        "33025,1.0000000000,0.0000000000",
        "33026,2.0000000000,0.0000000000",
        "33027,3.0000000000,0.0000000000"
    };

    ASSERT_TRUE(handler->openFile(filename, OpenViBE::CSV::EFileAccessMode::Write));
    handler->setFormatType(OpenViBE::CSV::EStreamType::Stimulations);

    for (size_t i = 0; i < stimCodes.size(); ++i) { handler->addEvent(stimCodes[i], double(i + 1), 0.0); }


    ASSERT_TRUE(handler->writeHeaderToFile());
    ASSERT_TRUE(handler->writeAllDataToFile());
    ASSERT_TRUE(handler->closeFile());
    releaseCSVHandler(handler);

    std::ifstream ifs(filename);
    std::string line;
    size_t i = 0;
    ifs.ignore(std::numeric_limits<std::streamsize>::max(), '\n');  // Ignore header
    while (std::getline(ifs, line)) {
        ASSERT_LT(i, expectedStims.size());
        ASSERT_STREQ(line.c_str(), expectedStims[i++].c_str());
    }
    ifs.close();
}

TEST(CSV_Writer_Test_Case, stimulationsOnlyWriterUnexpectedData)
{
    OpenViBE::CSV::ICSVHandler* handler = OpenViBE::CSV::createCSVHandler();
    const std::string filename          = directoryPath + "testCSVStimulations03.csv";

    ASSERT_TRUE(handler->openFile(filename, OpenViBE::CSV::EFileAccessMode::Write));
    handler->setFormatType(OpenViBE::CSV::EStreamType::Stimulations);

    ASSERT_FALSE(handler->addSample({ 0.0, 0.5, { -10.10, -5.05, 0.00, 5.05, 10.10 }, 1 }));

    ASSERT_TRUE(handler->closeFile());
    releaseCSVHandler(handler);
}

int uoCSVWriterTest(int argc, char* argv[])
{
    if (argv[1] != nullptr) { directoryPath = argv[1]; }
    testing::InitGoogleTest(&argc, argv);
    ::testing::GTEST_FLAG(filter) = "CSV_Writer_Test_Case.*";
    return RUN_ALL_TESTS();
}
