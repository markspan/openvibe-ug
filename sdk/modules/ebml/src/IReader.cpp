#include "ebml/IReader.h"
#include <cstring>

#include <cstdio>
#include <cstring>
#include <iostream>

#if 0
#define _Debug_ _is_in_debug_mode_(m_totalBytes)
static bool _is_in_debug_mode_(uint64_t value)
{
    static int i=0;
    // bool result=i++>5500000;
    bool result=value>29605500;
    if (result) std::cout << "Arround " << value << std::endl;
    return result;

}
#else
#define _Debug_ false
#endif

// ________________________________________________________________________________________________________________
//

inline bool needsTwoBytesToGetCodedSizeLength(const unsigned char* buffer) { return buffer[0] == 0; }

inline size_t getCodedSizeLength(const unsigned char* buffer)
{
    if (buffer[0] >> 7) { return 1; }
    if (buffer[0] >> 6) { return 2; }
    if (buffer[0] >> 5) { return 3; }
    if (buffer[0] >> 4) { return 4; }
    if (buffer[0] >> 3) { return 5; }
    if (buffer[0] >> 2) { return 6; }
    if (buffer[0] >> 1) { return 7; }
    if (buffer[0]) { return 8; }
    if (buffer[1] >> 7) { return 9; }
    return 10;
}

inline uint64_t getValue(unsigned char* buffer)
{
    uint64_t result     = 0;
    const size_t length = getCodedSizeLength(buffer);
    size_t ithBit       = length;
    for (size_t i = 0; i < length; ++i)
    {
        result = (result << 8) + (buffer[i]);
        result &= ~(ithBit > 0 && ithBit <= 8 ? (1 << (8 - ithBit)) : 0);
        ithBit -= 8;
    }
    return result;
}

// ________________________________________________________________________________________________________________
//

namespace EBML {
namespace {
class CReaderNode
{
public:
    CReaderNode(const CIdentifier& identifier, CReaderNode* parentNode) : m_ParentNode(parentNode), m_Id(identifier) { }

private:
    CReaderNode() = delete;

public:

    CReaderNode* m_ParentNode = nullptr;
    CIdentifier m_Id;
    size_t m_ContentSize     = 0;
    size_t m_ReadContentSize = 0;
    unsigned char* m_Buffer  = nullptr;
};
}  // namespace
}  // namespace EBML

// ________________________________________________________________________________________________________________
//

namespace EBML {
namespace {
class CReader final : public IReader
{
public:

    explicit CReader(IReaderCallback& callback) : m_readerCB(callback) { }
    ~CReader() override;
    bool processData(const void* buffer, const size_t size) override;
    CIdentifier getCurrentNodeID() const override;
    size_t getCurrentNodeSize() const override;
    void release() override;

protected:

    enum EStatus
    {
        FillingIdentifier,
        FillingContentSize,
        FillingContent,
    };

    IReaderCallback& m_readerCB;
    CReaderNode* m_currentNode  = nullptr;
    size_t m_pendingSize        = 0;
    size_t m_nPending           = 0;
    unsigned char* m_pending    = nullptr;
    EStatus m_status            = FillingIdentifier;
    EStatus m_lastStatus        = FillingIdentifier;
    CIdentifier m_currentID     = 0;
    size_t m_currentContentSize = 0;
    size_t m_totalBytes         = 0;
};
}  // namespace

// ________________________________________________________________________________________________________________
//

CReader::~CReader()
{
    delete [] m_pending;
    while (m_currentNode)
    {
        CReaderNode* parentNode = m_currentNode->m_ParentNode;
        delete m_currentNode;
        m_currentNode = parentNode;
    }
}

bool CReader::processData(const void* buffer, const size_t size)
{
    m_totalBytes += size;

    if (_Debug_)
    {
        std::cout << "Received " << size << " byte(s) new buffer :";
        for (int i = 0; i < int(size) /* && i<4*/; ++i) { printf("[%02X]", ((unsigned char*)buffer)[i]); }
        std::cout << "...\n";
    }

    if (!buffer || !size) { return true; }

    unsigned char* tmpBuffer = (unsigned char*)buffer;
    size_t currentSize       = size;
    bool finished            = false;
    while (!finished)
    {
        size_t processedPendingBytes = 0;
        size_t processedBytes        = 0;
        m_lastStatus                 = m_status;

        if (_Debug_ && m_nPending)
        {
            std::cout << m_nPending << " byte(s) pending : ";
            for (int i = 0; i < int(m_nPending); ++i) { printf("[%02X]", m_pending[i]); }
            std::cout << "\n";
        }

        // Processes data
        switch (m_status)
        {
            case FillingIdentifier:
            case FillingContentSize:
            {
                if (needsTwoBytesToGetCodedSizeLength(m_nPending ? m_pending : tmpBuffer))
                {
                    if (m_nPending + currentSize < 2)
                    {
                        finished = true;
                        break;
                    }

                    if (m_nPending == 1)
                    {
                        // assumes (currentSize != 0) because (m_nPending + currentSize >= 2) and (m_nPending == 1)
                        m_pending[1] = tmpBuffer[0];
                        tmpBuffer++;
                        m_nPending++;
                        currentSize--;
                    }
                }

                const size_t length = getCodedSizeLength(m_nPending ? m_pending : tmpBuffer);
                if (length > currentSize + m_nPending) { finished = true; }
                else
                {
                    unsigned char* encodedBuffer    = new unsigned char[length];
                    const size_t pendingBytesToCopy = (length > m_nPending ? m_nPending : length);
                    memcpy(encodedBuffer, m_pending, size_t(pendingBytesToCopy));
                    memcpy(encodedBuffer + pendingBytesToCopy, tmpBuffer, size_t(length - pendingBytesToCopy));
                    const uint64_t value = getValue(encodedBuffer);
                    delete [] encodedBuffer;
                    processedPendingBytes = pendingBytesToCopy;
                    processedBytes        = length;

                    switch (m_status)
                    {
                        case FillingIdentifier:
                        {
                            m_currentID = value;
                            m_status    = FillingContentSize;
                            if (_Debug_)
                            {
                                printf("Found identifier 0x%llX - Changing status to FillingContentSize...\n", static_cast<unsigned long long>(m_currentID));
                            }
                        }
                        break;

                        case FillingContentSize:
                        {
                            m_currentContentSize = value;
                            if (m_readerCB.isMasterChild(m_currentID))
                            {
                                m_status = FillingIdentifier;
                                if (_Debug_)
                                {
                                    std::cout << "Found content size " << m_currentContentSize << " of master node - Changing status to FillingIdentifier...\n";
                                }
                            }
                            else
                            {
                                m_status = FillingContent;
                                if (_Debug_)
                                {
                                    std::cout << "Found content size " << m_currentContentSize <<
                                            " of *non* master node - Changing status to FillingContent...\n";
                                }
                            }
                        }
                        break;

                        case FillingContent:
                            // Should never happen - avoids the warning
                            break;
                    }
                }
            }
            break;

            case FillingContent:
            {
                if (m_currentNode->m_ContentSize == 0)
                {
                    m_status = FillingIdentifier;
                    if (_Debug_)
                    {
                        std::cout << "Finished with " << m_currentNode->m_ContentSize << " byte(s) content - Changing status to FillingIdentifier...\n";
                    }
                    m_readerCB.processChildData(nullptr, 0);
                }
                else
                {
                    if (m_currentNode->m_ReadContentSize == 0 && m_currentNode->m_ContentSize <= currentSize)
                    {
                        m_status = FillingIdentifier;

                        processedBytes = m_currentNode->m_ContentSize;
                        if (_Debug_)
                        {
                            std::cout << "Optimized processing of " << m_currentNode->m_ContentSize <<
                                    " byte(s) content - Changing status to FillingIdentifier...\n";
                        }
                        m_readerCB.processChildData(tmpBuffer, m_currentNode->m_ContentSize);
                    }
                    else
                    {
                        if (m_currentNode->m_ContentSize - m_currentNode->m_ReadContentSize > currentSize)
                        {
                            memcpy(m_currentNode->m_Buffer + m_currentNode->m_ReadContentSize, tmpBuffer, size_t(currentSize));
                            processedBytes = currentSize;
                            finished       = true;
                        }
                        else
                        {
                            memcpy(m_currentNode->m_Buffer + m_currentNode->m_ReadContentSize, tmpBuffer,
                                   size_t(m_currentNode->m_ContentSize - m_currentNode->m_ReadContentSize));
                            processedBytes = m_currentNode->m_ContentSize - m_currentNode->m_ReadContentSize;

                            m_status = FillingIdentifier;
                            if (_Debug_)
                            {
                                std::cout << "Finished with " << m_currentNode->m_ContentSize << " byte(s) content - Changing status to FillingIdentifier...\n";
                            }
                            m_readerCB.processChildData(m_currentNode->m_Buffer, m_currentNode->m_ContentSize);
                        }
                    }
                }
            }
            break;
        }

        // Updates buffer pointer and size
        const size_t processedBytesInBuffer = processedBytes - processedPendingBytes;
        tmpBuffer += processedBytesInBuffer;
        currentSize -= processedBytesInBuffer;
        m_nPending -= processedPendingBytes;

        // Updates read size
        CReaderNode* node = m_currentNode;
        while (node)
        {
            node->m_ReadContentSize += processedBytes;
            node = node->m_ParentNode;
        }

        // Creates new node when needed
        if (m_status != FillingContentSize && m_lastStatus == FillingContentSize)
        {
            m_currentNode                = new CReaderNode(m_currentID, m_currentNode);
            m_currentNode->m_ContentSize = m_currentContentSize;
            m_currentNode->m_Buffer      = new unsigned char[m_currentContentSize];
            m_readerCB.openChild(m_currentNode->m_Id);
        }
        else
        {
            // Closes finished nodes
            while (m_currentNode && (m_currentNode->m_ContentSize == m_currentNode->m_ReadContentSize || m_currentNode->m_ContentSize == 0))
            {
                m_readerCB.closeChild();
                CReaderNode* parent = m_currentNode->m_ParentNode;
                delete [] m_currentNode->m_Buffer;
                delete m_currentNode;
                m_currentNode = parent;
            }
        }
    }

    // Updates pending data
    if (m_nPending + currentSize > m_pendingSize)
    {
        unsigned char* pending = new unsigned char[m_nPending + currentSize + 1
        ]; // Ugly hack, reserve 1 more byte on pending data so we are sure we can insert this additional pending byte when only one byte is pending and two bytes are needed for decoding identifier and/or buffer size
        memcpy(pending, m_pending, m_nPending);
        delete [] m_pending;
        m_pending     = pending;
        m_pendingSize = m_nPending + currentSize;
    }
    memcpy(m_pending + m_nPending, tmpBuffer, currentSize);
    m_nPending += currentSize;

    if (_Debug_) { std::cout << "\n"; }
    return true;
}

CIdentifier CReader::getCurrentNodeID() const { return m_currentNode ? m_currentNode->m_Id : CIdentifier(); }
size_t CReader::getCurrentNodeSize() const { return m_currentNode ? m_currentNode->m_ContentSize : 0; }

void CReader::release() { delete this; }

// ________________________________________________________________________________________________________________
//

EBML_API IReader* createReader(IReaderCallback& callback) { return new CReader(callback); }

}  // namespace EBML
