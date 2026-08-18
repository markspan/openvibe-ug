#ifdef TARGET_OS_Windows

#include "ovasCDriverMensiaAcquisition.h"
#include "ovasCConfigurationDriverMensiaAcquisition.h"

#include <toolkit/ovtk_all.h>

//#include <Windows.h>
#include <system/WindowsUtilities.h>

#include <string>

#include <fstream>

namespace OpenViBE {
namespace AcquisitionServer {

namespace {
enum class EAcquisitionModes { Undefined = 0, Data = 1, Impedance = 2 };

CString mensiaDll = "openvibe-driver-mensia-acquisition.dll";
HINSTANCE libMensiaAcquisition; // Library Handle

typedef int (* initialize_acquisition_driver_t)(const char* sDeviceIdentifier, IDriverContext& ctx);

typedef const char* (* get_name_t)(size_t);

typedef const char* (* get_device_url_t)(size_t);

typedef bool (*preconfigure_t)(size_t, const char*);
typedef bool (*configure_t)(size_t, const char*);
typedef uint32_t (*get_sampling_t)(size_t);
typedef uint32_t (*get_channel_count_t)(size_t);
typedef const char* (*get_channel_name_t)(size_t, size_t);
typedef bool (*is_impedance_check_requested_t)(size_t);
typedef void (*set_impedance_check_requested_t)(size_t, bool);
typedef float (*get_channel_impedance_t)(size_t, size_t);
typedef uint32_t (*get_impedance_limit_t)(size_t);
typedef void (*set_impedance_limit_t)(size_t, uint32_t);
typedef uint32_t (*get_experiment_id_t)(size_t);
typedef uint32_t (*set_experiment_id_t)(size_t, uint32_t);
typedef uint32_t (*get_subject_age_t)(size_t);
typedef uint32_t (*set_subject_age_t)(size_t, uint32_t);
typedef uint32_t (*get_subject_gender_t)(size_t);
typedef uint32_t (*set_subject_gender_t)(size_t, uint32_t);
typedef uint32_t (*set_sample_count_per_buffer_t)(size_t, uint32_t);
typedef bool (*initialize_t)(size_t, IDriverCallback*, uint32_t, const char*, EAcquisitionModes);
typedef bool (*start_t)(size_t);
typedef bool (*stop_t)(size_t);
typedef bool (*uninitialize_t)(size_t);
typedef bool (*loop_t)(size_t);

initialize_acquisition_driver_t fpInitializeAcquisitionDriver;
get_name_t fpGetName;
get_device_url_t fpGetDeviceURL;

preconfigure_t fpPreconfigure;
configure_t fpConfigure;
get_sampling_t fpGetSampling;
get_channel_count_t fpGetChannelCount;
get_channel_name_t fpGetChannelName;
is_impedance_check_requested_t fpIsImpedanceCheckRequested;
set_impedance_check_requested_t fpSetImpedanceCheckRequested;
get_channel_impedance_t fpGetChannelImpedance;
get_impedance_limit_t fpGetImpedanceLimit;
set_impedance_limit_t fpSetImpedanceLimit;
get_experiment_id_t fpGetExperimentID;
set_experiment_id_t fpSetExperimentID;
get_subject_age_t fpGetSubjectAge;
set_subject_age_t fpSetSubjectAge;
get_subject_gender_t fpGetSubjectGender;
set_subject_gender_t fpSetSubjectGender;
set_sample_count_per_buffer_t fpSetSampleCountPerBuffer;
initialize_t fpInitialize;
start_t fpStart;
stop_t fpStop;
uninitialize_t fpUninitialize;
loop_t fpLoop;
}  // namespace

template <typename T>
void CDriverMensiaAcquisition::loadDLLfunct(T* pointer, const char* name)
{
    *pointer = T(GetProcAddress(libMensiaAcquisition, name));
    if (!*pointer) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Load method " << name << "\n";
        m_valid = false;
    }
}


CDriverMensiaAcquisition::CDriverMensiaAcquisition(IDriverContext& ctx, const char* driverID)
    : IDriver(ctx)
      // This hax only works because m_settings does creates a copy of the string
      , m_settings(std::string(std::string("AcquisitionServer_Driver_MensiaAcquisition_") + driverID).c_str(), m_driverCtx.getConfigurationManager())
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverMensiaAcquisition::CDriverMensiaAcquisition\n";
    m_valid = true;

    // Load the Mensia Acquisition Library
    const CString path = m_driverCtx.getConfigurationManager().expand("${Path_Bin}") + "/" + mensiaDll;
    if (!std::ifstream(path.toASCIIString()).is_open()) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverMensiaAcquisition::CDriverMensiaAcquisition: " << " dll file ["
                << path << "] not openable, perhaps it was not installed.\n";
        m_valid = false;
        return;
    }

    libMensiaAcquisition = HINSTANCE(System::WindowsUtilities::utf16CompliantLoadLibrary(path.toASCIIString()));
    if (!libMensiaAcquisition) {
        m_driverCtx.getLogManager() << "CDriverMensiaAcquisition::CDriverMensiaAcquisition: utf16CompliantLoadLibrary failed to load: "
                << path << " with error " << size_t(GetLastError()) << "\n";
    }

    loadDLLfunct<initialize_acquisition_driver_t>(&fpInitializeAcquisitionDriver, "initializeAcquisitionDriver");
    loadDLLfunct<get_name_t>(&fpGetName, "getName");
    loadDLLfunct<get_device_url_t>(&fpGetDeviceURL, "getDeviceURL");
    loadDLLfunct<preconfigure_t>(&fpPreconfigure, "preconfigure");
    loadDLLfunct<configure_t>(&fpConfigure, "configure");
    loadDLLfunct<get_sampling_t>(&fpGetSampling, "getSamplingRate");
    loadDLLfunct<get_channel_count_t>(&fpGetChannelCount, "getChannelCount");
    loadDLLfunct<get_channel_name_t>(&fpGetChannelName, "getChannelName");
    loadDLLfunct<is_impedance_check_requested_t>(&fpIsImpedanceCheckRequested, "isImpedanceCheckRequested");
    loadDLLfunct<set_impedance_check_requested_t>(&fpSetImpedanceCheckRequested, "setImpedanceCheckRequested");
    loadDLLfunct<get_channel_impedance_t>(&fpGetChannelImpedance, "getChannelImpedance");
    loadDLLfunct<get_impedance_limit_t>(&fpGetImpedanceLimit, "getImpedanceLimit");
    loadDLLfunct<set_impedance_limit_t>(&fpSetImpedanceLimit, "setImpedanceLimit");
    loadDLLfunct<get_experiment_id_t>(&fpGetExperimentID, "getExperimentID");
    loadDLLfunct<set_experiment_id_t>(&fpSetExperimentID, "setExperimentID");
    loadDLLfunct<get_subject_age_t>(&fpGetSubjectAge, "getSubjectAge");
    loadDLLfunct<set_subject_age_t>(&fpSetSubjectAge, "setSubjectAge");
    loadDLLfunct<get_subject_gender_t>(&fpGetSubjectGender, "getSubjectGender");
    loadDLLfunct<set_subject_gender_t>(&fpSetSubjectGender, "setSubjectGender");
    loadDLLfunct<set_sample_count_per_buffer_t>(&fpSetSampleCountPerBuffer, "setSampleCountPerBuffer");
    loadDLLfunct<initialize_t>(&fpInitialize, "initialize");
    loadDLLfunct<start_t>(&fpStart, "start");
    loadDLLfunct<stop_t>(&fpStop, "stop");
    loadDLLfunct<uninitialize_t>(&fpUninitialize, "uninitialize");
    loadDLLfunct<loop_t>(&fpLoop, "loop");

    if (!m_valid) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Could not initialize Mensia Acquisition Driver driver\n";
        return;
    }

    // prepare the device library

    const int id = fpInitializeAcquisitionDriver(driverID, ctx);

    // Negative value is considered an error
    if (id < 0) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Could not initialize Mensia Acquisition Driver driver\n";
        return;
    }
    m_driverID = uint32_t(id);


    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Initialized Mensia Acquisition Driver on id [" << m_driverID << "]" << "\n";

    //TODO_JL Set sampling frenquency and channel count in header?
    m_header.setSamplingFrequency(128);
    m_header.setChannelCount(0);
    m_header.setExperimentID(0);
    m_header.setSubjectAge(0);
    m_header.setSubjectGender(0);

    m_settings.add("Header", &m_header);
    m_settings.add("DeviceURL", &m_deviceURL);
    //TODO_JL Add the URL to settings so it can be saved (we should be able to expose it)
    m_settings.load();
    fpSetExperimentID(m_driverID, m_header.getExperimentID());
    fpSetSubjectAge(m_driverID, m_header.getSubjectAge());
    fpSetSubjectGender(m_driverID, m_header.getSubjectGender());

    fpSetImpedanceCheckRequested(m_driverID, m_header.isImpedanceCheckRequested());
    fpSetImpedanceLimit(m_driverID, m_header.getImpedanceLimit());

    fpPreconfigure(m_driverID, m_deviceURL.toASCIIString());

    m_header.setChannelCount(fpGetChannelCount(m_driverID));
    m_header.setSamplingFrequency(fpGetSampling(m_driverID));

    for (size_t index = 0; index < m_header.getChannelCount(); ++index) { m_header.setChannelName(index, fpGetChannelName(m_driverID, index)); }
}

void CDriverMensiaAcquisition::release()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverMensiaAcquisition::release\n";
    delete this;
}

const char* CDriverMensiaAcquisition::getName() { return fpGetName(m_driverID); }

//___________________________________________________________________//
//                                                                   //

bool CDriverMensiaAcquisition::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverMensiaAcquisition::initialize\n";

    if (m_driverCtx.isConnected()) { return false; }

    m_nSamplePerSentBlock = nSamplePerSentBlock;
    fpSetSampleCountPerBuffer(m_driverID, m_nSamplePerSentBlock);
    m_deviceURL = CString(fpGetDeviceURL(m_driverID));

    m_callback = &callback;

    m_header.setImpedanceCheckRequested(fpIsImpedanceCheckRequested(m_driverID));
    const EAcquisitionModes mode = m_header.isImpedanceCheckRequested() ? EAcquisitionModes::Impedance : EAcquisitionModes::Data;
    if (!fpInitialize(m_driverID, m_callback, m_nSamplePerSentBlock, m_deviceURL.toASCIIString(), mode)) { return false; }

    fpSetExperimentID(m_driverID, m_header.getExperimentID());
    fpSetSubjectAge(m_driverID, m_header.getSubjectAge());
    fpSetSubjectGender(m_driverID, m_header.getSubjectGender());

    m_header.setSamplingFrequency(fpGetSampling(m_driverID));
    m_header.setChannelCount(fpGetChannelCount(m_driverID));
    m_header.setExperimentID(fpGetExperimentID(m_driverID));
    m_header.setSubjectAge(fpGetSubjectAge(m_driverID));
    m_header.setSubjectGender(fpGetSubjectGender(m_driverID));
    m_header.setImpedanceLimit(fpGetImpedanceLimit(m_driverID));
    m_header.setImpedanceCheckRequested(fpIsImpedanceCheckRequested(m_driverID));


    for (size_t uiChannelIndex = 0; uiChannelIndex < m_header.getChannelCount(); ++uiChannelIndex) {
        m_header.setChannelName(uiChannelIndex, fpGetChannelName(m_driverID, uiChannelIndex));
        m_header.setChannelUnits(uiChannelIndex, OVTK_UNIT_Volts, OVTK_FACTOR_Micro);
    }

    return true;
}

bool CDriverMensiaAcquisition::start()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverMensiaAcquisition::start\n";

    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    if (!fpStart(m_driverID)) { return false; }

    return true;
}

bool CDriverMensiaAcquisition::loop()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "CDriverMensiaAcquisition::loop\n";

    if (!m_driverCtx.isConnected()) { return false; }

    if (m_driverCtx.isStarted()) { if (!fpLoop(m_driverID)) { return false; } }
    else {
        if (!fpLoop(m_driverID)) { return false; }
        // impedance check here
        if (m_driverCtx.isImpedanceCheckRequested()) {
            for (size_t j = 0; j < m_header.getChannelCount(); ++j) { m_driverCtx.updateImpedance(j, double(fpGetChannelImpedance(m_driverID, j))); }
        }
    }

    return true;
}

bool CDriverMensiaAcquisition::stop()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverMensiaAcquisition::stop\n";


    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }

    return fpStop(m_driverID);
}

bool CDriverMensiaAcquisition::uninitialize()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverMensiaAcquisition::uninitialize\n";

    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    fpUninitialize(m_driverID);

    delete [] m_sample;
    m_sample   = nullptr;
    m_callback = nullptr;

    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CDriverMensiaAcquisition::isConfigurable()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverMensiaAcquisition::isConfigurable\n";
    return true;
}

bool CDriverMensiaAcquisition::configure()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverMensiaAcquisition::configure\n";
    fpSetExperimentID(m_driverID, m_header.getExperimentID());
    fpSetSubjectAge(m_driverID, m_header.getSubjectAge());
    fpSetSubjectGender(m_driverID, m_header.getSubjectGender());

    /*CConfigurationDriverMensiaAcquisition config(m_driverCtx, Directories::getDataDir() + "/applications/acquisition-server/interface-Generic-Oscillator.ui" );
*/
    if (fpConfigure(m_driverID, m_deviceURL.toASCIIString())) {
        m_deviceURL = CString(fpGetDeviceURL(m_driverID));

        // We need to escape the URL before we save the setting to the file because
        // of the way OpenViBE handles strings
        const CString tempURL  = m_deviceURL;
        std::string escapedURL = m_deviceURL.toASCIIString();
        size_t pos             = 0;
        while ((pos = escapedURL.find('{', pos)) != std::string::npos) {
            escapedURL.replace(pos, 1, "\\{");
            pos += 2;
        }

        pos = 0;
        while ((pos = escapedURL.find('}', pos)) != std::string::npos) {
            escapedURL.replace(pos, 1, "\\}");
            pos += 2;
        }

        pos = 0;
        while ((pos = escapedURL.find('$', pos)) != std::string::npos) {
            escapedURL.replace(pos, 1, "\\$");
            pos += 2;
        }

        m_deviceURL = escapedURL.c_str();
        m_header.setSamplingFrequency(fpGetSampling(m_driverID));
        m_header.setChannelCount(fpGetChannelCount(m_driverID));
        m_header.setExperimentID(fpGetExperimentID(m_driverID));
        m_header.setSubjectAge(fpGetSubjectAge(m_driverID));
        m_header.setSubjectGender(fpGetSubjectGender(m_driverID));
        m_header.setImpedanceLimit(fpGetImpedanceLimit(m_driverID));
        m_header.setImpedanceCheckRequested(fpIsImpedanceCheckRequested(m_driverID));
        /*
        fpSetExperimentID(m_driverID, m_header.getExperimentID());
        fpSetSubjectAge(m_driverID, m_header.getSubjectAge());
        fpSetSubjectGender(m_driverID, m_header.getSubjectGender());
        */

        /*
        for (size_t index = 0; index < m_header.getChannelCount(); ++index) { m_header.setChannelName(index, ""); }
        */

        m_settings.save();
        m_deviceURL = tempURL;
        return true;
    }


    return false;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_OS_Windows
