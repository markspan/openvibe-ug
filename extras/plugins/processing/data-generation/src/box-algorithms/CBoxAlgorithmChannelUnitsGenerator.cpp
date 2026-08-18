/*
 * Generates a channel units stream with user-specified unit and factor
 */
#include "CBoxAlgorithmChannelUnitsGenerator.hpp"

namespace OpenViBE {
namespace Plugins {
namespace DataGeneration {

bool CChannelUnitsGenerator::initialize()
{
    m_headerSent = false;

    m_nChannel = size_t(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0)));
    m_unit     = size_t(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1)));
    m_factor   = size_t(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2)));

    m_encoder.initialize(*this, 0);

    return true;
}

bool CChannelUnitsGenerator::uninitialize()
{
    m_encoder.uninitialize();
    return true;
}

bool CChannelUnitsGenerator::processClock(Kernel::CMessageClock& /*msg*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CChannelUnitsGenerator::process()
{
    Kernel::IBoxIO* boxContext = getBoxAlgorithmContext()->getDynamicBoxContext();

    if (!m_headerSent) {
        CMatrix* units = m_encoder.getInputMatrix();
        units->resize(m_nChannel, 2);
        units->setDimensionLabel(1, 0, "Unit");
        units->setDimensionLabel(1, 1, "Factor");

        for (size_t i = 0; i < m_nChannel; ++i) {
            units->getBuffer()[i * 2 + 0] = double(m_unit);
            units->getBuffer()[i * 2 + 1] = double(m_factor);
            units->setDimensionLabel(0, i, ("Channel " + std::to_string(i + 1)).c_str());
        }

        m_encoder.encodeHeader();
        boxContext->markOutputAsReadyToSend(0, 0, 0);
        m_encoder.encodeBuffer();

        boxContext->markOutputAsReadyToSend(0, 0, 0);

        m_headerSent = true;
    }

    return true;
}
}  // namespace DataGeneration
}  // namespace Plugins
}  // namespace OpenViBE
