#pragma once

#include "TConnectionDelegate.h"
#include "IConnectionSerial.h"

namespace Socket {
struct Socket_API SConnectionSerialDelegate
{
    SConnectionDelegate connectionDelegate;
    bool (*fpConnect)(void*, const char*, const size_t);
    size_t (*fpGetPendingByteCount)(void*);
    bool (*fpFlush)(void*);
    const char* (*fpGetLastError)(void*);
    void (*fpSaveLastError)(void*);
    // TODO for Android compatibility
    //bool(*fpIsErrorRaised)(void*); 
    //void(*fpClearError)(void*);
    //bool(*fSetTimeouts)(void*, const size_t decisecondsTimeout);
};

class Socket_API IConnectionSerialDelegate : public TConnectionDelegate<IConnectionSerial>
{
public:
    IConnectionSerialDelegate(const SConnectionSerialDelegate connectionSerialDelegate)
        : TConnectionDelegate<IConnectionSerial>(connectionSerialDelegate.connectionDelegate) { }

    ~IConnectionSerialDelegate() override { }

protected:
    SConnectionSerialDelegate m_connectionSerialDelegate;
};

extern Socket_API IConnectionSerialDelegate* createConnectionSerialDelegate(SConnectionSerialDelegate connectionSerialDelegate);
}  // namespace Socket
