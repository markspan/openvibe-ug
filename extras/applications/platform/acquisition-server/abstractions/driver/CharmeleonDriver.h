/**
 * SPDX-License-Identifier: AGPL-3.0-only
 * UG fork: new file, not in upstream OpenViBE. See UG.md, 2.3 and 2.41.
 */
#pragma once

#include "AbstractDriver.h"
#include "CharmeleonMontage.h"

#include <OVAcquisitionAbstractions_Export.h>

#include <openvibe/parameters/parameterPath.h>

#include <string>
#include <vector>

/**
 * @brief A driver whose channels a Charmeleon montage names, and whose amplifier Charmeleon can open to check its
 *        impedances.
 *
 * The TMSi and eego drivers derive from this, which holds all they share, so that a montage means the same on both:
 *   - the Montage setting, applied by the rules in CharmeleonMontage, and what it did shown under the settings
 *     (AbstractDriver::note()) with the entries it decides locked (AbstractDriver::channelLocks());
 *   - the channel names list, one entry per amplifier channel; only the channels with a name are recorded, and during
 *     an acquisition the list shows those only;
 *   - the hints in empty entries, and the channel information sent to the clients;
 *   - Charmeleon: which montage it opens, and taking over the montage it saved.
 *
 * A driver supplies its amplifier's channels (deviceChannels()), whether an acquisition holds the amplifier
 * (isAcquiring()), what Charmeleon needs to open it (charmeleonArguments()), and, if it can tell, the channels it
 * assumes while no amplifier is connected (assumedKinds()).
 *
 * AbstractDriver knows nothing of Charmeleon: the acquisition server finds a driver of this kind with dynamic_cast.
 */
class OVACQUISITIONABSTRACTIONS_EXPORT CharmeleonDriver : public AbstractDriver
{
public:
    /// What Charmeleon needs on its command line to open this driver's amplifier, e.g. {"--amplifier", "tmsi", ...}.
    virtual std::vector<std::string> charmeleonArguments() const = 0;

    /// The montage file for Charmeleon to open, found as locateCharmeleonMontage() finds it, so that it opens the file
    /// the driver reads; the setting itself when it is not found.
    std::string charmeleonMontage() const;

    /// Takes over the montage Charmeleon saved: reads it again, under its new name when Charmeleon saved it as another file.
    void takeOverCharmeleonMontage(const std::string& path);

    /// BIP1, BIP2, AUX1, ... for the bipolar and auxiliary channels, the amplifier's name for any other channel that is not EEG.
    std::vector<std::string> channelHints() const override;

    /// The names, and the montage with where each electrode sits, matched to the channels by name.
    ovChannelInfo channelInfo() const override;

protected:
    /**
     * @param logPrefix  Starts what the driver prints, e.g. "[TMSi]".
     * @param deviceName The amplifier as the user knows it, e.g. "the TMSi device", for the note when Play fails.
     */
    CharmeleonDriver(std::string logPrefix, std::string deviceName);

    /// One channel of the amplifier.
    struct DeviceChannel
    {
        MontageChannelKind kind = MontageChannelKind::Other;
        std::string name;  ///< The amplifier's name for it, e.g. "EEG12" or "BIP1"
    };

    /// The amplifier's channels, in its order; empty while they are not known, e.g. no amplifier is connected.
    virtual std::vector<DeviceChannel> deviceChannels() const = 0;

    /**
     * @brief The channels assumed while deviceChannels() is empty, so that the montage's BIP and AUX channels, and the
     *        names typed for them, have a place before the amplifier is seen.
     * @param count The entries of the list.
     * @return One kind per entry, or empty to assume nothing: every entry is then taken for EEG, provisionally.
     */
    virtual std::vector<MontageChannelKind> assumedKinds(size_t /*count*/) const { return {}; }

    /// True while an acquisition holds the amplifier: the list then shows the recorded channels only.
    virtual bool isAcquiring() const = 0;

    /// Brings the channel names list up to date: on the amplifier's channels when they are known, else provisionally.
    void updateChannelList();

    /**
     * @brief Gives the channel names list one entry per amplifier channel, with the montage applied.
     *
     * Names kept for another amplifier, or another number of channels, are dropped: the EEG channels get the
     * amplifier's names, the others none.
     * @return False when the amplifier's channels are not known, or the montage can not be used.
     */
    bool fitNamesToDevice();

    /**
     * @brief The channels to record: those with a name, in the amplifier's order. Keeps the full list for
     *        restoreChannelList().
     * @param names Set to their names.
     * @return Their indices among deviceChannels(); empty, with the reason reported, when no channel has a name.
     */
    std::vector<size_t> namedChannels(std::vector<std::string>& names);

    /// For the duration of the acquisition: the list shows @p names, the recorded channels and any the driver adds,
    /// none locked; the note shows what the montage did.
    void showRecordedChannels(const std::vector<std::string>& names);

    /// After an acquisition: the full list again, as namedChannels() kept it.
    void restoreChannelList();

    /// Prints @p message and keeps it as the reason the attempt to start failed.
    void report(const std::string& message) const;

    /**
     * @brief Ends a failed init(): reports @p message, if any, and shows why Play failed under the settings.
     * @return False, for init() to return.
     */
    bool failInit(const std::string& message = {});

    /// Forgets why a previous attempt failed; for the start of init().
    void clearLastError() { m_lastError.clear(); }

    OpenViBE::Parameters::ParameterPath m_paramMontage;

private:
    /// Applies the montage, if one is set, to @p names, and shows what it did; false when it is set but can not be used.
    bool applyMontage(std::vector<std::string>& names);
    /// Applies a newly chosen montage, or restores the amplifier's names when it is cleared.
    void onMontageChanged();

    const std::string m_logPrefix;
    const std::string m_deviceName;
    Note m_montageNote;               ///< What the montage did, shown again once an acquisition has started
    mutable std::string m_lastError;  ///< Why the last attempt to start failed, see report()
    std::vector<std::string> m_configuredNames;  ///< The full list during an acquisition, see namedChannels()
    std::vector<bool> m_configuredLocks;         ///< The locks that go with it
};
