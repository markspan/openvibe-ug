#pragma once

#include "defines.h"
#include <cinttypes>
#include <cstdlib>  // size_t for unix

namespace FS {
class IEntryEnumeratorCallBack;

class FS_API IEntryEnumerator
{
public:

    class FS_API IAttributes
    {
    public:
        virtual bool isFile() = 0;
        virtual bool isDirectory() = 0;
        virtual bool isSymbolicLink() = 0;

        virtual bool isArchive() = 0;
        virtual bool isReadOnly() = 0;
        virtual bool isHidden() = 0;
        virtual bool isSystem() = 0;
        virtual bool isExecutable() = 0;

        virtual size_t getSize() = 0;
    protected:
        virtual ~IAttributes() {}
    };

    class FS_API IEntry
    {
    public:
        virtual const char* getName() = 0;
    protected:
        virtual ~IEntry() {}
    };

    virtual bool enumerate(const char* sWildcard, bool bRecursive = false) = 0;
    virtual void release() = 0;
protected:
    virtual ~IEntryEnumerator() { }
};

class FS_API IEntryEnumeratorCallBack
{
public:
    virtual bool callback(IEntryEnumerator::IEntry& rEntry, IEntryEnumerator::IAttributes& rAttributes) = 0;
    virtual ~IEntryEnumeratorCallBack() { }
};

extern FS_API IEntryEnumerator* createEntryEnumerator(IEntryEnumeratorCallBack& rCallBack);
}  // namespace FS
