#include "IXMLHandler.h"

#include <expat.h>
#include <cstring>
#include <stack>
#include <fstream>
#include <sstream>
#include <fs/Files.h>

namespace XML {
class IXMLHandlerImpl final : public IXMLHandler
{
public:
    void release() override;
    IXMLHandlerImpl();

    //Parsing
    IXMLNode* parseFile(const char* sPath) override;
    IXMLNode* parseString(const char* str, const size_t& size) override;

    //XML extraction
    bool writeXMLInFile(const IXMLNode& rNode, const char* sPath) const override;

    //Error handling
    std::string getLastErrorString() const override;

    //Internal function for parsing
    void openChild(const char* name, const char** sAttributeName, const char** sAttributeValue, const size_t nAttribute);
    void processChildData(const char* data);
    void closeChild();

    std::stringstream& getErrorStringStream() const;

protected:
    ~IXMLHandlerImpl() override;

private:
    XML_Parser m_pXMLParser;
    std::stack<IXMLNode*> m_oNodeStack;
    IXMLNode* m_pRootNode;
    mutable std::stringstream m_ssErrorStringStream;
};

//Callback for expat
static void XMLCALL ExpatXMLStart(void* pData, const char* element, const char** attribute);
static void XMLCALL ExpatXMLEnd(void* data, const char* element);
static void XMLCALL ExpatXMLData(void* data, const char* value, int length);

IXMLHandlerImpl::~IXMLHandlerImpl()
{
    XML_ParserFree(m_pXMLParser);
    while (!m_oNodeStack.empty())
    {
        IXMLNode* node = m_oNodeStack.top();
        node->release();
        m_oNodeStack.pop();
    }
}

void IXMLHandlerImpl::release() { delete this; }


IXMLHandlerImpl::IXMLHandlerImpl(): m_pXMLParser(nullptr), m_pRootNode(nullptr)
{
    m_pXMLParser = XML_ParserCreate(nullptr);
    XML_SetElementHandler(m_pXMLParser, ExpatXMLStart, ExpatXMLEnd);
    XML_SetCharacterDataHandler(m_pXMLParser, ExpatXMLData);
    XML_SetUserData(m_pXMLParser, this);
}

IXMLNode* IXMLHandlerImpl::parseFile(const char* sPath)
{
    std::ifstream file;
    FS::Files::openIFStream(file, sPath, std::ios::binary);
    if (file.is_open())
    {
        //Compute size
        file.seekg(0, std::ios::end);
        const size_t fileLen = size_t(file.tellg());
        file.seekg(0, std::ios::beg);

        //Read the file
        char* buffer = new char[fileLen];
        file.read(buffer, fileLen);
        file.close();

        //Start the parsing with the other function
        IXMLNode* res = parseString(buffer, fileLen);

        delete[] buffer;
        return res;
    }
    this->getErrorStringStream() << "Error : unable to open the file" << sPath << "." << std::endl;
    return nullptr;
}

IXMLNode* IXMLHandlerImpl::parseString(const char* str, const size_t& size)
{
    m_pRootNode             = nullptr;
    const XML_Status status = XML_Parse(m_pXMLParser, str, int(size), false);

    //We delete what is still in the stack
    while (!m_oNodeStack.empty())
    {
        // std::cout << "Warning : the file has been parsed but some tags are not closed. The file is probably not well-formed." << std::endl;
        IXMLNode* node = m_oNodeStack.top();
        node->release();
        m_oNodeStack.pop();
    }
    if (status != XML_STATUS_OK)
    {
        const XML_Error code = XML_GetErrorCode(m_pXMLParser);
        // Although printf() is not too elegant, this component has no context to use e.g. LogManager -> printf() is better than a silent fail.
        this->getErrorStringStream() << "processData(): expat error " << code << " on the line " << XML_GetCurrentLineNumber(m_pXMLParser) <<
                " of the input .xml\n";
        return nullptr;
    }
    return m_pRootNode;
}

bool IXMLHandlerImpl::writeXMLInFile(const IXMLNode& rNode, const char* sPath) const
{
    std::ofstream file;
    FS::Files::openOFStream(file, sPath, std::ios::binary);
    if (file.is_open())
    {
        char* xml = rNode.getXML();
        file.write(xml, strlen(xml));
        file.close();
        free(xml);
        return true;
    }
    this->getErrorStringStream() << "Error : unable to open the file " << sPath << "." << std::endl;
    return false;
}

void IXMLHandlerImpl::openChild(const char* name, const char** sAttributeName, const char** sAttributeValue, const size_t nAttribute)
{
    IXMLNode* node = createNode(name);
    for (size_t i = 0; i < nAttribute; ++i) { node->addAttribute(sAttributeName[i], sAttributeValue[i]); }
    m_oNodeStack.push(node);
}

void IXMLHandlerImpl::processChildData(const char* data)
{
    IXMLNode* node = m_oNodeStack.top();
    node->appendPCData(data);
}

void IXMLHandlerImpl::closeChild()
{
    IXMLNode* node = m_oNodeStack.top();
    m_oNodeStack.pop();
    //If the stack is empty this means that node is the root
    if (m_oNodeStack.empty()) { m_pRootNode = node; }
    else
    {//If node, that means that the node if
        IXMLNode* parentNode = m_oNodeStack.top();
        parentNode->addChild(node);
    }
}

std::stringstream& IXMLHandlerImpl::getErrorStringStream() const
{
    // empty the string contents and clear the error flags
    m_ssErrorStringStream.str("");
    m_ssErrorStringStream.clear();
    return m_ssErrorStringStream;
}

std::string IXMLHandlerImpl::getLastErrorString() const { return m_ssErrorStringStream.str(); }

static void XMLCALL ExpatXMLStart(void* pData, const char* element, const char** attribute)
{
    size_t nAttribute = 0;
    while (attribute[nAttribute++]) { }
    nAttribute >>= 1;

    // $$$ TODO take 64bits size into consideration
    const char** name  = new const char*[size_t(nAttribute)];
    const char** value = new const char*[size_t(nAttribute)];

    for (size_t i = 0; i < nAttribute; ++i)
    {
        name[i]  = attribute[(i << 1)];
        value[i] = attribute[(i << 1) + 1];
    }

    static_cast<IXMLHandlerImpl*>(pData)->openChild(element, name, value, nAttribute);

    delete [] name;
    delete [] value;
}

static void XMLCALL ExpatXMLEnd(void* data, const char* /*element*/) { static_cast<IXMLHandlerImpl*>(data)->closeChild(); }

static void XMLCALL ExpatXMLData(void* data, const char* value, const int length)
{
    const std::string str(value, length);
    static_cast<IXMLHandlerImpl*>(data)->processChildData(str.c_str());
}

OV_API IXMLHandler* createXMLHandler() { return new IXMLHandlerImpl(); }

}  // namespace XML
