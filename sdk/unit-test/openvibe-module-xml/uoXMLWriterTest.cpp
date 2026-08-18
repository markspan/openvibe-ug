///-------------------------------------------------------------------------------------------------
/// 
/// \file uoXMLWriterTest.cpp
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

#include <iostream>
#include <fstream>
#include <cstdio>

#include <xml/IWriter.h>
#include <xml/IXMLHandler.h>

#include <fs/Files.h>

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

class CWriterCallBack : public XML::IWriterCallBack
{
public:
    explicit CWriterCallBack(const char* filename) : m_file(FS::Files::open(filename, "wb")) { }
    ~CWriterCallBack() override { if (m_file) { std::fclose(m_file); } }    // in case release is not called

    void write(const char* outputData) override { if (m_file) { std::fputs(outputData, m_file); } }

    // necessary thjor the test to close the stream and re-open it for inspection
    void release()
    {
        std::fclose(m_file);
        m_file = nullptr;
    }

private:
    std::FILE* m_file{ nullptr };
};

TEST(XML_Writer_Test_Case, validateWriter)
{
    std::string expectedFile = DATA_DIR "/ref_data.xml";
    std::string outputFile   = TEMP_DIR "/uoXMLWriterTest.xml";

    FS::Files::createPath(TEMP_DIR);
    ASSERT_TRUE(FS::Files::directoryExists(TEMP_DIR));

    FS::Files::removeAll(outputFile.c_str());
    ASSERT_FALSE(FS::Files::fileExists(outputFile.c_str()));

    // The test serializes a known xml sequence and compares the output
    // to a reference.

    // serializing
    CWriterCallBack callback(outputFile.c_str());

    XML::IWriter* writer = createWriter(callback);

    writer->openChild("Document"); ///< Document Node
    writer->setAttribute("name", "test_reference");

    writer->openChild("NodeWithChildren"); ///< NodeWithChildren Node

    writer->openChild("ChildNodeWithData");
    writer->setChildData("child node data with more than 10 alphanumeric characters");
    writer->closeChild();

    writer->openChild("ChildNodeEmpty");
    writer->closeChild();

    writer->openChild("ChildNodeWithChildren");
    writer->openChild("ChildNodeEmpty");
    writer->closeChild();
    writer->closeChild();

    writer->closeChild(); ///< NodeWithChildren END

    writer->openChild("NodeWithData");
    writer->setAttribute("status", "hasData");
    writer->setChildData("node data with special characters <>,;:!?./&\"'(-_)=~#{[|`\\^@]}/*-+");
    writer->closeChild();

    writer->openChild("NodeEmptyWithNumber666");
    writer->setAttribute("status", "noData");
    writer->setAttribute("ref", "test");
    writer->closeChild();

    writer->closeChild(); ///< Document Node END

    writer->release();
    callback.release();

    // comparison part
    std::ifstream generatedStream;
    std::ifstream expectedStream;

    FS::Files::openIFStream(generatedStream, outputFile.c_str());
    FS::Files::openIFStream(expectedStream, expectedFile.c_str());

    ASSERT_TRUE(generatedStream.is_open());
    ASSERT_TRUE(expectedStream.is_open());

    std::string generatedString;
    std::string expectedString;

    while (std::getline(expectedStream, expectedString)) {
        std::getline(generatedStream, generatedString);
        ASSERT_EQ(expectedString, generatedString);
    }

    // last check to verify the expected file has no additional line
    std::getline(generatedStream, generatedString);
}

TEST(XML_Writer_Test_Case, validateHandlerWriteToJapanesePath)
{
    std::string expectedFile = DATA_DIR "/日本語/ref_data_jp.xml";
    std::string outputFile   = TEMP_DIR "/オッペﾝヴィベ/日本語.xml";

    FS::Files::createPath(TEMP_DIR);
    ASSERT_TRUE(FS::Files::directoryExists(TEMP_DIR));

    FS::Files::removeAll(outputFile.c_str());
    ASSERT_FALSE(FS::Files::fileExists(outputFile.c_str()));

    XML::IXMLHandler* xmlHandler = XML::createXMLHandler();
    std::string testData         = "<Document name=\"日本語\"><Node>日本語 1</Node><Node>日本語 2</Node><Node>日本語 3</Node></Document>";
    XML::IXMLNode* rootNode      = xmlHandler->parseString(testData.c_str(), testData.size());
    xmlHandler->writeXMLInFile(*rootNode, outputFile.c_str());

    // comparison part
    std::ifstream generatedStream;
    std::ifstream expectedStream;

    FS::Files::openIFStream(generatedStream, outputFile.c_str());
    FS::Files::openIFStream(expectedStream, expectedFile.c_str());

    ASSERT_TRUE(generatedStream.is_open());
    ASSERT_TRUE(expectedStream.is_open());

    std::string generatedString;
    std::string expectedString;

    while (std::getline(expectedStream, expectedString)) {
        std::getline(generatedStream, generatedString);
        ASSERT_EQ(expectedString, generatedString);
    }

    // last check to verify the expected file has no additional line
    std::getline(generatedStream, generatedString);
}

int uoXMLWriterTest(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);

    ::testing::GTEST_FLAG(filter) = "XML_Writer_Test_Case.*";
    return RUN_ALL_TESTS();
}
