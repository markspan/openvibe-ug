/*
* HID driver for OpenViBE
*
* \authors (NeuroServo, NeuroTechX)
* \developer (Innocent Aguié)
* \date Wed Nov 23 00:24:00 2016
*
*/

#if defined TARGET_OS_Windows
#if defined TARGET_HAS_ThirdPartyNeuroServo

#include <windows.h>

#include "HidDeviceNotifier.h"
#include "HidDeviceNotifierRef.h"
#include <thread>

#define WND_CLASS_NAME TEXT("SampleAppWindowClass")


HidDeviceNotifier::HidDeviceNotifier(const GUID interfaceGuid, CHAR* devicePathName)
{
	_deviceGuid         = interfaceGuid;
	_devicePathName     = devicePathName;
	_appName            = "Window Notifier";
	_isDeviceRegistered = false;
}

void HidDeviceNotifier::startRegistration()
{
	_isDeviceRegistered = true;

	// Start a separate thread for reading data from the device
	std::thread notifierThread(registerNotification, this);
	notifierThread.detach();
}

#endif

#endif // TARGET_OS_Windows
