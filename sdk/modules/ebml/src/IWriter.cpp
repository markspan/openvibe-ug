#include "ebml/IWriter.h"

#include <vector>
#include <cstring>

inline size_t getCodedSizeLength(const uint64_t value)
{
    if (value < 0x000000000000007fLL) { return 1; }
    if (value < 0x0000000000003fffLL) { return 2; }
    if (value < 0x00000000001fffffLL) { return 3; }
    if (value < 0x000000000fffffffLL) { return 4; }
    if (value < 0x00000007ffffffffLL) { return 5; }
    if (value < 0x000003ffffffffffLL) { return 6; }
    if (value < 0x0001ffffffffffffLL) { return 7; }
    if (value < 0x00ffffffffffffffLL) { return 8; }
    if (value < 0x7fffffffffffffffLL) { return 9; }
    return 10;
}

inline bool getCodedBuffer(const uint64_t value, unsigned char* buffer, size_t* size)
{
    const size_t length = getCodedSizeLength(value);

    if (length > *size) { return false; }

    size_t bit = length;
    for (size_t i = 0; i < length; ++i)
    {
        const size_t byteShift = length - i - 1;
        size_t byte            = (byteShift >= 8 ? 0 : static_cast<unsigned char>((value >> (byteShift * 8)) & 0xff));
        byte |= (bit > 0 && bit <= 8 ? (1 << (8 - bit)) : 0);
        bit -= 8;

        buffer[i] = static_cast<unsigned char>(byte);
    }

    *size = length;
    return true;
}

// ________________________________________________________________________________________________________________
//

namespace EBML {
namespace {
class CWriterNode final
{
public:

    CWriterNode(const CIdentifier& identifier, CWriterNode* parentNode) : m_ID(identifier), m_ParentNode(parentNode) {}
    ~CWriterNode();
    void process(IWriterCallback& callback);

protected:

    size_t getTotalContentSize(bool identifierAndSize);

private:

    CWriterNode() = delete;

public:

    CIdentifier m_ID;
    CWriterNode* m_ParentNode = nullptr;
    size_t m_BufferLength     = 0;
    unsigned char* m_Buffer   = nullptr;
    bool m_Buffered           = false;
    std::vector<CWriterNode*> m_Childrens;
};
}  // namespace

// ________________________________________________________________________________________________________________
//

CWriterNode::~CWriterNode()
{
    for (auto i = m_Childrens.begin(); i != m_Childrens.end(); ++i) { delete (*i); }

    if (m_Buffer)
    {
        delete [] m_Buffer;
        m_Buffer = nullptr;
    }
}

void CWriterNode::process(IWriterCallback& callback)
{
    unsigned char id[10];
    unsigned char pContentSize[10];
    size_t contentSizeLength = sizeof(pContentSize);
    size_t identifierLength  = sizeof(id);
    const size_t contentSize = getTotalContentSize(false);

    if (!getCodedBuffer(contentSize, pContentSize, &contentSizeLength)) { } // SHOULD NEVER HAPPEN
    if (!getCodedBuffer(m_ID, id, &identifierLength)) { }   // SHOULD NEVER HAPPEN

    callback.write(id, identifierLength);
    callback.write(pContentSize, contentSizeLength);

    if (m_Childrens.empty()) { callback.write(m_Buffer, m_BufferLength); }
    else { for (auto i = m_Childrens.begin(); i != m_Childrens.end(); ++i) { (*i)->process(callback); } }
}

size_t CWriterNode::getTotalContentSize(const bool identifierAndSize)
{
    size_t contentSize = 0;
    if (m_Childrens.empty()) { contentSize = m_BufferLength; }
    else { for (auto i = m_Childrens.begin(); i != m_Childrens.end(); ++i) { contentSize += (*i)->getTotalContentSize(true); } }

    size_t res = contentSize;
    if (identifierAndSize)
    {
        res += getCodedSizeLength(m_ID);
        res += getCodedSizeLength(contentSize);
    }

    return res;
}

// ________________________________________________________________________________________________________________
//

namespace {
class CWriter final : public IWriter
{
public:

    explicit CWriter(IWriterCallback& callback) : m_callback(callback) {}
    bool openChild(const CIdentifier& identifier) override;
    bool setChildData(const void* buffer, const size_t size) override;
    bool closeChild() override;
    void release() override;

protected:

    CWriterNode* m_node = nullptr;
    IWriterCallback& m_callback;

private:
    CWriter() = delete;
};
}  // namespace

// ________________________________________________________________________________________________________________
//

bool CWriter::openChild(const CIdentifier& identifier)
{
    if (m_node) { if (m_node->m_Buffered) { return false; } }

    CWriterNode* pResult = new CWriterNode(identifier, m_node);
    if (m_node) { m_node->m_Childrens.push_back(pResult); }
    m_node = pResult;
    return true;
}

bool CWriter::setChildData(const void* buffer, const size_t size)
{
    if (!m_node) { return false; }

    if (!m_node->m_Childrens.empty()) { return false; }

    unsigned char* bufferCopy = nullptr;
    if (size)
    {
        if (!buffer) { return false; }
        bufferCopy = new unsigned char[size];
        if (!bufferCopy) { return false; }
        memcpy(bufferCopy, buffer, size);
    }

    delete [] m_node->m_Buffer;

    m_node->m_BufferLength = size;
    m_node->m_Buffer       = bufferCopy;
    m_node->m_Buffered     = true;
    return true;
}

bool CWriter::closeChild()
{
    if (!m_node) { return false; }

    if ((!m_node->m_Buffered) && (m_node->m_Childrens.empty()))
    {
        m_node->m_BufferLength = 0;
        m_node->m_Buffer       = nullptr;
        m_node->m_Buffered     = true;
    }

    CWriterNode* parent = m_node->m_ParentNode;
    if (!parent)
    {
        m_node->process(m_callback);
        delete m_node;
    }

    m_node = parent;
    return true;
}

void CWriter::release()
{
    while (m_node) { closeChild(); }
    delete this;
}

// ________________________________________________________________________________________________________________
//

EBML_API IWriter* createWriter(IWriterCallback& callback) { return new CWriter(callback); }

}  // namespace EBML
