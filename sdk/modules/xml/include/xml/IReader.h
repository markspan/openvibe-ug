#pragma once

#include "defines.h"
#include <cstdlib>  // fix Unix compatibility

namespace XML {
class XML_API IReaderCallback
{
public:
    virtual ~IReaderCallback() { }
    virtual void openChild(const char* name, const char** attributeName, const char** attributeValue, const size_t nAttribute) = 0;
    virtual void processChildData(const char* data) = 0;
    virtual void closeChild() = 0;
};

class XML_API IReaderCallBack : public IReaderCallback { };

class XML_API IReader
{
public:
    virtual bool processData(const void* buffer, const size_t size) = 0;
    virtual void release() = 0;
protected:
    virtual ~IReader() { }
};

extern XML_API IReader* createReader(IReaderCallback& callback);
}  // namespace XML
