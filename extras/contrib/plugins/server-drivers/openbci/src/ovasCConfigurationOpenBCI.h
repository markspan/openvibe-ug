/*
 * OpenBCI driver for OpenViBE
 *
 * \author Jeremy Frey
 * \author Yann Renard
 */
#pragma once

#include "../ovasCConfigurationBuilder.h"

#include <gtk/gtk.h>
#include <map>

namespace OpenViBE {
namespace AcquisitionServer {
class CConfigurationOpenBCI final : public CConfigurationBuilder
{
public:

    const static uint16_t DEFAULT_SAMPLING      = 250;  // sampling rate with no daisy module (divided by 2 with daisy module)
    const static uint16_t DEFAULT_N_EEG_CHANNEL = 8;    // number of EEG channels with no daisy module (multiplied by 2 with daisy module)
    const static uint16_t DEFAULT_N_ACC_CHANNEL = 3;    // number of Acc channels (daisy module does not have an impact)

    enum class EDaisyStatus { Active, Inactive };

    typedef struct
    {
        int nEEGChannel;
        int nAccChannel;
        int sampling;
    } daisy_Info_t;

    static uint32_t getMaximumTtyCount();
    static CString getTTYFileName(uint32_t ttyNumber);
    static bool isTTYFile(const CString& filename);

    CConfigurationOpenBCI(const char* gtkBuilderFilename, uint32_t& usbIdx);
    ~CConfigurationOpenBCI() override;

    bool preConfigure() override;
    bool postConfigure() override;

    void setAdditionalCommands(const CString& cmds) { m_additionalCmds = cmds; }
    CString getAdditionalCommands() const { return m_additionalCmds; }
    void setReadBoardReplyTimeout(const uint32_t timeout) { m_readBoardReplyTimeout = timeout; }
    uint32_t getReadBoardReplyTimeout() const { return m_readBoardReplyTimeout; }
    void setFlushBoardReplyTimeout(const uint32_t timeout) { m_flushBoardReplyTimeout = timeout; }
    uint32_t getFlushBoardReplyTimeout() const { return m_flushBoardReplyTimeout; }
    void setDaisyModule(const bool module) { m_daisyModule = module; }
    bool getDaisyModule() const { return m_daisyModule; }

    void checkbuttonDaisyModuleCB(EDaisyStatus status) const;

    static daisy_Info_t getDaisyInformation(EDaisyStatus status);

protected:

    std::map<int, int> m_comboSlotsIndexToSerialPort;
    uint32_t& m_usbIdx;
    GtkListStore* m_listStore = nullptr;
    GtkEntry* m_entryComInit  = nullptr;
    CString m_additionalCmds;
    uint32_t m_readBoardReplyTimeout  = 0;
    uint32_t m_flushBoardReplyTimeout = 0;
    bool m_daisyModule                = false;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
