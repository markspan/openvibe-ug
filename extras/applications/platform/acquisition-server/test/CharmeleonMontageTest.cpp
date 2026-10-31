/**
 * @file CharmeleonMontageTest.cpp
 * @brief Tests of the Charmeleon montage rules the TMSi and eego drivers share.
 *
 * UG fork: new file, not in upstream OpenViBE.
 */

#include <CharmeleonMontage.h>

#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

using Kind = MontageChannelKind;

/// A folder of its own per test, removed afterwards; also the home of the kept montage copies.
class CharmeleonMontageTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        m_folder         = fs::temp_directory_path() / ("ov-montage-test-" + std::to_string(stamp));
        fs::create_directories(m_folder);
        // The kept copies go to %LOCALAPPDATA% (Windows) or $HOME (elsewhere): point both into the test's folder
#if defined(_WIN32)
        _wputenv_s(L"LOCALAPPDATA", m_folder.wstring().c_str());
#else
        setenv("HOME", m_folder.c_str(), 1);
#endif
    }

    void TearDown() override
    {
        std::error_code ec;
        fs::remove_all(m_folder, ec);
    }

    /// Writes @p json to a file in the test's folder and returns its path.
    std::string write(const std::string& name, const std::string& json) const
    {
        const fs::path path = m_folder / name;
        std::ofstream(path) << json;
        return path.string();
    }

    /// Loads @p json as a montage, failing the test when it can not be read.
    CharmeleonMontage load(const std::string& json) const
    {
        CharmeleonMontage montage;
        std::string error;
        EXPECT_TRUE(loadCharmeleonMontage(write("montage.json", json), montage, error)) << error;
        return montage;
    }

    fs::path m_folder;
};

/// A small amplifier: 4 EEG, 2 bipolar, 1 auxiliary and a trigger channel, as TMSi lists them.
const std::vector<Kind> LAYOUT = {Kind::EEG, Kind::EEG, Kind::EEG, Kind::EEG, Kind::Bipolar, Kind::Bipolar, Kind::Auxiliary, Kind::Other};

}  // namespace

TEST_F(CharmeleonMontageTest, NamesTheEegChannelsByHardwareChannelAndEmptiesTheOthers)
{
    const CharmeleonMontage montage = load(R"({
        "Fz": { "IsActive": true, "LabelText": "Fz", "HardwareChannel": 1 },
        "Cz": { "IsActive": true, "LabelText": "Cz", "HardwareChannel": 3 }
    })");

    const MontageApplication result = applyCharmeleonMontage(montage, LAYOUT, {"EEG1", "EEG2", "EEG3", "EEG4", "", "", "", ""});

    EXPECT_EQ(result.names, (std::vector<std::string>{"Fz", "", "Cz", "", "", "", "", ""}));
    // Every EEG entry is the montage's, named or not; the others stay the user's
    EXPECT_EQ(result.locked, (std::vector<bool>{true, true, true, true, false, false, false, false}));
    EXPECT_FALSE(result.warning);
    EXPECT_NE(result.summary.find("2 EEG electrodes"), std::string::npos) << result.summary;
}

TEST_F(CharmeleonMontageTest, KeepsTheNamesTypedForOtherChannels)
{
    const CharmeleonMontage montage = load(R"({ "Cz": { "IsActive": true, "LabelText": "Cz", "HardwareChannel": 1 } })");

    const MontageApplication result = applyCharmeleonMontage(montage, LAYOUT, {"", "", "", "", "ECG", "", "GSR", "Trigger"});

    EXPECT_EQ(result.names[4], "ECG");
    EXPECT_EQ(result.names[6], "GSR");
    EXPECT_EQ(result.names[7], "Trigger");
}

TEST_F(CharmeleonMontageTest, PlacesOtherChannelsByKindAndNumber)
{
    const CharmeleonMontage montage = load(R"({
        "Cz":    { "IsActive": true, "LabelText": "Cz",   "HardwareChannel": 1 },
        "Right": { "IsActive": true, "LabelText": "HEOG", "HardwareChannel": 0, "Channel": "BIP2" },
        "Top":   { "IsActive": true, "LabelText": "GSR",  "Channel": "aux 1" }
    })");

    const MontageApplication result = applyCharmeleonMontage(montage, LAYOUT, std::vector<std::string>(LAYOUT.size()));

    EXPECT_EQ(result.names[5], "HEOG");
    EXPECT_EQ(result.names[6], "GSR");
    EXPECT_TRUE(result.locked[5]);
    EXPECT_TRUE(result.locked[6]);
    EXPECT_FALSE(result.locked[4]);
    EXPECT_NE(result.summary.find("2 other channels"), std::string::npos) << result.summary;
}

TEST_F(CharmeleonMontageTest, ReportsElectrodesTheAmplifierHasNoChannelFor)
{
    const CharmeleonMontage montage = load(R"({
        "Cz":   { "IsActive": true, "LabelText": "Cz",   "HardwareChannel": 2 },
        "Left": { "IsActive": true, "LabelText": "Left", "HardwareChannel": 73 },
        "EOG":  { "IsActive": true, "LabelText": "EOG",  "Channel": "BIP9" }
    })");

    const MontageApplication result = applyCharmeleonMontage(montage, LAYOUT, std::vector<std::string>(LAYOUT.size()));

    EXPECT_TRUE(result.warning);
    EXPECT_NE(result.summary.find("Left (EEG 73)"), std::string::npos) << result.summary;
    EXPECT_NE(result.summary.find("EOG (BIP9)"), std::string::npos) << result.summary;
}

TEST_F(CharmeleonMontageTest, LeavesOutInactiveElectrodesAndThoseOnNoChannel)
{
    CharmeleonMontage montage;
    std::string error;
    ASSERT_TRUE(loadCharmeleonMontage(write("m.json", R"({
        "Fz": { "IsActive": false, "LabelText": "Fz", "HardwareChannel": 1 },
        "Pz": { "IsActive": true,  "LabelText": "Pz", "HardwareChannel": 0 },
        "Cz": { "IsActive": true,  "LabelText": "Cz", "HardwareChannel": "2" }
    })"),
                                      montage, error));

    ASSERT_EQ(montage.electrodes.size(), 1u);
    EXPECT_EQ(montage.electrodes[0].name, "Cz");
    EXPECT_EQ(montage.electrodes[0].number, 2);
}

TEST_F(CharmeleonMontageTest, KeepsTheFirstOfTwoElectrodesOnOneChannel)
{
    const CharmeleonMontage montage = load(R"({
        "Fp1": { "IsActive": true, "LabelText": "Fp1", "HardwareChannel": 2 },
        "FC1": { "IsActive": true, "LabelText": "FC1", "HardwareChannel": 2 }
    })");

    ASSERT_EQ(montage.electrodes.size(), 1u);
    EXPECT_EQ(montage.electrodes[0].name, "Fp1");
    ASSERT_EQ(montage.problems.size(), 1u);
    EXPECT_NE(montage.problems[0].find("FC1"), std::string::npos);

    const MontageApplication result = applyCharmeleonMontage(montage, LAYOUT, std::vector<std::string>(LAYOUT.size()));
    EXPECT_TRUE(result.warning);
}

TEST_F(CharmeleonMontageTest, ReportsAChannelThatIsNotBipolarOrAuxiliary)
{
    const CharmeleonMontage montage = load(R"({
        "Cz":  { "IsActive": true, "LabelText": "Cz",  "HardwareChannel": 1 },
        "ECG": { "IsActive": true, "LabelText": "ECG", "Channel": "EXG7" }
    })");

    ASSERT_EQ(montage.electrodes.size(), 1u);
    ASSERT_EQ(montage.problems.size(), 1u);
    EXPECT_NE(montage.problems[0].find("EXG7"), std::string::npos);
}

TEST_F(CharmeleonMontageTest, EmptiesOtherChannelsCarryingAnElectrodeName)
{
    // An older montage counted all device channels, and so put electrodes on bipolar channels
    const CharmeleonMontage montage = load(R"({ "Oz": { "IsActive": true, "LabelText": "Oz", "HardwareChannel": 4 } })");

    const MontageApplication result = applyCharmeleonMontage(montage, LAYOUT, {"", "", "", "", "Oz", "ECG", "", ""});

    EXPECT_EQ(result.names[3], "Oz");
    EXPECT_EQ(result.names[4], "");
    EXPECT_EQ(result.names[5], "ECG");
}

TEST_F(CharmeleonMontageTest, WithoutAnAmplifierPlacesTheEegNamesInChannelOrder)
{
    const CharmeleonMontage montage = load(R"({
        "Fz":  { "IsActive": true, "LabelText": "Fz",  "HardwareChannel": 2 },
        "EOG": { "IsActive": true, "LabelText": "EOG", "Channel": "BIP1" }
    })");

    const MontageApplication result = applyCharmeleonMontage(montage, {}, {"a", "b", "c"});

    EXPECT_EQ(result.names, (std::vector<std::string>{"", "Fz", ""}));
    // Only what the montage named is locked: the rest may turn out not to be EEG
    EXPECT_EQ(result.locked, (std::vector<bool>{false, true, false}));
    EXPECT_NE(result.summary.find("No amplifier connected"), std::string::npos) << result.summary;
    EXPECT_NE(result.summary.find("EOG (BIP1)"), std::string::npos) << result.summary;
}

TEST_F(CharmeleonMontageTest, WarnsWhenMadeForAnotherAmplifier)
{
    const CharmeleonMontage montage = load(R"({
        "Cz": { "IsActive": true, "LabelText": "Cz", "HardwareChannel": 1 },
        "Amplifier": { "Type": "TMSi Refa", "EEGChannels": 64, "BipolarChannels": 4, "AuxiliaryChannels": 4 }
    })");
    ASSERT_TRUE(montage.hasAmplifier);
    EXPECT_EQ(montage.amplifier.eegChannels, 64);

    const MontageApplication result = applyCharmeleonMontage(montage, LAYOUT, std::vector<std::string>(LAYOUT.size()));

    EXPECT_TRUE(result.warning);
    EXPECT_NE(result.summary.find("Made for TMSi Refa"), std::string::npos) << result.summary;
}

TEST_F(CharmeleonMontageTest, RefusesFilesThatAreNotMontages)
{
    CharmeleonMontage montage;
    std::string error;
    EXPECT_FALSE(loadCharmeleonMontage((m_folder / "missing.json").string(), montage, error));
    EXPECT_FALSE(error.empty());
    EXPECT_FALSE(loadCharmeleonMontage(write("list.json", "[1, 2]"), montage, error));
    EXPECT_FALSE(loadCharmeleonMontage(write("none.json", R"({ "Cz": { "IsActive": false, "LabelText": "Cz", "HardwareChannel": 1 } })"), montage, error));
}

TEST_F(CharmeleonMontageTest, FindsTheKeptCopyOnceTheFileIsGone)
{
    const std::string path = write("session.json", R"({ "Cz": { "IsActive": true, "LabelText": "Cz", "HardwareChannel": 1 } })");

    MontageApplication result;
    std::string error;
    ASSERT_TRUE(applyCharmeleonMontageSetting(path, LAYOUT, std::vector<std::string>(LAYOUT.size()), result, error)) << error;
    EXPECT_EQ(locateCharmeleonMontage(path).source, MontageLocation::Source::AsGiven);

    fs::remove(path);
    const MontageLocation location = locateCharmeleonMontage(path);
    EXPECT_EQ(location.source, MontageLocation::Source::KeptCopy);

    ASSERT_TRUE(applyCharmeleonMontageSetting(path, LAYOUT, std::vector<std::string>(LAYOUT.size()), result, error)) << error;
    EXPECT_EQ(result.names[0], "Cz");
    EXPECT_TRUE(result.warning);
    EXPECT_NE(result.summary.find("copy"), std::string::npos) << result.summary;
}

TEST_F(CharmeleonMontageTest, SaysWhenAMontageIsNotFound)
{
    MontageApplication result;
    std::string error;
    EXPECT_FALSE(applyCharmeleonMontageSetting((m_folder / "nowhere.json").string(), LAYOUT, {}, result, error));
    EXPECT_NE(error.find("not found"), std::string::npos) << error;
    EXPECT_EQ(locateCharmeleonMontage("").source, MontageLocation::Source::NotFound);
}

TEST_F(CharmeleonMontageTest, KeepsTheSettingWhenCharmeleonSavedToTheSameFile)
{
    const std::string path = write("own.json", R"({ "Cz": { "IsActive": true, "LabelText": "Cz", "HardwareChannel": 1 } })");
    const std::string other = write("other.json", R"({ "Cz": { "IsActive": true, "LabelText": "Cz", "HardwareChannel": 1 } })");

    EXPECT_EQ(montageSettingAfterSave(path, path), path);
    // Charmeleon reports full paths, which may be spelled differently from the setting
    EXPECT_EQ(montageSettingAfterSave(path, fs::absolute(fs::path(path)).string()), path);
    // Save As: the driver follows the new file
    EXPECT_EQ(montageSettingAfterSave(path, other), other);
    EXPECT_EQ(montageSettingAfterSave(path, ""), path);
}
