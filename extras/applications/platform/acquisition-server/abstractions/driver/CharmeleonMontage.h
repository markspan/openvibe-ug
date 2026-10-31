#pragma once

/**
 * @file CharmeleonMontage.h
 * @brief Reads Charmeleon montage files and applies them to a driver's channel names.
 *
 * A montage is a JSON object of flat entries, one per electrode:
 * @code
 * {
 *   "Cz":    { "IsActive": true, "LabelText": "Cz",   "HardwareChannel": 16, "Angle": 0, "Radius": 0 },
 *   "Right": { "IsActive": true, "LabelText": "HEOG", "Channel": "BIP1", "X": 0.85, "Y": 0.72 },
 *   "Amplifier": { "Type": "TMSi Refa", "EEGChannels": 64, "BipolarChannels": 4, "AuxiliaryChannels": 4 }
 * }
 * @endcode
 * HardwareChannel n (1-based) is the amplifier's n-th EEG channel. "Channel" names any other channel by
 * kind and number, BIP1, AUX2, ..., counted in the amplifier's own order; it takes precedence over
 * HardwareChannel. Inactive electrodes, and those on channel 0, are not in use. "Amplifier", written by
 * Charmeleon 2.2 and later, describes the amplifier the montage was made for.
 *
 * UG fork: new file, not in upstream OpenViBE. The TMSi and eego drivers share these rules, so that a montage
 * means the same on both; montages.md, at the top of the repository, explains their use.
 */

#include <OVAcquisitionAbstractions_Export.h>

#include <string>
#include <vector>

/// The kinds of amplifier channel a montage can name.
enum class MontageChannelKind { EEG, Bipolar, Auxiliary, Other };

/// Prefix of the channels of a kind in a montage's "Channel" field (BIP, AUX), empty for EEG and Other.
OVACQUISITIONABSTRACTIONS_EXPORT std::string montageChannelPrefix(MontageChannelKind kind);

/// The contents of a montage file.
struct OVACQUISITIONABSTRACTIONS_EXPORT CharmeleonMontage
{
    struct Electrode
    {
        std::string name;
        MontageChannelKind kind = MontageChannelKind::EEG;
        int number = 0;  ///< 1-based, among the channels of its kind
    };

    struct Amplifier
    {
        std::string type;
        int eegChannels       = 0;
        int bipolarChannels   = 0;
        int auxiliaryChannels = 0;
    };

    std::vector<Electrode> electrodes;  ///< The electrodes in use, in file order, each on its own channel
    Amplifier amplifier;                ///< What the montage was made for, when it says
    bool hasAmplifier = false;
    std::vector<std::string> problems;  ///< What is wrong in the file itself, e.g. two electrodes on one channel
};

/**
 * @brief Loads a Charmeleon montage.
 * @param path    Path of the montage file (.json).
 * @param montage Filled with the file's contents.
 * @param error   Set to a description of the problem when the file can not be used.
 * @return False when the file can not be read, is not a montage, or has no electrode in use.
 */
OVACQUISITIONABSTRACTIONS_EXPORT bool loadCharmeleonMontage(const std::string& path, CharmeleonMontage& montage, std::string& error);

/// A montage applied to a channel list.
struct OVACQUISITIONABSTRACTIONS_EXPORT MontageApplication
{
    std::vector<std::string> names;  ///< The channel names, with the montage's
    std::vector<bool> locked;        ///< The entries the montage decides, which the user can not usefully edit
    std::string summary;             ///< One or more sentences for the user
    bool warning = false;            ///< True when the summary reports something the user should look at
};

/**
 * @brief Applies a montage to a driver's channel names.
 *
 * Every EEG channel gets the name of the electrode on it, or none: the montage decides which EEG
 * channels are recorded. A Bipolar or Auxiliary channel the montage names gets that name; the others
 * keep theirs, unless it is the name of one of the montage's electrodes, left from an older montage.
 *
 * @param montage The montage.
 * @param layout  The kind of each channel, in device order. Empty when the amplifier is not connected and
 *                the driver can not tell: every channel is then taken to be EEG, as they come first in the
 *                amplifiers' lists.
 * @param names   The current names, one per channel.
 * @param assumed True when @p layout is what the driver assumes, the amplifier not being connected (e.g.
 *                TMSi: EEG, then 4 BIP and 4 AUX). As with an empty layout, the result is then provisional:
 *                only the entries the montage names are locked, and the summary says so.
 */
OVACQUISITIONABSTRACTIONS_EXPORT MontageApplication applyCharmeleonMontage(const CharmeleonMontage& montage, const std::vector<MontageChannelKind>& layout,
                                                                           std::vector<std::string> names, bool assumed = false);

/// Where a montage setting was found.
struct OVACQUISITIONABSTRACTIONS_EXPORT MontageLocation
{
    enum class Source { AsGiven, CharmeleonFolder, KeptCopy, NotFound };

    std::string path;  ///< The file to read, empty when not found
    Source source = Source::NotFound;
};

/**
 * @brief Finds the file a montage setting stands for.
 *
 * The setting as a path first; then, by its file name, Charmeleon's montage library and its default
 * layouts; last, the copy kept when the setting was last loaded, so that a moved or deleted file does
 * not stop a recording.
 */
OVACQUISITIONABSTRACTIONS_EXPORT MontageLocation locateCharmeleonMontage(const std::string& setting);

/// Keeps a copy of the montage file @p path loaded for @p setting, for locateCharmeleonMontage().
OVACQUISITIONABSTRACTIONS_EXPORT void keepCharmeleonMontageCopy(const std::string& setting, const std::string& path);

/**
 * @brief The montage setting to keep after Charmeleon saved a montage to @p savedPath.
 * @return @p setting when it already stands for that file (as a path, or by name in Charmeleon's library),
 *         else @p savedPath: the montage was saved under another name.
 */
OVACQUISITIONABSTRACTIONS_EXPORT std::string montageSettingAfterSave(const std::string& setting, const std::string& savedPath);

/// Folder Charmeleon is installed in, as its installer recorded it; empty when it is not installed.
OVACQUISITIONABSTRACTIONS_EXPORT std::string charmeleonInstallFolder();

/**
 * @brief Reads the montage a driver setting names and applies it: locate, load, apply and keep a copy.
 * @param setting The driver's Montage setting; must not be empty.
 * @param layout  As for applyCharmeleonMontage().
 * @param names   As for applyCharmeleonMontage().
 * @param result  The application; its summary starts with the montage's file name.
 * @param error   Set when the montage can not be used. @p result is then left as it was.
 * @param assumed As for applyCharmeleonMontage().
 */
OVACQUISITIONABSTRACTIONS_EXPORT bool applyCharmeleonMontageSetting(const std::string& setting, const std::vector<MontageChannelKind>& layout,
                                                                    const std::vector<std::string>& names, MontageApplication& result, std::string& error,
                                                                    bool assumed = false);
