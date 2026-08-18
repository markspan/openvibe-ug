#include "box-algorithms/ovpCBoxAlgorithmPython3.h"

#ifdef TARGET_OS_Windows
#include "windows.h"
#endif

#include <string>
#include <iostream>

//-------------------------------------------------------------------------------------------------
class CPython3Initializer
{
public:
    CPython3Initializer();
    ~CPython3Initializer();
    bool isPython3Available() const { return m_pythonAvailable; }
private:
    bool m_pythonAvailable;
};


CPython3Initializer::CPython3Initializer() : m_pythonAvailable(false)
{
    try
    {
        // We do not care about the last file, since it is the OpenViBE runtime path
        if (!Py_IsInitialized())
        {
            PyConfig config;
            PyConfig_InitIsolatedConfig(&config);


            OpenViBE::CString pythonRuntimeDir = OpenViBE::Directories::getBinDir() + "/python_runtime";

            PyConfig_SetBytesString(&config, &config.home, pythonRuntimeDir.toASCIIString());

            PyStatus status = Py_InitializeFromConfig(&config);
            PyConfig_Clear(&config);

            if (PyStatus_IsError(status)) {
                std::cout << "Failed initializing Python from config" << std::endl;
                if (PyStatus_Exception(status)) {
                    Py_ExitStatusException(status);
                }
            }
            m_pythonAvailable = Py_IsInitialized();
        }
    }
    catch (const std::exception &e)
    {
        std::cout << "Error Initializing Python: " << e.what() << std::endl;
    }
}

CPython3Initializer::~CPython3Initializer()
{
    if (m_pythonAvailable)
    {
        m_pythonAvailable = false;
        Py_Finalize();
    }
}
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
namespace OpenViBE {
namespace Plugins {
namespace Python {

OVP_Declare_Begin()
    static CPython3Initializer python3Init;
    if (python3Init.isPython3Available()) { OVP_Declare_New(CBoxAlgorithmPython3Desc) }
OVP_Declare_End()

}  // namespace Python
}  // namespace Plugins
}  // namespace OpenViBE
//-------------------------------------------------------------------------------------------------
