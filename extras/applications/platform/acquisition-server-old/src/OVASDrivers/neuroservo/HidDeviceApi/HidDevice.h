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
#include "Hidsdi.h"
#include <thread>

#include "HidDeviceNotifier.h"

class HidDevice
{
public:
    HidDevice(unsigned short vendorID, unsigned short productID, unsigned short receivdedDataSize);
    HidDevice();
    ~HidDevice();

    /* Client available methods */
    bool connect();
    bool writeToDevice(BYTE data[], int nbOfBytes);
    bool isDeviceConnected() { return _isDeviceConnected; }
    void setHidDeviceInfos(unsigned short vendorID, unsigned short productID, unsigned short receivdedDataSize);

    /* Callbacks methods */
    std::function<void(BYTE [])> dataReceived;
    std::function<void()> deviceConnected;
    std::function<void()> deviceDetached;
    std::function<void()> deviceAttached;


private:
    /* Methods to perform specific actions */
    bool configure();
    bool getReadWriteHandle();
    bool getCapabilities();
    void deviceIsConnected();
    void readThread();

    /* Callbacks from Register Device Notification */
    void deviceOnAttached();
    void deviceOnDetached();

    /* Instances */

    // Device identification
    unsigned short _vendorID      = 0;
    unsigned short _productID     = 0;
    unsigned short _versionNumber = 0;
    GUID _hidGui;

    // Device connexion state
    bool _isDeviceAttached  = false;
    bool _isDeviceConnected = false;

    // Device communication handle
    HANDLE _deviceHandle;
    HANDLE _readDeviceHandle;
    HANDLE _writeDeviceHandle;
    CHAR* _devicePathName = nullptr;

    // Device capabilities
    HIDP_CAPS _deviceCaps;

    // Background worker
    std::thread _readTask;

    // To be able to read the data
    BYTE* _readData = nullptr;
    unsigned short _readDataSize;

    // Device notifier
    HidDeviceNotifier* _deviceNotifier = nullptr;

    HANDLE _hEventObject;
    OVERLAPPED _hidOverlapped;
};

#endif
#endif
