#if defined(TARGET_HAS_ThirdPartyEEGOAPI)

// stl includes
#include <algorithm>
#include <bitset>
#include <exception>
#include <memory>
#include <regex>

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
#include <cstddef>
#include <type_traits>
#include <utility>
#endif

// OV includes
#include <toolkit/ovtk_all.h>
#include <system/ovCTime.h>

// auxilliary classes for this implementation
#include "ovasCDriverEEGO.h"
#include "ovasCConfigurationEEGO.h"

// The interface to the driver
// We have to setup the binding method and the unicode support first
#define _UNICODE
#if defined(_MBCS) // Only unicode should be supported. Better stay away from MBCS methods!
#undef _MBCS
#endif
#include <eemagine/sdk/factory.h> // Where it all starts

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
namespace std {
    template<class T> struct _Unique_if {
        typedef unique_ptr<T> _Single_object;
    };

    template<class T> struct _Unique_if<T[]> {
        typedef unique_ptr<T[]> _Unknown_bound;
    };

    template<class T, size_t N> struct _Unique_if<T[N]> {
        typedef void _Known_bound;
    };

    template<class T, class... Args>
        typename _Unique_if<T>::_Single_object
        make_unique(Args&&... args) {
            return unique_ptr<T>(new T(std::forward<Args>(args)...));
        }

    template<class T>
        typename _Unique_if<T>::_Unknown_bound
        make_unique(size_t n) {
            typedef typename remove_extent<T>::type U;
            return unique_ptr<T>(new U[n]());
        }

    template<class T, class... Args>
        typename _Unique_if<T>::_Known_bound
        make_unique(Args&&...) = delete;
}
#endif

// Namespaces
namespace OpenViBE {
namespace AcquisitionServer {
namespace es = eemagine::sdk;

//___________________________________________________________________//
//                                                                   //

CDriverEEGO::CDriverEEGO(IDriverContext& ctx)
    : IDriver(ctx)
      , m_settings("AcquisitionServer_Driver_EEGO", m_driverCtx.getConfigurationManager()), m_triggerChannel(-1) // == Nonexistent
      , m_lastTriggerValue(~0) // Set every bit to 1
      , m_iBIPRange(1500)
      , m_iEEGRange(1000)
      , m_sEEGMask("0xFFFFFFFFFFFFFFFF") // 64 channels
      , m_sBIPMask("0xFFFFFF") // 24 channels
{
    m_header.setSamplingFrequency(500);
    m_header.setChannelCount(88);

    // The following class allows saving and loading driver settings from the acquisition server .conf file
    m_settings.add("Header", &m_header);

    // To save your custom driver settings, register each variable to the SettingsHelper
    //m_settings.add("SettingName", &variable);
    m_settings.add("BIPRange", &m_iBIPRange);
    m_settings.add("EEGRange", &m_iEEGRange);

    m_settings.add("EEGMask", &m_sEEGMask);
    m_settings.add("BIPMask", &m_sBIPMask);

    m_settings.load();
}

CDriverEEGO::~CDriverEEGO() {}

const char* CDriverEEGO::getName() { return "EEGO"; }

bool CDriverEEGO::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    if (m_driverCtx.isConnected()
        || !m_header.isChannelCountSet()
        || !m_header.isSamplingFrequencySet()) { return false; }

    try
    {
        // Builds up a buffer to store
        // acquired samples. This buffer
        // will be sent to the acquisition
        // server later...
        m_sample          = std::make_unique<float[]>(m_header.getChannelCount() * nSamplePerSentBlock);
        m_samplesInBuffer = 0;

        // Get the amplifier. If none is connected an exception will be thrown
        try { m_pAmplifier.reset(factory().getAmplifier()); }
        catch (const std::exception& ex)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Failure to get an amplifier! Reason: " << ex.what() << "\n";
            throw;
        }

        if (m_driverCtx.isImpedanceCheckRequested())
        {
            // end streaming first, if started
            m_pStream.reset();

            // After init we are in impedance mode until the recording is started
            m_pStream.reset(m_pAmplifier->OpenImpedanceStream(getRefChannelMask()));
        }
    }
    catch (const std::exception& ex)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Failed to initialize the driver. Exception: " << ex.what() << "\n";

        m_sample.reset();
        m_pAmplifier.reset();
        m_pStream.reset();

        return false;
    }

    // Save parameters
    m_callback            = &callback;
    m_nSamplePerSentBlock = nSamplePerSentBlock;

    return true;
}

bool CDriverEEGO::check_configuration()
{
    // get masks from configuration
    const uint64_t maskBip = getBipChannelMask();
    const uint64_t maskEEG = getRefChannelMask();

    const std::bitset<64> bitsetEEG(maskEEG);
    const std::bitset<24> bitsetBip(maskBip);

    const size_t allChannels = bitsetBip.count() + bitsetEEG.count() + 2; // trigger and sample count as additional channels
    if (allChannels < m_header.getChannelCount())
    {
        // Not enough channels, we have to reduce them
        GtkWidget* dialog = gtk_message_dialog_new(nullptr, // parent
                                                   GTK_DIALOG_MODAL, // Behavoir
                                                   GTK_MESSAGE_QUESTION, // Type
                                                   GTK_BUTTONS_OK_CANCEL, // buttons
                                                   "The channels masks are set to only stream %ld channels, but %d channels should be streamed.\n"
                                                   "Change the amount of channels to %ld?", allChannels, m_header.getChannelCount(), allChannels);
        const gint res = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        dialog = nullptr;
        switch (res)
        {
            case GTK_RESPONSE_OK:
                // update the channel count to contain only the selected masks
                m_header.setChannelCount(allChannels);
                break;
            default:
                // Nothing can be done here
                return false;
        }
    }

    return true;
}

uint64_t CDriverEEGO::getRefChannelMask() const
{
    uint64_t maskEEG(0);
    if (!CHeaderEEGO::convertMask(m_sEEGMask, maskEEG))
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Error converting mask: m_sEEGMask: " << m_sEEGMask << "\n";
    }
    return maskEEG;
}

uint64_t CDriverEEGO::getBipChannelMask() const
{
    uint64_t maskBip(0);
    if (!CHeaderEEGO::convertMask(m_sBIPMask, maskBip))
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Error converting mask: maskBip: " << maskBip << "\n";
    }
    return maskBip;
}

eemagine::sdk::factory& CDriverEEGO::factory()
{
    if (m_pFactory == nullptr)
    {
        // create the amplifier factory
        // To initialize we need to locate the path of the DLL
        // Create path to the dll
#ifdef _WIN32
        const CString libDir = Directories::getBinDir() + "\\eego-SDK.dll";
        const std::string path(libDir.toASCIIString());
#else
        const std::string path("libeego-SDK.so");
#endif // _WIN32

        m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "SDK dll/so path: " << path << "\n";
        m_pFactory = std::make_unique<es::factory>(path);

        // to check what is going on case of error; Log version
        const auto version = m_pFactory->getVersion();
        m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "EEGO SDK Version: " << version.major << "." << version.minor << "." << version.micro << "." <<
                version.build << "\n";
    }

    return *m_pFactory;
}

bool CDriverEEGO::start()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted() || !m_pAmplifier) { return false; }

    // Check configuration
    if (!check_configuration()) { return false; }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "CDriverEEGO::start - Check amplifier" << "\n";
    /*
     * Check device type and set BIP range accordingly
     * This step is necessary for the 511 series amplifiers
     */
    try {
        // stop old streams, if existing
        m_pStream.reset();
        m_pAmplifier->OpenEegStream(m_header.getSamplingFrequency());

        const std::string amplifierType = m_pAmplifier->getType();

        std::regex amplifierType511("EE511");
        if (std::regex_search(amplifierType, amplifierType511) && m_iBIPRange != 2.5 * m_iEEGRange) {
                m_iBIPRange = 2.5 * m_iEEGRange;
                m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Amplifier type EE-511: BIP Signal Range is constrained to 2.5 times the EEG Signal range. Forcing BIP Signal Range to " << m_iBIPRange << "\n";
        }

        /*
         * Needed in order to successfully call OpenEEGStream again.
         * Needs to be done in two steps, otherwised the amplifier is not released
         */
        m_pAmplifier.reset();
        m_pAmplifier.reset(factory().getAmplifier());

    } catch(const std::exception& ex) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Did not manage to verify amplifier type: " << ex.what() << "\n";
            return false;
    }

    // ...
    // request hardware to start
    // sending data
    // ..
    const double bipRange = m_iBIPRange / 1000.;
    const double eegRange = m_iEEGRange / 1000.;

    try
    {
        // stop old streams, if existing
        m_pStream.reset();
        // Create the eeg stream
        m_pStream.reset(m_pAmplifier->OpenEegStream(m_header.getSamplingFrequency(), eegRange, bipRange, getRefChannelMask(), getBipChannelMask()));

        // Error check
        if (!m_pStream)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "The stream returned is NULL!" << "\n";
            return false;
        }

        // find trigger channel
        auto list                  = m_pStream->getChannelList();
        const auto triggerIterator = std::find_if(list.begin(), list.end(), [](eemagine::sdk::channel& chan)
        {
            return chan.getType() == eemagine::sdk::channel::trigger;
        });

        if (triggerIterator == list.end())
        {
            m_triggerChannel = -1; // Unkown
        }
        else { m_triggerChannel = (*triggerIterator).getIndex(); }

        // Wait till we are really getting data.
        while (m_pStream->getData().getSampleCount() == 0)
        {
            System::Time::sleep(5); // Do Nothing
        }
    }
    catch (const std::exception& ex)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not open EEG stream: " << ex.what() << "\n";
        return false;
    }

    return true;
}

bool CDriverEEGO::loop()
{
    bool result = false;

    try { result = loop_wrapped(); }
    catch (const std::exception& ex) { m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Error in data update: " << ex.what() << "\n"; }
    catch (...) { m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unknown error in data update." << "\n"; }

    return result;
}

bool CDriverEEGO::loop_wrapped()
{
    if (!m_driverCtx.isConnected()) { return false; }
    if (!m_driverCtx.isStarted() && !m_driverCtx.isImpedanceCheckRequested()) { return true; } // Nothing to be done here!
    if (!m_pStream) { return false; }

    // Check if we really provide enough channels
    // When doing impedance only the normal EEG channels are tested. This is fine and handled.
    if (m_pStream->getChannelList().size() < m_header.getChannelCount() && m_driverCtx.isStarted()) // !started -> impedance
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "The amplifier got asked for more channels than it could provide" << "\n";
        return false;
    }

    if (m_driverCtx.isStarted()) // Normal operation
    {
        eemagine::sdk::buffer data;

        try { data = m_pStream->getData(); }
        catch (const std::exception& ex)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Error fetching data: " << ex.what() << "\n";
            return false;
        }

        const size_t nSample = data.getSampleCount();

        // For EEGO the with every index increment, the channel is incremented.
        // For OpenVibe it means next sample. Therefor we have to transpose the data
        for (size_t sample = 0; sample < nSample; ++sample)
        {
            for (size_t channel = 0; channel < m_header.getChannelCount(); ++channel)
            {
                const int ovIdx = int(m_samplesInBuffer + channel * m_nSamplePerSentBlock);

                const double& sampleVal = data.getSample(channel, sample);
                m_sample[ovIdx]         = float(sampleVal);
            }

            // Add potential triggers to stimulation set
            // check for triggers
            if (m_triggerChannel >= 0) // Only try to find triggers when the channel exists
            {
                // A trigger is detected when the level changes in positive direction, all additional bits are seen as trigger code
                // a change from 1 to 0 is ignored
                const uint32_t currentTriggers    = uint32_t(data.getSample(m_triggerChannel, sample));
                const uint32_t currentNewTriggers = currentTriggers & ~m_lastTriggerValue;
                m_lastTriggerValue                = currentTriggers;

                if (currentNewTriggers != 0)
                {
                    const uint64_t currentTime = CTime(m_header.getSamplingFrequency(), m_samplesInBuffer).time();
                    m_stimSet.push_back(OVTK_StimulationId_Label(currentNewTriggers), currentTime, 0);
                }
            }

            // Send buffer counter
            m_samplesInBuffer++;

            // Send buffer is full, so send it
            if (m_samplesInBuffer == m_nSamplePerSentBlock)
            {
                m_callback->setSamples(m_sample.get());
                m_callback->setStimulationSet(m_stimSet);

                // When your sample buffer is fully loaded,
                // it is advised to ask the acquisition server
                // to correct any drift in the acquisition automatically.
                m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());

                m_samplesInBuffer = 0;
                m_stimSet.clear();
            }
        }
    }
    else // Impedance
    {
        // Get the impedance data, here the data is always the most current state.
        // The method can block if impedance still needs to be calculated.
        eemagine::sdk::buffer data;
        try { data = m_pStream->getData(); }
        catch (const std::exception& ex)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Error fetching data: " << ex.what() << "\n";
            return false;
        }

        // We have to take care not to r/w over any boundary.
        const size_t minChannels = std::min(size_t(data.getChannelCount()), m_header.getChannelCount());
        for (size_t channel = 0; channel < minChannels; ++channel) { m_driverCtx.updateImpedance(channel, data.getSample(channel, 0)); }
    }

    return true;
}

bool CDriverEEGO::stop()
{
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }

    // ...
    // request the hardware to stop
    // sending data
    // ...
    if (m_driverCtx.isImpedanceCheckRequested())
    {
        try
        {
            m_pStream.reset();
            m_pStream.reset(m_pAmplifier->OpenImpedanceStream(getRefChannelMask())); // And we can stream Impedances once more.
        }
        catch (const std::exception& ex)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Error restarting impedance: " << ex.what() << "\n";
            return false;
        }
    }

    return true;
}

bool CDriverEEGO::uninitialize()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    // ...
    // uninitialize hardware here
    // ...
    m_pStream.reset();
    m_pAmplifier.reset();
    m_sample.reset();

    m_callback        = nullptr;
    m_samplesInBuffer = 0;

    return true;
}

//___________________________________________________________________//
//                                                                   //
bool CDriverEEGO::isConfigurable()
{
    return true; // change to false if your device is not configurable
}

bool CDriverEEGO::configure()
{
    // Change this line if you need to specify some references to your driver attribute that need configuration, e.g. the connection ID.
    CConfigurationEEGO config(m_driverCtx,
                              Directories::getDataDir() + "/applications/acquisition-server/interface-EEGO.ui",
                              m_header);

    m_header.setBIPRange(m_iBIPRange);
    m_header.setEEGRange(m_iEEGRange);
    m_header.setBIPMask(m_sBIPMask);
    m_header.setEEGMask(m_sEEGMask);
    if (!config.configure(m_header)) { return false; }

    m_iBIPRange = m_header.getBIPRange();
    m_iEEGRange = m_header.getEEGRange();
    m_sBIPMask  = m_header.getBIPMask();
    m_sEEGMask  = m_header.getEEGMask();

    m_settings.save();

    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif
