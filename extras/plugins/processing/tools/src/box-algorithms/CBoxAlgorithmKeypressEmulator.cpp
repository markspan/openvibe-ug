///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmKeypressEmulator.cpp
/// \author Jussi T. Lindgren (Inria)
/// \version 0.1.
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

#include "CBoxAlgorithmKeypressEmulator.hpp"

#include <array>

namespace OpenViBE {
namespace Plugins {
namespace Tools {

#if defined(TARGET_OS_Windows)
#include <winuser.h>
#endif

bool CBoxAlgorithmKeypressEmulator::initialize()
{
    m_decoder.initialize(*this, 0);

    m_triggerStimulation = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_keyToPress         = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

#if defined(TARGET_OS_Windows)
    const uint64_t modifier = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
    if (modifier == TypeId_Keypress_Modifier_Shift.id()) { m_modifier = VK_SHIFT; }
    else if (modifier == TypeId_Keypress_Modifier_Control.id()) { m_modifier = VK_CONTROL; }
    else if (modifier == TypeId_Keypress_Modifier_Alt.id()) { m_modifier = VK_MENU; }
#elif defined(TARGET_OS_Linux)
    // @todo implement the whole linux solution
    // Handle modifier on linux here
    getLogManager() << Kernel::LogLevel_Error << "This box is only implemented on Windows for the moment\n";
    return false;
#endif

    return true;
}
/*******************************************************************************/

bool CBoxAlgorithmKeypressEmulator::uninitialize()
{
    m_decoder.uninitialize();
    return true;
}
/*******************************************************************************/

bool CBoxAlgorithmKeypressEmulator::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}
/*******************************************************************************/

bool CBoxAlgorithmKeypressEmulator::process()
{
    const Kernel::IBoxIO& boxCtx = this->getDynamicBoxContext();

    for (size_t i = 0; i < boxCtx.getInputChunkCount(0); ++i) {
        m_decoder.decode(i);

        if (m_decoder.isHeaderReceived()) { }   // NOP
        if (m_decoder.isBufferReceived()) {
            const CStimulationSet* set = m_decoder.getOutputStimulationSet();
            for (size_t s = 0; s < set->size(); ++s) {
                if (set->getId(s) == m_triggerStimulation) {
#if defined(TARGET_OS_Windows)
                    getLogManager() << Kernel::LogLevel_Debug << "Received " << m_triggerStimulation << ", pressing Windows virtual key " << m_keyToPress
                            << " with modifier " << m_modifier << ", delay is at least "
                            << CTime(this->getPlayerContext().getCurrentTime() - set->getDate(s)) << "\n";

                    // @todo the stimulation time is not necessarily RIGHT NOW. A very
                    // accurate solution would call SendInput exactly when the key needs to be pressed. Unfortunately when 
                    // we receive the stim chunk, the keypress is already old, and we can't press keys into the past. In this sense
                    // sending immediately might be the best we can do. 
                    // Note that the speccing the "time" field of INPUT as nonzero doesn't send keypresses to the past or future.
                    if (m_modifier != 0) {
                        std::array<INPUT, 3> keypress;
                        memset(keypress.data(), 0, sizeof(INPUT) * 3);
                        keypress[0].type       = INPUT_KEYBOARD;
                        keypress[0].ki.wVk     = WORD(m_modifier);
                        keypress[1].type       = INPUT_KEYBOARD;
                        keypress[1].ki.wVk     = WORD(m_keyToPress);
                        keypress[2].type       = INPUT_KEYBOARD;
                        keypress[2].ki.wVk     = WORD(m_modifier);
                        keypress[2].ki.dwFlags = KEYEVENTF_KEYUP;
                        SendInput(3, keypress.data(), sizeof(INPUT));
                    }
                    else {
                        INPUT keypress;
                        memset(&keypress, 0, sizeof(INPUT));
                        keypress.type   = INPUT_KEYBOARD;
                        keypress.ki.wVk = WORD(m_keyToPress);
                        SendInput(1, &keypress, sizeof(INPUT));
                    }
#elif defined(TARGET_OS_Linux)
                    // @todo
#endif
                }
            }
        }
        if (m_decoder.isEndReceived()) { }  // NOP
    }
    return true;
}

void CBoxAlgorithmKeypressEmulator::RegisterEnums(const Kernel::IPluginModuleContext& ctx)
{
    ctx.getTypeManager().registerEnumerationType(TypeId_Keypress_Modifier, "Key modifier");
    ctx.getTypeManager().registerEnumerationEntry(TypeId_Keypress_Modifier, "None", TypeId_Keypress_Modifier_None.id());
    ctx.getTypeManager().registerEnumerationEntry(TypeId_Keypress_Modifier, "Shift", TypeId_Keypress_Modifier_Shift.id());
    ctx.getTypeManager().registerEnumerationEntry(TypeId_Keypress_Modifier, "Ctrl", TypeId_Keypress_Modifier_Control.id());
    ctx.getTypeManager().registerEnumerationEntry(TypeId_Keypress_Modifier, "Alt", TypeId_Keypress_Modifier_Alt.id());

    ctx.getTypeManager().registerEnumerationType(TypeId_Keypress_Key, "Key");

#if defined(TARGET_OS_Windows)
    // @note this solution makes the scenario not portable between win<->linux. The challenge is keys such as F1 etc,
    // otherwise we could just read ascii char and map it to a key.
    // @fixme we're also not able to expose all the possible keys to the user this way, but the problem seems to be that 
    // GetKeyNameTextA does not give meaningful & different names to all keys for some reason. 
    // - The current appoarch attempts to expose the usual 'english' keys that can be visualized by pango.
    // - We do not necessarily get arrow keys listed as these may not have a separate representation from 
    //   the corresponding 'numpad' keys with the functions we call here; esp. GetKeyNameTextA seems to
    //   depend on the keyboard in use. This limits the scenario portability across keyboards.
    for (int i = 0; i < 256; ++i) {
        const UINT mapped   = MapVirtualKeyA(UINT(i), MAPVK_VK_TO_VSC);
        const LONG scanCode = LONG(mapped << 16);

        if (scanCode) {
            std::array<CHAR, 512> buffer;
            if (GetKeyNameTextA(scanCode, buffer.data(), 512)) {
                std::string tmp(buffer.data());

                // Some weird characters here, may be difficult to render for pango, ignore this key
                if (tmp.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890_*&$%@^#!-+/\\ :;,.'`~()[]\"=")
                    != std::string::npos) { continue; }

                // GetKeyNameTextA can return the same name for different keys. Only use the first one.
                if (ctx.getTypeManager().getEnumerationEntryValueFromName(TypeId_Keypress_Key, buffer.data()) == CIdentifier::undefined().id()) {
                    ctx.getTypeManager().registerEnumerationEntry(TypeId_Keypress_Key, buffer.data(), i);
                }
            }
        }
    }

#elif defined(TARGET_OS_Linux)
    // @todo register the linux keys to the enum here
#endif
}
}  // namespace Tools
}  // namespace Plugins
}  // namespace OpenViBE
