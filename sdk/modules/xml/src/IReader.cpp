#include "IReader.h"

#include <expat.h>
#include <string>
#include <iostream>

namespace XML {
class CReader final : public IReader
{
public:
    explicit CReader(IReaderCallback& callback);
    bool processData(const void* buffer, const size_t size) override;
    void release() override;

    void openChild(const char* name, const char** sAttributeName, const char** sAttributeValue, const size_t nAttribute);
    void processChildData(const char* data);
    void closeChild();

protected:

    IReaderCallback& m_callback;
    XML_Parser m_pXMLParser;
    std::string m_data;
};

static void XMLCALL ExpatXMLStart(void* data, const char* element, const char** attribute);
static void XMLCALL ExpatXMLEnd(void* data, const char* element);
static void XMLCALL ExpatXMLData(void* data, const char* value, int length);

CReader::CReader(IReaderCallback& callback) : m_callback(callback), m_pXMLParser(nullptr)
{
    m_pXMLParser = XML_ParserCreate(nullptr);
    XML_SetElementHandler(m_pXMLParser, ExpatXMLStart, ExpatXMLEnd);
    XML_SetCharacterDataHandler(m_pXMLParser, ExpatXMLData);
    XML_SetUserData(m_pXMLParser, this);
}

bool CReader::processData(const void* buffer, const size_t size)
{
    // $$$ TODO take 64bits size into consideration
    const XML_Status status = XML_Parse(m_pXMLParser, static_cast<const char*>(buffer), static_cast<const int>(size), false);
    if (status != XML_STATUS_OK)
    {
        const XML_Error error = XML_GetErrorCode(m_pXMLParser);
        // Although printf() is not too elegant, this component has no context to use e.g. LogManager -> printf() is better than a silent fail.
        std::cout << "processData(): expat error " << error << " on the line " << XML_GetCurrentLineNumber(m_pXMLParser) << " of the input .xml\n";
    }

    return (status == XML_STATUS_OK);
}

void CReader::release()
{
    XML_ParserFree(m_pXMLParser);
    delete this;
}

void CReader::openChild(const char* name, const char** sAttributeName, const char** sAttributeValue, const size_t nAttribute)
{
    m_callback.openChild(name, sAttributeName, sAttributeValue, nAttribute);
    m_data = "";
}

void CReader::processChildData(const char* data) { m_data += data; }

void CReader::closeChild()
{
    if (m_data.size() != 0) { m_callback.processChildData(m_data.c_str()); }
    m_data = "";
    m_callback.closeChild();
}

XML_API IReader* createReader(IReaderCallback& callback) { return new CReader(callback); }

static void XMLCALL ExpatXMLStart(void* data, const char* element, const char** attribute)
{
    size_t nAttribute = 0;
    while (attribute[nAttribute++]) { }
    nAttribute >>= 1;

    // $$$ TODO take 64bits size into consideration
    const char** name  = new const char*[nAttribute];
    const char** value = new const char*[nAttribute];

    for (size_t i = 0; i < nAttribute; ++i)
    {
        name[i]  = attribute[(i << 1)];
        value[i] = attribute[(i << 1) + 1];
    }

    static_cast<CReader*>(data)->openChild(element, name, value, nAttribute);

    delete [] name;
    delete [] value;
}

static void XMLCALL ExpatXMLEnd(void* data, const char* /*element*/) { static_cast<CReader*>(data)->closeChild(); }

static void XMLCALL ExpatXMLData(void* data, const char* value, const int length)
{
    const std::string str(value, length);
    static_cast<CReader*>(data)->processChildData(str.c_str());
}

}  // namespace XML
