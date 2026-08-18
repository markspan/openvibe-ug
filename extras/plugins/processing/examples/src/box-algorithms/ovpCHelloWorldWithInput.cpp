/*
 * Prints user-specified greeting to the log every time process() is called. Passes the signal through. 
 */
#include "ovpCHelloWorldWithInput.h"

namespace OpenViBE {
namespace Plugins {
namespace Examples {

bool CHelloWorldWithInput::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CHelloWorldWithInput::process()
{
    Kernel::IBoxIO* boxContext = getBoxAlgorithmContext()->getDynamicBoxContext();
    const size_t nInput        = getBoxAlgorithmContext()->getStaticBoxContext()->getInputCount();
    uint64_t start             = 0;
    uint64_t end               = 0;
    size_t chunkSize           = 0;
    const uint8_t* buffer      = nullptr;

    for (size_t i = 0; i < nInput; ++i)
    {
        for (size_t j = 0; j < boxContext->getInputChunkCount(i); ++j)
        {
            boxContext->getInputChunk(i, j, start, end, chunkSize, buffer);
            boxContext->appendOutputChunkData(i, buffer, chunkSize);
            boxContext->markOutputAsReadyToSend(i, start, end);
            boxContext->markInputAsDeprecated(i, j);
        }
    }

    const CString myGreeting = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    getLogManager() << Kernel::LogLevel_Info << ": " << myGreeting << "\n";

    return true;
}
}  // namespace Examples
}  // namespace Plugins
}  // namespace OpenViBE
