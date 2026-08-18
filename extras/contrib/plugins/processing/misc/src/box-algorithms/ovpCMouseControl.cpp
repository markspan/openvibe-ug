#include "ovpCMouseControl.h"

#include <iostream>

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
    #include <unistd.h>
#endif

namespace OpenViBE {
namespace Plugins {
namespace Tools {   //Ambiguous without OpenViBEPlugins

bool CMouseControl::initialize()
{
    m_decoder = new Toolkit::TStreamedMatrixDecoder<CMouseControl>(*this, 0);

#if !defined(TARGET_OS_Linux) || !defined(TARGET_OS_MacOS)
    getLogManager() << Kernel::LogLevel_Error << "This box algorithm is not implemented for your operating system\n";
    return false;
#else
    return true;
#endif
}

bool CMouseControl::uninitialize()
{
    m_decoder->uninitialize();
    delete m_decoder;

    return true;
}

bool CMouseControl::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CMouseControl::process()
{
    Kernel::IBoxIO* boxContext = getBoxAlgorithmContext()->getDynamicBoxContext();
    const size_t nInputChunk   = boxContext->getInputChunkCount(0);

    for (size_t i = 0; i < nInputChunk; ++i)
    {
        m_decoder->decode(i);
        if (m_decoder->isBufferReceived())
        {
            CMatrix* iMatrix = m_decoder->getOutputMatrix();
            if (iMatrix->getBufferElementCount() != 1)
            {
                getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Error << "Error, dimension size isn't 1 for Amplitude input !\n";
                return false;
            }
#if defined TARGET_OS_Linux
            const double* iBuffer = iMatrix->getBuffer();
            m_pMainDisplay=::XOpenDisplay(NULL);
            if (!m_pMainDisplay)
            {
                getLogManager() << Kernel::LogLevel_Error << "Impossible to open Display.\n";
                return false;
            }
            m_oRootWindow=DefaultRootWindow(m_pMainDisplay);  //all X11 screens
            ::XSelectInput(m_pMainDisplay, m_oRootWindow, ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|OwnerGrabButtonMask);

            int offsetY = 0;
            int offsetX = int(iBuffer[0]*100.0);

            getLogManager() << Kernel::LogLevel_Debug << "offsetX = " << offsetX << "\n";

            ::XWarpPointer(m_pMainDisplay, m_oRootWindow, 0, 0, 0, 0, 0, offsetX, offsetY);
            ::XCloseDisplay(m_pMainDisplay);
#endif
            // TODO
            // For windows use:
            // SetCursorPos(int x, int y)
        }
    }
    return true;
}
}  // namespace Tools
}  // namespace Plugins
}  // namespace OpenViBE
