///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxTCPWriter.hpp
/// \brief Class of the box TCP Writer.
/// \author Jussi T. Lindgren (Inria).
/// \version 1.0.
/// \date 11/09/2013
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#pragma once

#include "defines.hpp"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <ctime>
#include <boost/asio.hpp>

enum { TCPWRITER_RAW, TCPWRITER_HEX, TCPWRITER_STRING }; // stimulation output types

namespace OpenViBE {
namespace Plugins {
namespace NetworkIO {

//--------------------------------------------------------------------------------
/// <summary> The class CBoxTCPWriter describes the box TCP Writer. </summary>
class CBoxTCPWriter final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_TCPWriter)

protected:
    bool sendToClients(const void* buffer, size_t size);

    // Stream decoder
    Toolkit::TStimulationDecoder<CBoxTCPWriter> m_stimulationDecoder;
    Toolkit::TStreamedMatrixDecoder<CBoxTCPWriter> m_matrixDecoder;
    Toolkit::TSignalDecoder<CBoxTCPWriter> m_signalDecoder;
    Toolkit::TDecoder<CBoxTCPWriter>* m_activeDecoder = nullptr;

    boost::asio::io_context m_ioContext;
    boost::asio::ip::tcp::acceptor* m_acceptor = nullptr;
    std::vector<boost::asio::ip::tcp::socket*> m_sockets;

    uint64_t m_outputStyle = 0;

    CIdentifier m_inputType = CIdentifier::undefined();

    // Data written as global output header, 8*4 = 32 bytes. Padding allows dumb readers to step with double (==8 bytes).
    size_t m_rawVersion       = 0;                  // in network byte order, version of the raw stream
    size_t m_endianness       = 0;                  // in network byte order, 0==unknown, 1==little, 2==big, 3==pdp
    size_t m_frequency        = 0;                  // this and the rest are in host byte order
    size_t m_nChannels        = 0;
    size_t m_nSamplesPerChunk = 0;
    size_t m_reserved0        = 0;
    size_t m_reserved1        = 0;
    size_t m_reserved2        = 0;

    void startAccept();
    void handleAccept(const boost::system::error_code& ec, boost::asio::ip::tcp::socket* pSocket);
};

//--------------------------------------------------------------------------------
/// <summary> Listener of the box TCP Writer. </summary>
class CBoxTCPWriterListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    CBoxTCPWriterListener(): m_lastType(CIdentifier::undefined()) { }

    bool onInputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        CIdentifier newType = CIdentifier::undefined();
        box.getInputType(index, newType);
        // Set the right enumeration according to the type if we actualy change it
        // TODO find a way to init m_lastType with the right value
        if (m_lastType != newType) {
            if (newType != OV_TypeId_Stimulations) { box.setSettingType(1, TypeID_TCPWriter_RawOutputStyle); }
            else { box.setSettingType(1, TypeID_TCPWriter_OutputStyle); }
            box.setSettingValue(1, "Raw");
            m_lastType = newType;
        }
        return true;
    }

private:
    CIdentifier m_lastType = CIdentifier::undefined();

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

//--------------------------------------------------------------------------------
/// <summary> Descriptor of the box TCP Writer. </summary>
class CBoxTCPWriterDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "TCP Writer"; }
    CString getAuthorName() const override { return "Jussi T. Lindgren"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Send input stream out via a TCP socket"; }
    CString getDetailedDescription() const override { return "\n"; }
    CString getCategory() const override { return "Acquisition and network IO"; }
    CString getVersion() const override { return "0.2"; }
    CString getStockItemName() const override { return "gtk-connect"; }

    CIdentifier getCreatedClass() const override { return Box_TCPWriter; }
    IPluginObject* create() override { return new CBoxTCPWriter; }

    IBoxListener* createBoxListener() const override { return new CBoxTCPWriterListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input 1",OV_TypeId_StreamedMatrix);

        prototype.addSetting("Port",OV_TypeId_Integer, "5678");
        prototype.addSetting("Output format", TypeID_TCPWriter_RawOutputStyle, "Raw");

        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);

        prototype.addInputSupport(OV_TypeId_StreamedMatrix);
        prototype.addInputSupport(OV_TypeId_Signal);
        prototype.addInputSupport(OV_TypeId_Stimulations);

        prototype.addFlag(OV_AttributeId_Box_FlagIsUnstable);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_TCPWriterDesc)
};
}  // namespace NetworkIO
}  // namespace Plugins
}  // namespace OpenViBE
