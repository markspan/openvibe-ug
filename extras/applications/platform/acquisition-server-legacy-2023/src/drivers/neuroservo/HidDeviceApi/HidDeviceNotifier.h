/*
* HID driver for OpenViBE
*
* \authors (NeuroServo, NeuroTechX)
* \developer (Innocent Aguie)
* \date Wed Nov 23 00:24:00 2016
*
*/
#pragma once

#if defined TARGET_OS_Windows
#if defined TARGET_HAS_ThirdPartyNeuroServo

#include <Windows.h>
#include <functional>

class HidDeviceNotifier
{
public:
    HidDeviceNotifier(GUID interfaceGuid, CHAR* devicePathName);
    HidDeviceNotifier() { }
    ~HidDeviceNotifier() { }

    void startRegistration();

    /* Callbacks methods */
    std::function<void()> deviceDetached;
    std::function<void()> deviceAttached;

    /* Member functions */
    GUID getDeviceGuid() const { return _deviceGuid; }
    CHAR* getDevicePathName() const { return _devicePathName; }
    LPCSTR getAppName() const { return _appName; }
    bool isDeviceRegistrationStarted() const { return _isDeviceRegistered; }


private:
    /* Members */
    // Device related information
    GUID _deviceGuid;
    CHAR* _devicePathName = nullptr;

    // For informational messages and window titles
    LPCSTR _appName;

    bool _isDeviceRegistered = false;
};

#endif
#endif
