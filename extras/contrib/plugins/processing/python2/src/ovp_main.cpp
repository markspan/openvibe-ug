#if defined TARGET_HAS_ThirdPartyPython2 && !(defined(WIN32) && defined(TARGET_BUILDTYPE_Debug))
// Windows debug build doesn't typically link as most people don't have the python debug library.

#include "box-algorithms/ovpCBoxAlgorithmPython2.h"

#if defined(PY_MAJOR_VERSION) && (PY_MAJOR_VERSION == 2)

#ifdef TARGET_OS_Windows
#include "windows.h"
#endif

#include <string>
#include <iostream>

//-------------------------------------------------------------------------------------------------
class CPython2Initializer
{
public:
    CPython2Initializer();
    ~CPython2Initializer();
    bool isPython2Available() const { return m_pythonAvailable; }
private:
#ifdef TARGET_OS_Windows
    static bool checkPython2Path();
#endif
    // PyThreadState *m_pMainPyThreadState;
    bool m_pythonAvailable;
};

#ifdef TARGET_OS_Windows
bool CPython2Initializer::checkPython2Path()
{
    std::string path = Py_GetPath();
    size_t found = path.find_first_of(';');
    while (found != std::string::npos)
    {
        if (found > 0)
        {
            std::string filename = path.substr(0, found);
            const bool exists    = (_access(filename.c_str(), 0) == 0);
            if (exists) { return true; }
        }
        path  = path.substr(found + 1);
        found = path.find_first_of(';');
    }

    std::cout << "Python directory not found. You probably have a corrupted python installation!" << std::endl;
    std::cout << "The tried path from Py_GetPath() was [" << Py_GetPath() << "]\n";

    return false;
}
#endif

CPython2Initializer::CPython2Initializer() : m_pythonAvailable(false)
{
#ifdef TARGET_OS_Windows
    __try
    {
        // We do not care about the last file, since it is the OpenViBE runtime path
        if (!Py_IsInitialized() && checkPython2Path())
        {
            Py_Initialize();
            m_pythonAvailable = true;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { }
#else
        if (!Py_IsInitialized())
        {
            Py_Initialize();
            m_pythonAvailable = true;
        }
#endif
}

CPython2Initializer::~CPython2Initializer()
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
    static CPython2Initializer python2Init;
    if (python2Init.isPython2Available()) { OVP_Declare_New(CBoxAlgorithmPython2Desc) }
OVP_Declare_End()

}  // namespace Python
}  // namespace Plugins
}  // namespace OpenViBE
//-------------------------------------------------------------------------------------------------

#else
#pragma message ("WARNING: Python 2.x headers are required to build the Python plugin, different includes found, skipped")
#endif // #if defined(PY_MAJOR_VERSION) && (PY_MAJOR_VERSION == 2)
#endif // TARGET_HAS_ThirdPartyPython2
