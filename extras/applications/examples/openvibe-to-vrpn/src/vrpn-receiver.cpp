/*
 * Receives data from OpenViBE's VRPN boxes
 *
 * See here: http://openvibe.inria.fr/vrpn-tutorial-sending-data-from-openvibe-to-an-external-application/
 *
 */

#include <iostream>

#include <vrpn_Button.h>
#include <vrpn_Analog.h>

void VRPN_CALLBACK VrpnButtonCallback(void* userData, vrpn_BUTTONCB button)
{
    std::cout << "Button ID : " << button.button << " / Button State : " << button.state << std::endl;

    if (button.button == 1) {
        std::cout << "Quit requested by button press" << std::endl;
        *static_cast<bool*>(userData) = false;
    }
}

void VRPN_CALLBACK VrpnAnalogCallback(void* /*user_data*/, vrpn_ANALOGCB analog)
{
    for (int i = 0; i < analog.num_channel; ++i) { std::cout << "Analog Channel : " << i << " / Analog Value : " << analog.channel[i] << std::endl; }
}

int main(const int argc, char** argv)
{
    if (argc != 1 && argc != 3) {
        std::cout << "Usage:\n\n" << argv[0] << " [buttonDevice] [analogDevice]\n";
        return 1;
    }

    const char* buttonDevice = "openvibe_vrpn_button@localhost";
    const char* analogDevice = "openvibe_vrpn_analog@localhost";

    if (argc == 3) {
        buttonDevice = argv[1];
        analogDevice = argv[2];
    }

    std::cout << "Polling these VRPN devices\n  Button: " << buttonDevice << "\n  Analog: " << analogDevice << "\n";

    /* flag used to stop the program execution */
    bool running = true;

    /* Binding of the VRPN Button to a callback */
    vrpn_Button_Remote* vrpnButton = new vrpn_Button_Remote(buttonDevice);
    vrpnButton->register_change_handler(&running, VrpnButtonCallback);

    /* Binding of the VRPN Analog to a callback */
    vrpn_Analog_Remote* vrpnAnalog = new vrpn_Analog_Remote(analogDevice);
    vrpnAnalog->register_change_handler(nullptr, VrpnAnalogCallback);

    /* The main loop of the program, each VRPN object must be called in order to process data */
    while (running) { vrpnButton->mainloop(); }

    vrpnAnalog->unregister_change_handler(nullptr, VrpnAnalogCallback);
    vrpnButton->unregister_change_handler(&running, VrpnButtonCallback);

    delete vrpnButton;
    delete vrpnAnalog;

    return 0;
}
