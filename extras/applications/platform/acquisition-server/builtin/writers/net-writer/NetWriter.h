#pragma once

#include <AbstractWriterBuilder.h>

#include <memory>

class NetWriterPrivate;
class NetWriter : public AbstractWriter
{
public:
    NetWriter(const std::string& writerName);
    virtual ~NetWriter();

    std::pair<size_t, void*> sendCallback(void* data, uint64_t clientId);

protected:
    virtual size_t send(ovMsg* msg, uint64_t clientId = -1) override; ///< Send a message to the OPENVIBE.

private:
    std::unique_ptr<NetWriterPrivate> d;
};

REGISTER_WRITER(NetWriter, "Net Writer");
