#include "ovasCDriverCognionics.h"
#include "ovasCConfigurationCognionics.h"
#include <stdio.h>
#include <stdlib.h>

#if defined(WIN32)

#include <windows.h>

#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace AcquisitionServer {

//Cognionics functions

int init_SPP(int port);
void getData();
double get_IMP_CH(int ch);
void close_SPP();
unsigned long grab_SPP(int bytes, unsigned char* buf);
void write_SPP(int bytes, unsigned char* buf);

//Cognionics defs
#define GAIN 3.0
#define VREF 2.5
#define ISTIM 0.000000024
#define ADC_TO_VOLTS 2.0*(VREF/(4294967296.0*GAIN))
#define TO_Z 1.4/(ISTIM*2.0)

//Cognionics variables
int EEG_GRAB;
int CHS;
int SAMPLE_RATE;
float* dataBufferPtr;
int prev_trigger;
CStimulationSet cogStimulationSet;

//Cognionics Buffers
int* prev_4_samples; //buffer for previous 4 samples in each channel for computing MA and IMP


//___________________________________________________________________//
//                                                                   //

CDriverCognionics::CDriverCognionics(IDriverContext& ctx)
    : IDriver(ctx), m_settings("AcquisitionServer_Driver_Cognionics", m_driverCtx.getConfigurationManager()), m_comPort(1)
{
    m_header.setSamplingFrequency(300);
    m_header.setChannelCount(64);

    m_settings.add("Header", &m_header);
    m_settings.add("ComPort", &m_comPort);
    m_settings.load();
}

//___________________________________________________________________//
//                                                                   //

bool CDriverCognionics::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    if (m_driverCtx.isConnected() || !m_header.isChannelCountSet() || !m_header.isSamplingFrequencySet()) { return false; }

    // Builds up a buffer to store
    // acquired samples. This buffer
    // will be sent to the acquisition
    // server later...
    m_sample = new float[m_header.getChannelCount() * nSamplePerSentBlock];

    //initalize Cognionics varaibles, pointers and buffers
    EEG_GRAB       = nSamplePerSentBlock;
    SAMPLE_RATE    = m_header.getSamplingFrequency();
    CHS            = m_header.getChannelCount();
    dataBufferPtr  = &m_sample[0];
    prev_4_samples = new int[EEG_GRAB * CHS];

    if (!m_sample)
    {
        delete [] m_sample;
        delete [] prev_4_samples;
        m_sample = nullptr;
        return false;
    }

    // ...
    // initialize hardware and get
    // available header information
    // from it
    // Using for example the connection ID provided by the configuration (m_connectionID)
    // ...

    //init serial port
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Attempting to Connect to Device at COM Port: " << m_comPort << "\n";

    const int serSuccess = init_SPP(m_comPort);
    if (serSuccess == -1)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unable to Open Port, Please Check Device and Settings\n";
        return false;
    }
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Successfully Opened Port at COM: " << m_comPort << "\n";
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Number Channels (EEG + Packet Counter + Trigger): " << m_header.getChannelCount() << "\n";
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Sampling Rate: " << m_header.getSamplingFrequency() << "\n";

    for (size_t c = 0; c < m_header.getChannelCount(); ++c) { m_header.setChannelUnits(c, OVTK_UNIT_Volts, OVTK_FACTOR_Base); }

    //set impedance check mode ON
    getData();
    unsigned char imp_on = 0x11;
    write_SPP(1, &imp_on);

    // Saves parameters
    m_callback            = &callback;
    m_nSamplePerSentBlock = nSamplePerSentBlock;
    return true;
}

bool CDriverCognionics::start()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    // ...
    // request hardware to start
    // sending data
    // ...

    //set impedance check mode off
    getData();
    unsigned char imp_off = 0x12;
    write_SPP(1, &imp_off);

    return true;
}

bool CDriverCognionics::loop()
{
    if (!m_driverCtx.isConnected()) { return false; }

    if (m_driverCtx.isStarted())
    {
        // ...
        // receive samples from hardware
        // put them the correct way in the sample array
        // whether the buffer is full, send it to the acquisition server
        //...

        //get the number of samples specified
        getData();

        //OpenVibe call back for new samples
        m_callback->setSamples(m_sample);

        // When your sample buffer is fully loaded, 
        // it is advised to ask the acquisition server 
        // to correct any drift in the acquisition automatically.
        m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());

        // ...
        // receive events from hardware
        // and put them the correct way in a CStimulationSet object
        //...
        m_callback->setStimulationSet(cogStimulationSet);
        cogStimulationSet.clear();
    }
    else
    {
        //get a fresh batch of samples to keep connection alive
        getData();
        if (m_driverCtx.isImpedanceCheckRequested())
        {
            //update impedance values
            for (int c = 0; c < CHS; ++c) { m_driverCtx.updateImpedance(c, get_IMP_CH(c)); }
        }
    }

    return true;
}

bool CDriverCognionics::stop()
{
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }

    // ...
    // request the hardware to stop
    // sending data
    // ...

    //set impedance check mode on
    getData();
    unsigned char imp_off = 0x11;
    write_SPP(1, &imp_off);

    return true;
}

bool CDriverCognionics::uninitialize()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    // ...
    // uninitialize hardware here
    // ...
    close_SPP();

    //free buffers
    delete [] m_sample;
    delete [] prev_4_samples;
    m_sample   = nullptr;
    m_callback = nullptr;

    return true;
}

//___________________________________________________________________//
//                                                                   //
bool CDriverCognionics::isConfigurable()
{
    return true; // change to false if your device is not configurable
}

bool CDriverCognionics::configure()
{
    // Change this line if you need to specify some references to your driver attribute that need configuration, e.g. the connection ID.
    CConfigurationCognionics config(m_driverCtx,
                                    Directories::getDataDir() + "/applications/acquisition-server/interface-Cognionics.ui",
                                    m_comPort); // the specific header is passed into the specific configuration

    if (!config.configure(m_header)) { return false; }

    m_settings.save();

    return true;
}

void getData()
{
    unsigned char packet_start = 0;
    int lsb2, lsb1;
    unsigned char temp;

    for (int c = 0; c < EEG_GRAB; ++c)
    {
        packet_start = 0;

        //wait for packet start
        while (packet_start != 0xFF) { grab_SPP(1, &packet_start); }

        //get packet counter
        grab_SPP(1, &packet_start);

        //grab CHS-2 since the last two channels are for sync and packet counter
        for (int j = 0; j < (CHS - 2); ++j)
        {
            grab_SPP(1, &temp);
            int msb = temp;
            grab_SPP(1, &temp);
            lsb2 = temp;
            grab_SPP(1, &temp);
            lsb1 = temp;

            //reassemble 24-bit 2's compltement promoted to 32  bit int
            msb = (msb << 24) | (lsb2 << 17) | (lsb1 << 10);

            //shift previous 4 sample buffer
            *(prev_4_samples + j * 4 + 3) = *(prev_4_samples + j * 4 + 2);
            *(prev_4_samples + j * 4 + 2) = *(prev_4_samples + j * 4 + 1);
            *(prev_4_samples + j * 4 + 1) = *(prev_4_samples + j * 4 + 0);
            *(prev_4_samples + j * 4 + 0) = msb;

            const double sample = ADC_TO_VOLTS * double(msb);

            //4-point MA for devices without way to disable impedance
            /*
            eeg_sample = ADC_TO_VOLTS * ((double) *(prev_4_samples + j*4 + 3) +
            *(prev_4_samples + j*4 + 2) +
            *(prev_4_samples + j*4 + 1) +
            *(prev_4_samples + j*4 + 0) )/4.0;
            */

            *(dataBufferPtr + j * EEG_GRAB + c) = float(sample);
        }


        //save packet counter;
        *(dataBufferPtr + (CHS - 1) * EEG_GRAB + c) = float(packet_start);

        //dummy data
        grab_SPP(1, &temp);
        grab_SPP(1, &temp);

        //grab trigger
        grab_SPP(1, &temp);
        lsb2 = temp;
        grab_SPP(1, &temp);
        lsb1 = temp;

        //assemble trigger code
        int newTrigger = (lsb2 << 8) | lsb1;
        //downshift serial triggers if necessary
        if (newTrigger > 255) { newTrigger = newTrigger >> 8; }

        //store in EEG channel
        *(dataBufferPtr + (CHS - 2) * EEG_GRAB + c) = float(newTrigger);

        //detect new stimulation
        if (newTrigger != prev_trigger)
        {
            //time offset from start of chunk (c - current sample in the chunk)
            const uint64_t date = c * (1LL << 32) / SAMPLE_RATE;
            cogStimulationSet.push_back(OVTK_StimulationId_Label(newTrigger&0x000000ff), date, 0);
        }

        prev_trigger = newTrigger;
    }
}

double get_IMP_CH(const int ch)
{
    double d1 = double(*(prev_4_samples + ch * 4 + 2)) - double(*(prev_4_samples + ch * 4 + 0));
    double d2 = double(*(prev_4_samples + ch * 4 + 3)) - double(*(prev_4_samples + ch * 4 + 1));
    d1        = d1 * ADC_TO_VOLTS;
    d2        = d2 * ADC_TO_VOLTS;

    if (d1 < 0) { d1 = -d1; }
    if (d2 < 0) { d2 = -d2; }

    if (d2 > d1) { d1 = d2; }

    return d1 * TO_Z;
}

//serial port handling

HANDLE hSerial;
COMMTIMEOUTS timeouts = { 0 };

int init_SPP(const int port)
{
    char com[100];

    sprintf(com, "\\\\.\\COM%d", port);

    hSerial = CreateFile(com, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (hSerial == INVALID_HANDLE_VALUE) { return -1; }

    //setup serial port parameters
    DCB dcbSerialParams = { 0 };

    if (!GetCommState(hSerial, &dcbSerialParams))
    {
        //error getting state
    }

    dcbSerialParams.BaudRate = 1500000;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity   = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams))
    {
        //error setting serial port state
    }

    //setup serial port timeout
    COMMTIMEOUTS timeouts;
    timeouts.ReadIntervalTimeout        = 500;
    timeouts.ReadTotalTimeoutConstant   = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;

    timeouts.WriteTotalTimeoutConstant   = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    if (!SetCommTimeouts(hSerial, &timeouts))
    {
        //error occureed. Inform user
    }
    return 0;
}

void close_SPP() { CloseHandle(hSerial); }

unsigned long grab_SPP(const int bytes, unsigned char* buf)
{
    DWORD dwBytesRead = 0;
    ReadFile(hSerial, buf, bytes, &dwBytesRead, nullptr);

    return int(dwBytesRead);
}

void write_SPP(const int bytes, unsigned char* buf)
{
    DWORD dwBytesWritten = 0;
    WriteFile(hSerial, buf, bytes, &dwBytesWritten, nullptr);
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif //WIN32
