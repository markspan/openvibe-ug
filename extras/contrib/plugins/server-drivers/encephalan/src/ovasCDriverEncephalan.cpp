#if defined TARGET_OS_Windows

#include "ovasCDriverEncephalan.h"
#include "ovasCConfigurationEncephalan.h"

namespace OpenViBE {
namespace AcquisitionServer {

//---------------------------------------------------------------------------------------------------
CDriverEncephalan::CDriverEncephalan(IDriverContext& driverContext)
    : IDriver(driverContext), m_settings("AcquisitionServer_Driver_Encephalan", m_driverCtx.getConfigurationManager()), m_connectionIp("127.0.0.1")
{
    m_header.setSamplingFrequency(250);
    m_header.setChannelCount(8);

    m_settings.add("Header", &m_header);
    m_settings.load();
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CDriverEncephalan::initialize(const uint32_t sampleCountPerBlock, IDriverCallback& callback)
{
    if (m_driverCtx.isConnected() || !m_header.isChannelCountSet() || !m_header.isSamplingFrequencySet()) { return false; }

    m_sample = new float[m_header.getChannelCount() * sampleCountPerBlock];
    if (!m_sample) {
        delete[] m_sample;
        m_sample = nullptr;
        return false;
    }

    // Saves parameters
    m_callback        = &callback;
    m_nSamplePerBlock = sampleCountPerBlock;
    return connectEncephalan();
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CDriverEncephalan::start()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    // request hardware to start
    return sendRequestForData();
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CDriverEncephalan::loop()
{
    if (!m_driverCtx.isConnected()) { return false; }
    if (!m_driverCtx.isStarted()) { return true; }

    m_currentPoint = 0;
    ZeroMemory(m_sample, sizeof(float)*m_nSamplePerBlock*m_header.getChannelCount());
    while (m_currentPoint < m_nSamplePerBlock) { receiveData(); }
    m_callback->setSamples(m_sample);

    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CDriverEncephalan::stop() { return m_driverCtx.isConnected() && m_driverCtx.isStarted(); }
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CDriverEncephalan::uninitialize()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    delete[] m_sample;
    m_sample   = nullptr;
    m_callback = nullptr;

    closesocket(m_client);
    WSACleanup();

    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CDriverEncephalan::connectEncephalan()
{
    WSADATA wsaData;
    const int wsaret = WSAStartup(0x101, &wsaData);
    if (wsaret != 0) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Error WSAStartup (initialization windows socket api): " << WSAGetLastError() << "\n";
        return false;
    }

    m_client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_client == INVALID_SOCKET) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Error socket (creation a socket): " << WSAGetLastError() << "\n";
        WSACleanup();
        return false;
    }

    sockaddr_in sockaddrIn;
    sockaddrIn.sin_family      = AF_INET;
    sockaddrIn.sin_addr.s_addr = inet_addr(m_connectionIp.c_str());
    sockaddrIn.sin_port        = htons(u_short(m_connectionPort));
    const int connectError     = connect(m_client, reinterpret_cast<sockaddr*>(&sockaddrIn), sizeof(sockaddrIn));
    if (connectError != 0) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Error connect (connection to a specified socket): " << WSAGetLastError() << "\n";
        return false;
    }
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CDriverEncephalan::sendRequestForData() const
{
    BYTE* sendDataBuffer = new BYTE;

    const uint32_t id = 0x0001; //EEG_ONLY_MODE
    //const uint32_t id = 0x000C; //ALL_CHANNELS_MODE
    //const uint32_t id = 0x000E; //HD_CHANNELS_MODE

    CopyMemory(sendDataBuffer, &id, sizeof(id));
    if (!sendData(sendDataBuffer, sizeof(id))) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Error sendRequestForData: " << WSAGetLastError() << "\n";
        return false;
    }
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "sendRequestForData\n";
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CDriverEncephalan::sendData(BYTE* data, int dataSize) const
{
    if (m_client == INVALID_SOCKET) { return false; }

    const long fullLen = dataSize + long(sizeof(int));
    char* outData      = new char[fullLen];

    CopyMemory(&outData[0], &dataSize, sizeof(int));
    CopyMemory(&outData[sizeof(int)], &data[0], dataSize);

    if (send(m_client, outData, fullLen, 0) != fullLen) {
        delete[] outData;
        return false;
    }

    delete[]outData;
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CDriverEncephalan::receiveData()
{
    int inpSize         = 0;
    char* inpData       = new char[sizeof(inpSize) * sizeof(BYTE)];
    const int nReadSize = recv(m_client, inpData, sizeof(inpSize), 0);
    if (nReadSize <= 0) { return false; }

    CopyMemory(&inpSize, &inpData[0], sizeof(inpSize));
    delete[]inpData;

    if ((inpSize > 0) && (inpSize < 1024 * 512)) {
        //uint32_t ID = 0;
        inpData = new char[inpSize * sizeof(BYTE)];
        if (recv(m_client, inpData, inpSize, 0) == inpSize) { readData(reinterpret_cast<BYTE*>(inpData), inpSize); }
        delete[]inpData;
        inpData = nullptr;
    }
    else { return false; }
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
void CDriverEncephalan::readData(BYTE* data, const int dataSize)
{
    BYTE* pCurData  = data;
    int curDataSize = dataSize;

    uint32_t id = 0;
    getData(pCurData, curDataSize, &id, sizeof(id));
    switch (id) {
        case 0x0002: m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Recieved information on research\n";
            break;
        case 0x00E2: m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Recieved all the data including the kilohertz\n";
            break;
        case 0x0003: m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "0x0003 data package\n";
            break;
        case 0x0006: m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "0x0006 data package\n";
            break;
        case 0x0008: receiveEEGData(pCurData, curDataSize); // We came signal data
            break;
        case 0x00E8: m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Packet came with HD data\n";
            break;
        case 0x0009: m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "0x0009 data package\n";
            break;
        case 0x000A: m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "0x000A data package\n";
            break;
        case 0x0101: m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "0x0101 data package\n";
            break;
        default: m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Undefined data package\n";
    }
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
// Parsing package with EEG data
void CDriverEncephalan::receiveEEGData(BYTE* curData, int curDataSize)
{
    int sliceNum     = 0;
    int pointsNumber = 0;
    getData(curData, curDataSize, &sliceNum, sizeof(sliceNum));
    getData(curData, curDataSize, &pointsNumber, sizeof(pointsNumber));

    short* dstData = new short[pointsNumber];
    ZeroMemory(dstData, sizeof(short)*pointsNumber);
    getData(curData, curDataSize, dstData, pointsNumber * int(sizeof(short)));

    for (size_t iPos = 0; iPos < m_header.getChannelCount(); ++iPos) {
        float val = float(dstData[iPos]);
        CopyMemory(m_sample + (m_currentPoint + iPos * m_nSamplePerBlock), &val, sizeof(float));
    }
    m_currentPoint++;

    delete[] dstData;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
void CDriverEncephalan::getData(BYTE* & data, int& dataSize, void* dstData, const int dstSize)
{
    if (dataSize < dstSize) { return; }
    CopyMemory(dstData, data, dstSize);
    data += dstSize;
    dataSize -= dstSize;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CDriverEncephalan::configure()
{
    // Change this line if you need to specify some references to your driver attribute that need configuration, e.g. the connection ID.
    CConfigurationEncephalan configuration(m_driverCtx, Directories::getDataDir() + "/applications/acquisition-server/interface-Encephalan.ui",
                                           m_connectionPort, m_connectionIp);

    if (!configuration.configure(m_header)) { return false; }
    m_connectionIp = configuration.getConnectionIp();
    m_settings.save();

    return true;
}
//---------------------------------------------------------------------------------------------------

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_OS_Windows
