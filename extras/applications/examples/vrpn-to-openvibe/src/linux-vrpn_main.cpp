#if !defined(WIN32)

#include "GenericVRPNServer.hpp"
#include <iostream>
#include <cmath>
#include <unistd.h>

#define DEFAULT_PORT 50555

int main(int argc, char** argv)
{
    CGenericVrpnServer* vrpnServer = CGenericVrpnServer::GetInstance(DEFAULT_PORT);

    const char* buttonDevice = "button_test";
    const char* analogDevice = "analog_test";

    std::cout << "Creating devices [" << buttonDevice << "] and [" << analogDevice << "] using port [" << DEFAULT_PORT << "]\n";

    vrpnServer->AddButton(buttonDevice, 1);
    vrpnServer->AddAnalog(analogDevice, 2);

    double time   = 0;
    double period = 0;

    while (true) {
        if (period >= 2 * M_PI) {
            vrpnServer->ChangeButtonState(buttonDevice, 0, 1 - vrpnServer->GetButtonState(buttonDevice, 0));
            period = 0;
        }

        vrpnServer->ChangeAnalogState(analogDevice, sin(time), cos(time));

        time   = time + 0.01;
        period = period + 0.01;

        vrpnServer->Loop();

        // sleep for 10 miliseconds (on Unix)
        usleep(10000);
    }

    CGenericVrpnServer::DeleteInstance();
    vrpnServer = nullptr;

    return 0;
}

#endif // !WIN32
