#pragma once

#include "IReader.h"

namespace EBML {

// ________________________________________________________________________________________________________________
//

template <class TOwnerClass>
class TReaderCallbackProxy1 final : public IReaderCallback
{
public:
    TReaderCallbackProxy1(TOwnerClass& ownerObject, bool (TOwnerClass::*mfpIsMasterChild)(const CIdentifier& identifier),
                          void (TOwnerClass::*mfpOpenChild)(const CIdentifier& identifier),
                          void (TOwnerClass::*mfpProcessChildData)(const void* buffer, const size_t size), void (TOwnerClass::*mfpCloseChild)())
        : m_ownerObject(ownerObject), m_mfpIsMasterChild(mfpIsMasterChild), m_mfpOpenChild(mfpOpenChild), m_mfpProcessChildData(mfpProcessChildData),
          m_mfpCloseChild(mfpCloseChild) { }

    bool isMasterChild(const CIdentifier& identifier) override
    {
        if (m_mfpIsMasterChild) { return (m_ownerObject.*m_mfpIsMasterChild)(identifier); }
        return false;
    }

    void openChild(const CIdentifier& identifier) override { if (m_mfpOpenChild) { (m_ownerObject.*m_mfpOpenChild)(identifier); } }

    void processChildData(const void* buffer, const size_t size) override
    {
        if (m_mfpProcessChildData) { (m_ownerObject.*m_mfpProcessChildData)(buffer, size); }
    }

    void closeChild() override { if (m_mfpCloseChild) { (m_ownerObject.*m_mfpCloseChild)(); } }

protected:
    TOwnerClass& m_ownerObject;
    bool (TOwnerClass::*m_mfpIsMasterChild)(const CIdentifier& identifier);
    void (TOwnerClass::*m_mfpOpenChild)(const CIdentifier& identifier);
    void (TOwnerClass::*m_mfpProcessChildData)(const void* buffer, size_t size);
    void (TOwnerClass::*m_mfpCloseChild)();
};

// ________________________________________________________________________________________________________________
//

template <class TOwnerClass, bool (TOwnerClass::*TMfpIsMasterChild)(const CIdentifier& identifier),
          void (TOwnerClass::*TMfpOpenChild)(const CIdentifier& identifier),
          void (TOwnerClass::*TMfpProcessChildData)(const void* buffer, size_t size), void (TOwnerClass::*TMfpCloseChild)()>
class TReaderCallbackProxy2 final : public IReaderCallback
{
public:
    TReaderCallbackProxy2(TOwnerClass& ownerObject)
        : m_ownerObject(ownerObject), m_mfpIsMasterChild(TMfpIsMasterChild), m_mfpOpenChild(TMfpOpenChild), m_mfpProcessChildData(TMfpProcessChildData),
          m_mfpCloseChild(TMfpCloseChild) { }

    bool isMasterChild(const CIdentifier& identifier) override
    {
        if (m_mfpIsMasterChild) { return (m_ownerObject.*m_mfpIsMasterChild)(identifier); }
        return false;
    }

    void openChild(const CIdentifier& identifier) override { if (m_mfpOpenChild) { (m_ownerObject.*m_mfpOpenChild)(identifier); } }

    void processChildData(const void* buffer, const size_t size) override
    {
        if (m_mfpProcessChildData) { (m_ownerObject.*m_mfpProcessChildData)(buffer, size); }
    }

    void closeChild() override { if (m_mfpCloseChild) { (m_ownerObject.*m_mfpCloseChild)(); } }

protected:
    TOwnerClass& m_ownerObject;
    bool (TOwnerClass::*m_mfpIsMasterChild)(const CIdentifier& identifier);
    void (TOwnerClass::*m_mfpOpenChild)(const CIdentifier& identifier);
    void (TOwnerClass::*m_mfpProcessChildData)(const void* buffer, size_t size);
    void (TOwnerClass::*m_mfpCloseChild)();
};

// ________________________________________________________________________________________________________________
//
}  // namespace EBML
