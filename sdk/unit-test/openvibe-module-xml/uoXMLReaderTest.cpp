///-------------------------------------------------------------------------------------------------
/// 
/// \file uoXMLReaderTest.cpp
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
#include <iomanip>
#include <vector>
#include <map>
#include <memory>

#include "xml/IReader.h"
#include "xml/IXMLHandler.h"
#include <fs/Files.h>

#include <gtest/gtest.h>

class CReaderCallBack final : public XML::IReaderCallBack
{
public:
    struct SNode
    {
        std::string name;
        std::string data;
        std::map<std::string, std::string> attributes;
        std::vector<std::shared_ptr<SNode>> children;
        std::shared_ptr<SNode> parent{ nullptr };
    };

    std::shared_ptr<SNode> m_CurrentNode{ nullptr };

protected:
    void openChild(const char* name, const char** attributeName, const char** attributeValue, const size_t nAttribute) override
    {
        const auto node = std::make_shared<SNode>();

        if (m_CurrentNode) {
            node->parent = m_CurrentNode;
            m_CurrentNode->children.push_back(node);
        }

        m_CurrentNode       = node;
        m_CurrentNode->name = name;

        for (size_t i = 0; i < nAttribute; ++i) { m_CurrentNode->attributes[attributeName[i]] = attributeValue[i]; }
    }

    void processChildData(const char* data) override { m_CurrentNode->data = data; }

    void closeChild() override { if (m_CurrentNode->parent) { m_CurrentNode = m_CurrentNode->parent; } }
};

TEST(XML_Reader_Test_Case, validateReader)
{
    std::string dataFile = std::string(DATA_DIR) + "/ref_data.xml";

    CReaderCallBack callback;
    XML::IReader* xmlReader = createReader(callback);

    FILE* inputTestDataFile = fopen(dataFile.c_str(), "r");

    ASSERT_NE(nullptr, inputTestDataFile);

    char dataBuffer[1024];
    while (!feof(inputTestDataFile)) {
        size_t length = std::fread(dataBuffer, 1, sizeof(dataBuffer), inputTestDataFile);
        xmlReader->processData(dataBuffer, length);
    }
    std::fclose(inputTestDataFile);

    xmlReader->release();

    // Analyze results

    auto rootNode = callback.m_CurrentNode;

    // Root node check
    ASSERT_EQ("Document", rootNode->name);
    ASSERT_EQ(1, rootNode->attributes.size());
    ASSERT_EQ("test_reference", rootNode->attributes["name"]);

    ASSERT_EQ(3, rootNode->children.size());

    auto complexChild = rootNode->children[0];
    auto dataChild    = rootNode->children[1];
    auto emptyChild   = rootNode->children[2];

    // Simple child with data check
    ASSERT_EQ("NodeWithData", dataChild->name);
    ASSERT_EQ("node data with special characters <>,;:!?./&\"'(-_)=~#{[|`\\^@]}/*-+", dataChild->data);
    ASSERT_EQ(1, dataChild->attributes.size());
    ASSERT_EQ("hasData", dataChild->attributes["status"]);

    // Simple child with no data and multiple attributes check
    ASSERT_EQ("NodeEmptyWithNumber666", emptyChild->name);
    ASSERT_EQ(2, emptyChild->attributes.size());
    ASSERT_EQ("noData", emptyChild->attributes["status"]);
    ASSERT_EQ("test", emptyChild->attributes["ref"]);

    // Complex child check
    ASSERT_EQ("NodeWithChildren", complexChild->name);
    ASSERT_TRUE(complexChild->attributes.empty());

    ASSERT_EQ(3, complexChild->children.size());

    dataChild    = complexChild->children[0];
    emptyChild   = complexChild->children[1];
    complexChild = complexChild->children[2];

    ASSERT_EQ("ChildNodeWithData", dataChild->name);
    ASSERT_EQ("child node data with more than 10 alphanumeric characters", dataChild->data);
    ASSERT_EQ(0, dataChild->attributes.size());

    ASSERT_EQ("ChildNodeEmpty", emptyChild->name);
    ASSERT_TRUE(emptyChild->attributes.empty());

    ASSERT_EQ("ChildNodeWithChildren", complexChild->name);
    ASSERT_TRUE(complexChild->attributes.empty());
    ASSERT_EQ(1, complexChild->children.size());
}

TEST(XML_Reader_Test_Case, validateHandlerReadJapanese)
{
    const std::string dataFile = std::string(DATA_DIR) + "/日本語/ref_data_jp.xml";

    XML::IXMLHandler* xmlHandler = XML::createXMLHandler();
    XML::IXMLNode* rootNode      = xmlHandler->parseFile(dataFile.c_str());

    ASSERT_NE(nullptr, rootNode);
    ASSERT_EQ(std::string("Document"), rootNode->getName());
    ASSERT_TRUE(rootNode->hasAttribute("name"));
    ASSERT_EQ(std::string("日本語"), rootNode->getAttribute("name"));
    ASSERT_EQ(3, rootNode->getChildCount());

    ASSERT_STREQ("日本語 1", rootNode->getChild(0)->getPCData());

    xmlHandler->release();
}

TEST(XML_Reader_Test_Case, validateHandlerReadFrench)
{
    const std::string dataFile = std::string(DATA_DIR) + "/Français/ref_data_fr.xml";

    XML::IXMLHandler* xmlHandler = XML::createXMLHandler();
    XML::IXMLNode* rootNode      = xmlHandler->parseFile(dataFile.c_str());

    ASSERT_NE(nullptr, rootNode);
    ASSERT_EQ(std::string("Document"), rootNode->getName());
    ASSERT_TRUE(rootNode->hasAttribute("name"));
    ASSERT_EQ(std::string("Français"), rootNode->getAttribute("name"));
    ASSERT_EQ(3, rootNode->getChildCount());

    ASSERT_STREQ("Français 1 (àèáéîôïç)", rootNode->getChild(0)->getPCData());

    xmlHandler->release();
}

TEST(XML_Reader_Test_Case, validateHandlerReadNBSP)
{
    const std::string dataFile = std::string(DATA_DIR) + "/NB\xC2\xA0SP/ref_data_nbsp.xml";

    XML::IXMLHandler* xmlHandler = XML::createXMLHandler();
    XML::IXMLNode* rootNode      = xmlHandler->parseFile(dataFile.c_str());

    ASSERT_NE(nullptr, rootNode);
    ASSERT_EQ(std::string("Document"), rootNode->getName());
    ASSERT_TRUE(rootNode->hasAttribute("name"));
    ASSERT_EQ(std::string("NB\xC2\xA0SP"), rootNode->getAttribute("name"));
    ASSERT_EQ(3, rootNode->getChildCount());

    ASSERT_STREQ("NB\xC2\xA0SP 1", rootNode->getChild(0)->getPCData());

    xmlHandler->release();
}

int uoXMLReaderTest(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);

    ::testing::GTEST_FLAG(filter) = "XML_Reader_Test_Case.*";
    return RUN_ALL_TESTS();
}
