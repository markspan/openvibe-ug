/*
 * Receives data from one or two OpenViBE TCPWriter boxes, a signal writer and a stimulus writer.
 *
 * The code reads the two corresponding sockets asynchronously. With signal, the header contents
 * are printed first. If a stimulus is received, the code will print out the stimulus
 * and how many bytes of signal have been read the meanwhile. In order not to make this
 * code more complicated by mutexes, the signal thread does not try to print anything to cout.
 * Instead, it either discards the data or writes it to a file (if name has been given)
 *
 * This code is meant to be used with the corresponding scenario 'tcpwriter.xml'
 *
 * \author Jussi T. Lindgren (Inria)
 * \date 20 May 11 12:55:22 2014
 *
 */

#include <iostream>
#include <fstream>
#include <cstdlib>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

using boost::asio::ip::tcp;

/*
 * \class TCPWriterClient
 * \brief Basic illustratation of how to read from TCPWriter using boost::asio
 */
class TCPWriterClient
{
protected:
    tcp::socket m_stimulusSocket;
    tcp::socket m_signalSocket;

    static const int HEADER_SIZE = 32;                          // signal header size
    int m_bufferSize             = 0;                           // signal buffer size

    std::array<char, HEADER_SIZE> m_signalHeaderBuffer;
    std::vector<char> m_signalBuffer;

    uint64_t m_signalBytesRead = 0;                 // how many bytes of signal read so far

    boost::asio::streambuf m_stimulusStream;

    std::ofstream m_dataFile;

public:
    ~TCPWriterClient() { if (m_dataFile.is_open()) { m_dataFile.close(); } }

    TCPWriterClient(boost::asio::io_service& ioService, const char* address, const char* signalPort, const char* stimulusPort, const char* dataFilename)
        : m_stimulusSocket(ioService), m_signalSocket(ioService)
    {
        boost::system::error_code error;
        tcp::resolver resolver(ioService);

        if (dataFilename != nullptr) { m_dataFile.open(dataFilename, std::ios::binary | std::ios::trunc | std::ios::out); }

        if ((stimulusPort != nullptr) && strcmp(stimulusPort, "0") != 0) {
            // Stimulus port
            std::cout << "Connecting to stimulus port [" << address << " : " << stimulusPort << "]\n";
            const tcp::resolver::query query = tcp::resolver::query(tcp::v4(), address, stimulusPort);
            m_stimulusSocket.connect(*resolver.resolve(query), error);

            // Tell ASIO to read a stimulus
            async_read_until(m_stimulusSocket, m_stimulusStream, "\n", boost::bind(&TCPWriterClient::StimulusHandler, this, boost::asio::placeholders::error));
        }

        if ((signalPort != nullptr) && strcmp(signalPort, "0") != 0) {
            // Signal port
            std::cout << "Connecting to signal port [" << address << " : " << signalPort << "]\n";
            const tcp::resolver::query query = tcp::resolver::query(tcp::v4(), address, signalPort);
            m_signalSocket.connect(*resolver.resolve(query), error);

            const tcp::no_delay noDelay(true);
            m_signalSocket.set_option(noDelay);

            // Tell ASIO to read the signal header
            async_read(m_signalSocket, boost::asio::buffer(m_signalHeaderBuffer.data(), HEADER_SIZE), boost::asio::transfer_at_least(HEADER_SIZE),
                       boost::bind(&TCPWriterClient::SignalHeaderHandler, this, boost::asio::placeholders::error));
        }
    }


    void StimulusHandler(const boost::system::error_code& error)
    {
        if (error == boost::asio::error::eof) { return; }           // Connection closed cleanly by peer.
        if (error.value() != 0) { throw boost::system::system_error(error); }   // Some other error.

        // std::cout.write(stimulusBuffer.data(), strlen(stimulusBuffer.data()));
        std::cout << "(" << m_signalBytesRead << " bytes of signal read), stimulus: " << &m_stimulusStream;

        // Tell ASIO to read again
        async_read_until(m_stimulusSocket, m_stimulusStream, "\n", boost::bind(&TCPWriterClient::StimulusHandler, this, boost::asio::placeholders::error));
    }

    void SignalHeaderHandler(const boost::system::error_code& error)
    {
        if (error == boost::asio::error::eof) { return; }           // Connection closed cleanly by peer.
        if (error.value() != 0) { throw boost::system::system_error(error); }   // Some other error.

        std::cout << "Received signal header\n";

        const char* buf = m_signalHeaderBuffer.data();

        // the print assumes the stream is little endian

        std::cout << "  Version:    " << ntohl(*(uint32_t*)&buf[0]) << "\n";
        std::cout << "  Endianness: " << ntohl(*(uint32_t*)&buf[4]) << "\n";
        std::cout << "  Hz:         " << *(uint32_t*)&buf[8] << "\n";
        std::cout << "  Channels:   " << *(uint32_t*)&buf[12] << "\n";
        std::cout << "  nSamples:   " << *(uint32_t*)&buf[16] << "\n";

        if (m_dataFile.is_open()) {
            m_dataFile.write(buf, HEADER_SIZE);
            m_dataFile.flush();
        }

        std::cout << "Will now read signal in background.\n";
        if (m_stimulusSocket.is_open()) { std::cout << "Stimulations will be printed when received...\n"; }

        const size_t nChannels = size_t(*(uint32_t*)&buf[12]);
        const size_t nSamples  = size_t(*(uint32_t*)&buf[16]);

        // Note that the buffer size should be the size of the chunk, or the read function may not launch the callback
        // before the chunk has been filled. This can cause delays with sparse streams like classifier outputs.
        m_bufferSize = int(nSamples * nChannels * sizeof(double));
        m_signalBuffer.resize(m_bufferSize);

        // Tell ASIO to read the actual signal
        async_read(m_signalSocket, boost::asio::buffer(m_signalBuffer.data(), m_bufferSize), boost::asio::transfer_at_least(m_bufferSize),
                   boost::bind(&TCPWriterClient::SignalHandler, this, boost::asio::placeholders::error));
    }

    void SignalHandler(const boost::system::error_code& error)
    {
        if (error == boost::asio::error::eof) { return; }                           // Connection closed cleanly by peer.
        if (error.value() != 0) { throw boost::system::system_error(error); }   // Some other error.

        // signalBuffer will now contain bufferSize/sizeof(double) amount of samples in doubles. Here we just discard the data.
        const char* buf = m_signalBuffer.data();
        //const double* signalData = reinterpret_cast<const double*>(buf); Use this to fill your data matrix with the signal...

        if (m_dataFile.is_open()) {
            m_dataFile.write(buf, m_bufferSize);
            m_dataFile.flush();
        }

        m_signalBytesRead += m_bufferSize;

        // Tell ASIO to read more
        async_read(m_signalSocket, boost::asio::buffer(m_signalBuffer.data(), m_bufferSize), boost::asio::transfer_at_least(m_bufferSize),
                   boost::bind(&TCPWriterClient::SignalHandler, this, boost::asio::placeholders::error));
    }
};

int main(int argc, char** argv)
{
    try {
        if (argc > 1 && ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0) || (argc != 4 && argc != 5))) {
            std::cout << "Usage: " << argv[0] << " ip_address signal_port stimulus_port [signal_file]" << std::endl;
            std::cout << "If no argument is given, the program tries to communicate on localhost using default port value." << std::endl;
            std::cout << "Ports that are 0 will be skipped. Optional filename can be specified to write the signal to." << std::endl;
            return EXIT_SUCCESS;
        }

        const char* hostname     = "localhost";
        const char* signalPort   = "5678";
        const char* stimulusPort = "5679";
        const char* filename     = nullptr;

        if (argc >= 4) {
            hostname     = argv[1];
            signalPort   = argv[2];
            stimulusPort = argv[3];
        }
        if (argc == 5) { filename = argv[4]; }

        std::cout << "This example is intended to be used together with the tcp-writer.xml tutorial.\n\n";
        boost::asio::io_service ioService;
        TCPWriterClient client(ioService, hostname, signalPort, stimulusPort, filename);
        ioService.run();
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
