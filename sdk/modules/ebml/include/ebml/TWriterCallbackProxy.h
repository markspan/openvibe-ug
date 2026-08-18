#pragma once

#include "IWriter.h"

namespace EBML {

// ________________________________________________________________________________________________________________
//

template <class TOwnerClass>
class TWriterCallbackProxy1 final : public IWriterCallback
{
public:
    TWriterCallbackProxy1(TOwnerClass& ownerObject, void (TOwnerClass::*mfpWrite)(const void* buffer, size_t size))
        : m_ownerObject(ownerObject), m_mfpWrite(mfpWrite) { }

    void write(const void* buffer, const size_t size) override { if (m_mfpWrite) { (m_ownerObject.*m_mfpWrite)(buffer, size); } }

protected:
    TOwnerClass& m_ownerObject;
    void (TOwnerClass::*m_mfpWrite)(const void* buffer, size_t size);
};

// ________________________________________________________________________________________________________________
//

template <class TOwnerClass, void (TOwnerClass::*TMfpWrite)(const void* buffer, size_t size)>
class TWriterCallbackProxy2 final : public IWriterCallback
{
public:
    TWriterCallbackProxy2(TOwnerClass& ownerObject) : m_ownerObject(ownerObject), m_mfpWrite(TMfpWrite) { }

    void write(const void* buffer, const size_t size) override { if (m_mfpWrite) { (m_ownerObject.*m_mfpWrite)(buffer, size); } }

protected:
    TOwnerClass& m_ownerObject;
    void (TOwnerClass::*m_mfpWrite)(const void* buffer, size_t size);
};

// ________________________________________________________________________________________________________________
//
}  // namespace EBML
