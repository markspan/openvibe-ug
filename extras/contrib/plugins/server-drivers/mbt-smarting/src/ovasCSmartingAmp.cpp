#include "ovasCSmartingAmp.h"

#include <system/ovCTime.h>
#include <boost/bind.hpp>

#include <thread>
#include <functional>
#include <iostream>

#define CHANNEL_SCALAR_FACTOR 2.235174445530706e-2
#define GYRO_SCALAR_FACTOR 0.00875

SmartingAmp::SmartingAmp()
{
    m_port.reset(new boost::asio::serial_port(m_io));
    m_timer.reset(new boost::asio::deadline_timer(m_io));
}

void SmartingAmp::disconnect()
{
    if (!m_port->is_open()) { return; }
    m_port->close();
}

void SmartingAmp::write(const unsigned char* data, const size_t& size) const
{
    boost::system::error_code code;
    m_port->write_some(boost::asio::buffer(data, size), code);
}

int SmartingAmp::read(unsigned char* data, const size_t size) const
{
    boost::system::error_code code;
    return m_port->read_some(boost::asio::buffer(data, size), code);
}

void SmartingAmp::acquire()
{
    if (m_port.get() == nullptr || !m_port->is_open()) { return; }

    m_port->async_read_some(boost::asio::buffer(m_receiveBuffer, MAX_PACKAGE_SIZE),
                            boost::bind(&SmartingAmp::onReceive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void SmartingAmp::onReceive(const boost::system::error_code& ec, const size_t bytesTransferred)
{
    std::lock_guard<std::mutex> m(m_on_receive_lock);
    if (m_port.get() == nullptr || !m_port->is_open()) { return; }

    if (ec)
    {
        //acquire();
        return;
    }

    for (uint32_t i = 0; i < bytesTransferred; ++i)
    {
        if (!m_byteArray.empty())
        {
            m_byteArray.push_back(m_receiveBuffer[i]);
            if (m_byteArray.size() == 83)
            {
                if (m_byteArray[82] == '<')
                {
                    float* sample = convert_data(m_byteArray);
                    m_byteArray.clear();
                    m_receivedSamples++;

                    m_samples_lock.lock();
                    m_samplesBuffer.push(sample);
                    m_samples_lock.unlock();
                }
                else
                {
                    m_failedSamples++;
                    m_byteArray.clear();
                }
            }
        }

        if (m_byteArray.empty() && m_receiveBuffer[i] == '>') { m_byteArray.push_back(m_receiveBuffer[i]); }
    }
    acquire();
}

std::pair<unsigned char*, int> SmartingAmp::make_command(const Command command)
{
    unsigned char* cmd;
    std::pair<unsigned char*, size_t> out;
    switch (command)
    {
        case ON: cmd = new unsigned char[4];
            cmd[0] = '>';
            cmd[1] = 'O';
            cmd[2] = 'N';
            cmd[3] = '<';
            out    = std::make_pair(cmd, 4);
            break;
        case OFF: cmd = new unsigned char[5];
            cmd[0] = '>';
            cmd[1] = 'O';
            cmd[2] = 'F';
            cmd[3] = 'F';
            cmd[4] = '<';
            out    = std::make_pair(cmd, 5);
            break;
        case TEST: cmd = new unsigned char[6];
            cmd[0] = '>';
            cmd[1] = 'T';
            cmd[2] = 'E';
            cmd[3] = 'S';
            cmd[4] = 'T';
            cmd[5] = '<';
            out    = std::make_pair(cmd, 6);
            break;
        case NORMAL: cmd = new unsigned char[8];
            cmd[0] = '>';
            cmd[1] = 'N';
            cmd[2] = 'O';
            cmd[3] = 'R';
            cmd[4] = 'M';
            cmd[5] = 'A';
            cmd[6] = 'L';
            cmd[7] = '<';
            out    = std::make_pair(cmd, 8);
            break;
        case SELECT_CHANNELS: cmd = new unsigned char[8];
            cmd[0] = '>';
            cmd[1] = 'S';
            cmd[2] = 'C';
            cmd[3] = ';';
            cmd[4] = 0xFF;
            cmd[5] = 0xFF;
            cmd[6] = 0xFF;
            cmd[7] = '<';
            out    = std::make_pair(cmd, 8);
            break;
        case REFON: cmd = new unsigned char[7];
            cmd[0] = '>';
            cmd[1] = 'R';
            cmd[2] = 'E';
            cmd[3] = 'F';
            cmd[4] = 'O';
            cmd[5] = 'N';
            cmd[6] = '<';
            out    = std::make_pair(cmd, 7);
            break;
        case REFOFF: cmd = new unsigned char[8];
            cmd[0] = '>';
            cmd[1] = 'R';
            cmd[2] = 'E';
            cmd[3] = 'F';
            cmd[4] = 'O';
            cmd[5] = 'F';
            cmd[6] = 'F';
            cmd[7] = '<';
            out    = std::make_pair(cmd, 8);
            break;
        case IMPON: cmd = new unsigned char[7];
            cmd[0] = '>';
            cmd[1] = 'I';
            cmd[2] = 'M';
            cmd[3] = 'P';
            cmd[4] = 'O';
            cmd[5] = 'N';
            cmd[6] = '<';
            out    = std::make_pair(cmd, 7);
            break;
        case IMPOFF: cmd = new unsigned char[8];
            cmd[0] = '>';
            cmd[1] = 'I';
            cmd[2] = 'M';
            cmd[3] = 'P';
            cmd[4] = 'O';
            cmd[5] = 'F';
            cmd[6] = 'F';
            cmd[7] = '<';
            out    = std::make_pair(cmd, 8);
            break;
        case FREQUENCY_250: cmd = new unsigned char[5];
            cmd[0] = '>';
            cmd[1] = '2';
            cmd[2] = '5';
            cmd[3] = '0';
            cmd[4] = '<';
            out    = std::make_pair(cmd, 5);
            break;
        case FREQUENCY_500: cmd = new unsigned char[5];
            cmd[0] = '>';
            cmd[1] = '5';
            cmd[2] = '0';
            cmd[3] = '0';
            cmd[4] = '<';
            out    = std::make_pair(cmd, 5);
            break;
        case NOISE: cmd = new unsigned char[7];
            cmd[0] = '>';
            cmd[1] = 'N';
            cmd[2] = 'O';
            cmd[3] = 'I';
            cmd[4] = 'S';
            cmd[5] = 'E';
            cmd[6] = '<';
            out    = std::make_pair(cmd, 7);
            break;
    }

    return out;
}

CommandStatus SmartingAmp::send_command(const Command cmd)
{
    const std::pair<unsigned char*, size_t> newCmd = make_command(cmd);
    write(newCmd.first, newCmd.second);

    unsigned char* response = new unsigned char[OK_RESPONSE_SIZE];
    const int readed        = read(response, OK_RESPONSE_SIZE);
    return command_status(response, readed);
}

bool SmartingAmp::connect(std::string& portName)
{
    boost::system::error_code errorCode;
    m_port->open(portName, errorCode);
    if (errorCode)
    {
        std::cout << "error : port->open() failed...com_port_name = " << portName << ", e=" << errorCode.message() << std::endl;
        disconnect();
        // m_port->open(portName, error_code);
        return false;
    }
    try
    {
        m_port->set_option(boost::asio::serial_port_base::baud_rate(921600));
        m_port->set_option(boost::asio::serial_port_base::character_size(8));
        m_port->set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
        m_port->set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
        m_port->set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::hardware));
    }
    catch (const std::exception&)
    {
        disconnect();
        return false;
    }

    // NOTE: After connection is established wait 3s and then you can start using the amp 
    System::Time::sleep(3000);

    // NOTE: Leaving MAX_PORT_SIZE bytes to read for the first time because we are not sure
    // how many bytes are in serial port buffer
    read_with_timeout(MAX_PORT_SIZE, 3000);
    if (m_bytes_readed > 0) { if (m_commandReceiveBuffer[m_bytes_readed - 1] == 'g') { if (send_command(SELECT_CHANNELS) == Success) { return true; } } }
    disconnect();
    return false;
}

CommandStatus SmartingAmp::command_status(const unsigned char* response, const int readed)
{
    if (readed < 4) { return Failure; }

    char* msg = new char[5];
    msg[0]    = response[readed - 4];
    msg[1]    = response[readed - 3];
    msg[2]    = response[readed - 2];
    msg[3]    = response[readed - 1];
    msg[4]    = '\0';

    if (!strcmp(msg, ">OK<")) { return Success; }

    // TODO: Implement timeout expired method
    return Failure;
}

bool SmartingAmp::start()
{
    send_command(ON);

    m_samplesBuffer = std::queue<float*>();

    m_failedSamples   = 0;
    m_receivedSamples = 0;

    // start reading

    acquire();

    auto f = [this]() { this->m_io.run(); };
    // auto f = boost::bind(&boost::asio::io_service::run, &m_io);

    acquire_t.reset(new std::thread(f));

    return true;
}

float* SmartingAmp::get_sample()
{
    if (!m_samplesBuffer.empty())
    {
        m_samples_lock.lock();
        float* sample = m_samplesBuffer.front();
        m_samplesBuffer.pop();
        m_samples_lock.unlock();
        return sample;
    }
    return nullptr;
}

bool SmartingAmp::stop()
{
    // NOTE: Stop would rarely return SUCCESS
    off();

    try
    {
        // cancel all async operations
        m_io.stop();
        m_io.reset();

        // TODO: Implement proper stopping of the acquiring thread      
    }
    catch (std::exception&)
    {
        // to do
    }

    // NOTE: Currently, I am very unhappy with flushing
    // It slows down everything and after 6 play/stops open vibe is blocked
    // flush();

    //std::cout << "Failed " << m_failedSamples << std::endl;
    //std::cout << "Received " << m_receivedSamples << std::endl;

    return false;
}

float* SmartingAmp::convert_data(std::vector<unsigned char> in)
{
    float* converted = new float[30];

    // channel data
    for (int i = 0; i < 24; ++i) { converted[i] = get_channel_value(in[3 * i + 1], in[3 * i + 2], in[3 * i + 3]); }

    // gyroX
    converted[24] = get_gyro_value(in[74], in[75]);
    // gyroY
    converted[25] = get_gyro_value(in[76], in[77]);
    // gyroZ
    converted[26] = get_gyro_value(in[78], in[79]);

    // counter
    converted[27] = float(in[73]);

    // battery
    converted[28] = float(in[80] & 0x7F);//add baterry data, removing highest bit, because it's used for impedance measurement
    converted[29] = in[81]; //add checksum data

    return converted;
}

float SmartingAmp::get_channel_value(const unsigned char first, const unsigned char second, const unsigned char third)
{
    const int firstByte  = int(first) & 0xff;
    const int secondByte = int(second) & 0xff;
    const int thirdByte  = int(third) & 0xff;

    int channelValue = (firstByte << 16) + (secondByte << 8) + thirdByte;

    // Checking if value is positive or negative
    if (channelValue > 0x007FFFFF) { channelValue = channelValue - 0x01000000; }

    return float(channelValue) * float(CHANNEL_SCALAR_FACTOR);
}

float SmartingAmp::get_gyro_value(const unsigned char first, const unsigned char second)
{
    const int firstByte  = int(first) & 0xff;
    const int secondByte = int(second) & 0xff;

    int out = (firstByte << 8) + secondByte;

    if (out > 0x00007FFF) { out = out - 0x00008000; }
    else { out = out + 0x00008000; }

    return float(out) * float(GYRO_SCALAR_FACTOR);
}

void SmartingAmp::read_with_timeout(const int size, const size_t timeout)
{
    // Asynchronously read.
    if (m_port.get() == nullptr || !m_port->is_open()) { return; }

    m_bytes_readed = 0;

    m_port->async_read_some(boost::asio::buffer(m_commandReceiveBuffer, size),
                            boost::bind(&SmartingAmp::read_complete, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));


    // Setup a deadline time to implement our timeout.
    m_timer->expires_from_now(boost::posix_time::milliseconds(timeout));
    m_timer->async_wait(boost::bind(&SmartingAmp::timeout_expired, this, boost::asio::placeholders::error));

    // This will block until a character is read
    // or until it is cancelled.
    m_io.run();

    m_io.stop();
    // After a timeout & cancel it seems we need
    // to do a reset for subsequent reads to work.
    m_io.reset();
}

// Called when the timer's deadline expires.
void SmartingAmp::timeout_expired(const boost::system::error_code& /*error*/) { m_io.stop(); }

// Called when an async read completes or has been cancelled
void SmartingAmp::read_complete(const boost::system::error_code& error, const size_t bytesTransferred)
{
    // IMPORTANT NOTE: when timer expires, and port is canceled, read complete is 
    // called for the last time. Its extremly important to cancel the timer even though
    // it has already expired. So, here it goes
    if (error) { return; }

    m_bytes_readed = bytesTransferred;
    m_timer->cancel();
}

void SmartingAmp::off()
{
    const std::pair<unsigned char*, size_t> transformedCmd = make_command(OFF);
    write(transformedCmd.first, transformedCmd.second);
}
