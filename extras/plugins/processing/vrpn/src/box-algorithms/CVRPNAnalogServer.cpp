///-------------------------------------------------------------------------------------------------
/// 
/// \file CVRPNAnalogServer.cpp
/// \brief Classes implementation for the Box VRPN Analog Server.
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

#include "CVRPNAnalogServer.hpp"
#include "../IVRPNServerManager.hpp"

namespace OpenViBE {
namespace Plugins {
namespace VRPN {

bool CVRPNAnalogServer::initialize()
{
    const size_t nInput = this->getStaticBoxContext().getInputCount();

    // Gets server name, and creates an analog server for this server
    const CString name = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);

    // Creates the stream decoders
    for (size_t i = 0; i < nInput; ++i) {
        m_decoders[i] = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StreamedMatrixDecoder));
        m_decoders[i]->initialize();
    }

    // Creates the peripheral
    IVRPNServerManager::GetInstance().Initialize();
    IVRPNServerManager::GetInstance().AddServer(name, m_serverID);

    // We don't know the analog count yet before we get a header, so set to zero.
    // This convention will avoid client problems that misleadingly report 'no response from server'.
    // The client should check that the channel amount is positive and use that as
    // an indication of the box having received at least a proper header.
    IVRPNServerManager::GetInstance().SetAnalogCount(m_serverID, 0);
    IVRPNServerManager::GetInstance().ReportAnalog(m_serverID);

    return true;
}

bool CVRPNAnalogServer::uninitialize()
{
    const size_t nInput = this->getStaticBoxContext().getInputCount();

    // Releases decoders
    for (size_t i = 0; i < nInput; ++i) {
        m_decoders[i]->uninitialize();
        this->getAlgorithmManager().releaseAlgorithm(*m_decoders[i]);
    }
    m_decoders.clear();
    m_nAnalogs.clear();

    // Releases the peripheral
    IVRPNServerManager::GetInstance().Uninitialize();

    return true;
}

bool CVRPNAnalogServer::processClock(Kernel::CMessageClock& /*msg*/)
{
    // Note: This call doesn't seem to be necessary for VRPN sending the
    // data with reportAnarog(). Its utility is likely in keeping the
    // connection functional during periods with no data.
    IVRPNServerManager::GetInstance().Process();
    return true;
}

bool CVRPNAnalogServer::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CVRPNAnalogServer::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    const size_t nInput        = this->getStaticBoxContext().getInputCount();

    for (size_t i = 0; i < nInput; ++i) {
        for (size_t j = 0; j < boxContext.getInputChunkCount(i); ++j) {
            Kernel::TParameterHandler<const CMemoryBuffer*> ip_buffer(
                m_decoders[i]->getInputParameter(OVP_GD_Algorithm_StreamedMatrixDecoder_InputParameterId_MemoryBufferToDecode));
            Kernel::TParameterHandler<CMatrix*> op_matrix(m_decoders[i]->getOutputParameter(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputParameterId_Matrix));

            ip_buffer = boxContext.getInputChunk(i, j);
            m_decoders[i]->process();

            CMatrix* matrix = op_matrix;

            if (m_decoders[i]->isOutputTriggerActive(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputTriggerId_ReceivedHeader)) {
                m_nAnalogs[i] = matrix->getBufferElementCount();
                if (m_nAnalogs.size() == nInput) {
                    size_t nAnalog = 0;
                    for (size_t k = 0; k < nInput; ++k) { nAnalog += m_nAnalogs[k]; }

                    if (IVRPNServerManager::GetInstance().SetAnalogCount(m_serverID, nAnalog)) {
                        this->getLogManager() << Kernel::LogLevel_Trace << "Created VRPN analog server for " << nAnalog << " channel(s)\n";
                    }
                    else {
                        this->getLogManager() << Kernel::LogLevel_Error << "Failed to create VRPN analog server for " << nAnalog << " channel(s)\n";
                        return false;
                    }

                    m_analogSet = true;
                }
                boxContext.markInputAsDeprecated(i, j);
            }
            if (m_analogSet) {
                if (m_decoders[i]->isOutputTriggerActive(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputTriggerId_ReceivedBuffer)) {
                    if (m_analogSet) {
                        size_t offset = 0;
                        for (size_t k = 0; k < i; ++k) { offset += m_nAnalogs[k]; }
                        for (size_t k = 0; k < matrix->getBufferElementCount(); ++k) {
                            if (!IVRPNServerManager::GetInstance().SetAnalogState(m_serverID, offset + k, matrix->getBuffer()[k])) {
                                getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Warning <<
                                        "Could not set analog state for index " << k << "\n";
                            }
                        }
                        IVRPNServerManager::GetInstance().ReportAnalog(m_serverID);
                    }
                }
                if (m_decoders[i]->isOutputTriggerActive(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputTriggerId_ReceivedEnd)) { }
                boxContext.markInputAsDeprecated(i, j);
            }
        }
    }

    return true;
}

}  // namespace VRPN
}  // namespace Plugins
}  // namespace OpenViBE
