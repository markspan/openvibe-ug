#include "../ovasCTagStream.h"

#include <iostream>

int main()
{
    bool ok = false;

    OpenViBE::AcquisitionServer::Plugins::CTagStream tagStream1;
    std::cout << "1st stream created." << std::endl;

    // The construction of the second TagStream must fail because of port already in use.
    try { OpenViBE::AcquisitionServer::Plugins::CTagStream tagStream2; }
    catch (std::exception&) {   // This exception is expected
        std::cout << "2nd stream failed (as expected)" << std::endl;
        ok = true;
    }

    // The construction must succeed because another port is used.
    try { OpenViBE::AcquisitionServer::Plugins::CTagStream tagStream3(15362); }
    catch (std::exception& e) { // Unexpected exception
        std::cout << "Exception: " << e.what() << std::endl;
        ok = false;
    }
    std::cout << "3rd stream created." << std::endl;

    if (!ok) { return 1; }
    return 0;
}
