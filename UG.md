# OpenViBE UG: differences from upstream OpenViBE

This repository (`markspan/openvibe-ug`) is a fork of the Inria OpenViBE 4.0 repository
(`https://gitlab.inria.fr/openvibe/openvibe`). It adapts OpenViBE to the EEG setup of the UG lab:

- TMSi and eemagine eego amplifiers
- Charmeleon for montages and impedance checks
- BrainVision files as the recording format

This document lists every change against upstream, what each change does and what it means in use.

**In the code**, every place where the fork differs from upstream is marked with a comment that starts with `UG fork:`, in C++, QML, CMake and TOML alike. A new file says so at its top. To list them all:

```bash
git grep -n "UG fork"
```

**Montages** have their own how-to: [montages.md](montages.md). It covers why TMSi montages are tailor-made and eego montages semi-fixed, how to make one in Charmeleon, and how the server and the BrainVision writer use it.

## Base and scope

| | |
|---|---|
| Upstream | `origin/main` at `4891073` ("Merge branch 'development' into 'main'", 2026-10-09) |
| Fork | `main`, 19 commits on top of that base (`87afc64` onwards) |
| Upstream commits not in the fork | none at the time of writing |
| Size | 47 files, about 4,900 lines added and 160 removed |

To regenerate the list of changes:

```bash
git diff --stat $(git merge-base HEAD origin/main) HEAD
```

Everything outside the files listed here is identical to upstream.

---

## 1. Summary of functional differences

| Area | Upstream OpenViBE 4.0 | OpenViBE UG |
|---|---|---|
| TMSi amplifiers | No driver in the new Qt acquisition server; only an unported 3.x driver | New built-in TMSi driver (Windows, `TMSiSDK.dll` loaded at runtime) |
| eego amplifiers | Driver source present but not built | Built. Hex bit masks replaced by the channel list below |
| Channel selection (TMSi, eego) | Bit masks (eego) | Channel list read from the amplifier; a channel is recorded when it has a name |
| Montages | none | Charmeleon `.json` montages name and select the EEG channels, and name BIP and AUX channels; one set of rules for TMSi and eego; found by name in Charmeleon's library; a copy is kept |
| Montage feedback | n/a | The server says what the montage did, and shows the entries it decides read-only |
| Impedance check | Built-in check only | Charmeleon is launched for eego and TMSi, on the driver's own amplifier; its montage edits come back to the driver when it closes |
| Driver errors | Printed to the console only | Shown under the driver settings, including why Play failed |
| Drift correction | Settings declared in the UI code, never shown and never applied | Working drift correction as in the 3.x server, off by default, with settings in Preferences |
| Trigger timing | Triggers dated at the block they arrive in | Every sample is timestamped; each trigger is dated at its own sample |
| Hardware triggers | n/a (TMSi) / flank-decoded (eego) | Sent as `OVTK_StimulationId_Label(value)`, value 1–255, not inverted |
| Channel names in scenarios | Not sent; the client labels channels 1, 2, … | Sent to the Acquisition client box and shown in the Signal display and file writers |
| Signal display | Trigger shown by stimulation name or `Id 33025` | Hardware trigger shown as its value (`1`, `12`, …) |
| BrainVision writer | "(Gipsa)" box writes every stimulation's full ID as a marker | Extra "(UG)" box writes only hardware triggers, as `S  1` … `S255`, into a chosen directory, and electrode positions from a montage |
| Kernel tokens | no `Path_Documents`; the Foldername setting type is unregistered | `${Path_Documents}` added; `Foldername` setting type registered |
| Designer | Clicking two outputs could try to link a box to itself and wedge link creation | Second click on a port of the same kind restarts the link; failures end link creation |
| Path parameters | Re-choosing the same file in the file dialog does nothing | Re-choosing the same file applies it again (reloads an edited montage) |
| Client memory | Acquisition client leaked every received message's arrays | Arrays freed |
| Build | Fixed CMake 3.25 / Ninja 1.10 versions, `ninja -C build` | Any CMake ≥ 3.20 and any Ninja, `cmake --build build`; Qt located through `CMAKE_PREFIX_PATH` |

---

## 2. Changes in detail

### 2.1 TMSi driver (new)

**Files**
- `extras/applications/platform/acquisition-server/builtin/drivers/tmsi/CDriverTMSi.h` (new)
- `extras/applications/platform/acquisition-server/builtin/drivers/tmsi/CDriverTMSi.cpp` (new)
- `extras/applications/platform/acquisition-server/CMakeLists.txt` (adds the driver on Windows)

**What it does**

The driver is a port of the 3.x (Mensia) TMSi driver to the 4.0 `AbstractDriver` interface. It supports Refa, Porti, Mobita and similar amplifiers. `TMSiSDK.dll` is loaded with `LoadLibrary` when the driver is used, so no TMSi SDK is needed to build.

Settings, in order:

| Setting | Description |
|---|---|
| Connection protocol | USB by default |
| Device | List of the devices the SDK finds |
| Sampling rate | Default 512 Hz. If the device proposes a different rate, it is logged |
| Channel count | Read from the amplifier; kept in the session only to restore names when the amplifier is not connected |
| Montage | Optional Charmeleon montage: a `.json` path, or the name of one in Charmeleon's library (see 2.3) |
| Trigger channel | Device channel name; empty means the first digital channel that is not the sawtooth |
| Common mode rejection | Common average reference; on by default |

Channel model:
- **Channel list.** When the driver settings are shown, the driver opens the amplifier briefly and reads its signal format. The channel names list then gets exactly one entry per device channel. This is cached per device.
- **EEG (EXG) channels** start with their device name. With a montage, they get the electrode names the montage gives them, and EEG channels it leaves out stay empty and are not recorded (see 2.3).
- **BIP, AUX, DIG, SAW and other channels** start empty. The empty entry shows a greyed hint (`BIP1`, `BIP2`, `AUX1`, … or the device name) so it is clear which channel the entry stands for.
- **Recording.** Only channels with a non-empty name are recorded, in device order. Typing a name for a BIP or AUX channel records it under that name. If no channel has a name, Play fails with an error message.
- **During acquisition** the list shows only the recorded channels. The full list, including empty entries, comes back when acquisition stops.

A montage is applied by the rules both drivers share (2.3). In short, it names every EEG channel, and the BIP and AUX channels it addresses; the other names typed for BIP, AUX or DIG channels are kept.

When the driver cannot start (no device, a device that refuses access, no channel named, a sampling rate the device does not support), it says why under the settings (see 2.16).

Triggers:
- Triggers come from **one** channel, the trigger channel. They are never taken from the SAW (sawtooth) channel, which the SDK may also type as digital.
- The low 8 bits are sent, without inversion, as `OVTK_StimulationId_Label(value)` (0x8101–0x81FF).
- A trigger is sent when the value changes to a non-zero value.
- Each trigger is dated at the timestamp of its own sample (see 2.5).

The driver asks for drift correction when the policy is "Let the driver decide" (see 2.4) and hands impedance checking to Charmeleon (see 2.6).

**Consequences**
- Upstream 4.0 cannot record from a TMSi amplifier at all. The UG fork can, on Windows only.
- Users no longer give a channel count or channel masks. The amplifier decides the list, and naming decides what is recorded.
- The 3.x "Additional channels" setting is gone for TMSi. Old session files that carry it are ignored for that setting.
- A recording's channel order is always device order, whatever order names were typed in.
- On an amplifier with few EEG channels (e.g. 8 EXG plus BIP/AUX), montage electrodes beyond the EEG channels are not recorded. The note under the settings lists them.
- **Not yet verified on hardware:** the channel list, montage mapping, hints and Signal display names introduced in the last commits.

### 2.2 eego driver (built and reworked)

**Files**
- `extras/contrib/plugins/server-drivers/CMakeLists.txt` (adds `eemagine-eego` to the build)
- `extras/contrib/plugins/server-drivers/eemagine-eego/src/CDriverEEGO.h`
- `extras/contrib/plugins/server-drivers/eemagine-eego/src/CDriverEEGO.cpp`

**What it does**
- The eego driver, which upstream ships as source but does not build, is now built as a loadable driver plugin.
- **Channel selection** uses the same model as TMSi (2.1). The `EEGMask` and `BIPMask` hex settings are gone.
  - **Channel list.** When the driver settings are shown, the driver acquires the amplifier briefly and reads its channel list. No stream is needed. The channel count setting is set from it ("read from the amplifier"), and the channel names list gets one entry per referential (EEG1…EEGn) and bipolar (BIP1…BIPm) channel. The list is read once per run of the server; `init()` reads it again from the amplifier actually used.
  - **EEG channels** start with their name (`EEG1`, …). With a *Montage*, they get the montage's electrode names instead, by the rules both drivers share (2.3). HardwareChannel *n* is the *n*-th referential channel. On eego the cap fixes which electrode is on which referential channel, so a montage mostly mirrors the cap (see [montages.md](montages.md)).
  - **Bipolar channels** start empty, with greyed hints `BIP1`, `BIP2`, …. Typing a name records the channel under that name; a montage can name them too (`"Channel": "BIP1"`).
  - **Recording.** Only channels with a name are recorded, in amplifier order. During acquisition the list shows only the recorded channels plus `Trigger` and `SampleCounter`. The full list comes back afterwards.
- Channels are handed to the SDK as an explicit list. This also removes the 64-bit mask limit.
- The trigger and sample-counter channels are always appended, as upstream does.
- Every sample is timestamped, and each trigger is dated at its own sample (see 2.5).
- The driver asks for drift correction under "Let the driver decide" and uses Charmeleon for impedances when it is installed.
- When it cannot start, it says why under the settings (see 2.16).

**Consequences**
- Upstream sessions with `EEGMask` / `BIPMask` values do not carry over. Name the channels, or set a montage, instead.
- Showing the driver settings acquires the amplifier once. If another program holds it at that moment (e.g. a running Charmeleon), the list cannot be read. The log says so, and the driver tries again the next time the settings are read.
- **Not yet verified on hardware.**

### 2.3 Charmeleon montages (new)

**Files**
- `extras/applications/platform/acquisition-server/abstractions/driver/CharmeleonMontage.h` (new)
- `extras/applications/platform/acquisition-server/abstractions/driver/CharmeleonMontage.cpp` (new)
- `extras/applications/platform/acquisition-server/abstractions/CMakeLists.txt` (builds and installs it; links `Advapi32` for the registry)
- `extras/applications/platform/acquisition-server/test/CharmeleonMontageTest.cpp` (new)
- `CDriverTMSi.*` and `CDriverEEGO.*` (they apply montages through it)
- [`montages.md`](montages.md) (new): the how-to

**What it does**

One set of rules, shared by the TMSi and eego drivers, so that a montage means the same to both:

- **Reading** (`loadCharmeleonMontage()`). A montage is a JSON object of electrode entries with `IsActive`, `LabelText`, `HardwareChannel` and, since Charmeleon 2.2, optionally:
  - `Channel`: a BIP or AUX channel (`"BIP1"`, `"AUX2"`), which takes precedence over `HardwareChannel`;
  - `Theta` and `Phi`: an exact position;
  - an `Amplifier` entry describing the amplifier the montage was saved with.

  The reader skips inactive electrodes and electrodes on no channel. Of two electrodes on one channel it keeps the first, and it reports a name used for two channels.
- **Numbering.** HardwareChannel *n* is the *n*-th EEG channel: TMSi's EXG, eego's referential. `BIPn` and `AUXn` count the bipolar and auxiliary channels in the amplifier's own order, the same labels the server shows as hints.
- **Applying** (`applyCharmeleonMontage()`).
  - Every EEG channel gets the name of the electrode on it, or none: the montage decides which EEG channels are recorded.
  - A BIP or AUX channel the montage names gets that name. The others keep the names typed for them, unless a name is one of the montage's electrodes, left over from an older montage.
  - The result also says which entries the montage decided (the *locks*, shown read-only, see 2.8) and gives a summary for the note under the settings (2.16): electrodes the amplifier has no channel for, a montage made for another amplifier, problems in the file.
- **Without an amplifier connected,** the driver does not know which channels are EEG. Every entry then counts as EEG, in channel order, and only the entries the montage named are locked. Play applies the montage again, to the amplifier's real channels.
- **Finding the file** (`locateCharmeleonMontage()`), in this order:
  1. the setting as a path;
  2. by its file name, in Charmeleon's installed `montages` folder, then its `Resources` (default layouts), so a setting such as `ca-208.json` works on every PC with Charmeleon;
  3. the copy kept from its last successful load, in `%LOCALAPPDATA%\OpenViBE\acquisition-server\montages`.

  A note says when the kept copy is used.

**Consequences**
- The montage made for Charmeleon's impedance view is the same file that names and selects the recorded channels. There is one source of truth for electrode names.
- EOG, ECG and other channels on BIP or AUX inputs are named in the montage, so they need no typing in the server.
- A montage with two electrodes on one channel records only the first. The note says so, and Charmeleon 2.2 refuses to save such a montage.
- Montages from before Charmeleon 2.2 that put electrodes on numbers above the EEG range (the old Left/Right/Top/Bottom markers on e.g. 72–75) no longer reach BIP or AUX channels. The note lists them as not recorded; set their channel to `BIPn`/`AUXn` in Charmeleon.
- A montage file that is moved or deleted no longer stops a recording.
- The rules have unit tests (13 in `CharmeleonMontageTest.cpp`).

### 2.4 Drift correction (new)

**Files**
- `extras/applications/platform/acquisition-server/core/DriftCorrector.h` (new)
- `extras/applications/platform/acquisition-server/core/DriftCorrector.cpp` (new)
- `extras/applications/platform/acquisition-server/core/Acquisition.h`
- `extras/applications/platform/acquisition-server/core/Acquisition.cpp`
- `extras/applications/platform/acquisition-server/abstractions/driver/AbstractDriver.h` (`wantsDriftCorrection()`)
- `extras/applications/platform/acquisition-server/settings/SessionManager.h`
- `extras/applications/platform/acquisition-server/settings/SessionManager.cpp`
- `extras/applications/platform/acquisition-server/ui/AcquisitionController.h`
- `extras/applications/platform/acquisition-server/ui/AcquisitionController.cpp`
- `extras/applications/platform/acquisition-server/ui/qml/Main.qml`
- `extras/applications/platform/acquisition-server/CMakeLists.txt`
- `extras/applications/platform/acquisition-server/test/CMakeLists.txt`
- `extras/applications/platform/acquisition-server/test/DriftCorrectorTest.cpp` (new)

**What it does**

Upstream 4.0 measures and displays drift but never corrects it. Its UI code declares "Drift Correction Policy" and "Drift Tolerance" parameters that are neither shown nor used. The UG fork restores the 3.x behaviour:

- For every buffer, `DriftCorrector` compares the number of samples received with the number the elapsed time calls for. It averages this over the last 128 buffers.
- When the average exceeds the tolerance, it corrects at the end of the current buffer:
  - **Too few samples:** it repeats the last sample. Repeated samples get the same timestamp as the original.
  - **Too many samples:** it drops the last samples. Stimulations on dropped samples move to the new last sample, so none are lost.
- Two settings under Preferences control it, and both are saved in the session as `driftCorrectionPolicy` and `driftToleranceMs`:
  - *Drift correction*: "Disabled" (default), "Let the driver decide" or "Forced". "Let the driver decide" corrects drivers whose `wantsDriftCorrection()` returns true, which are TMSi and eego.
  - *Drift tolerance (ms)*: default 5, range 1–100.
- At Stop, the log reports how many samples were added and removed, and in how many corrections.

**Consequences**
- Out of the box nothing is corrected, and the recorded samples are exactly what the amplifier sent.
- With "Let the driver decide" or "Forced", long TMSi and eego recordings keep the number of samples in step with wall-clock time, as in 3.x. Upstream lets them drift.
- With correction on, the recorded signal can contain repeated or missing samples at correction points. This is the same trade-off 3.x made.
- The default policy matches what upstream had declared (Disabled), and the default tolerance differs (5 ms instead of 2 ms).
- Sessions saved while the default was "Let the driver decide" stored the policy under `driftCorrection`. On their first load "Let the driver decide" there is taken as the old default and becomes "Disabled", while "Forced" and "Disabled" are kept. From then on the policy is stored under `driftCorrectionPolicy`, so choosing "Let the driver decide" again sticks.

### 2.5 Exact trigger placement

**Files**
- `CDriverTMSi.cpp` and `CDriverEEGO.cpp` (they now implement `getData()` with per-sample timestamps)
- `extras/plugins/processing/acquisition/src/box-algorithms/BoxAcquisitionClient.cpp`

**What it does**
- Both drivers give every sample its own timestamp and date each trigger with its sample's timestamp.
- The Acquisition client box places each stimulation on the last sample stamped at or before the stimulation's date. Among samples that share a stamp (samples repeated by drift correction), it picks the first one, the original.

**Consequences**
- A trigger lands on the sample where the amplifier saw it, not at the start or end of the block it arrived in. Without this, its position could be off by up to one block.
- Drift correction cannot shift a trigger onto a repeated sample.

### 2.6 Impedance check through Charmeleon (new)

**Files**
- `extras/applications/platform/acquisition-server/ui/Charmeleon.h` (new)
- `extras/applications/platform/acquisition-server/ui/Charmeleon.cpp` (new)
- `extras/applications/platform/acquisition-server/ui/AcquisitionController.h`
- `extras/applications/platform/acquisition-server/ui/AcquisitionController.cpp`
- `extras/applications/platform/acquisition-server/core/Acquisition.h`
- `extras/applications/platform/acquisition-server/core/Acquisition.cpp`
- `extras/applications/platform/acquisition-server/abstractions/driver/AbstractDriver.h` (`usesCharmeleonForImpedance()`, `charmeleonArguments()`, `charmeleonMontage()`, `takeOverCharmeleonMontage()`)
- `extras/applications/platform/acquisition-server/CMakeLists.txt`
- `CDriverTMSi.*` and `CDriverEEGO.*`

**What it does**
- Drivers opt in with `usesCharmeleonForImpedance()`; eego and TMSi do. The impedance button is then enabled even when the driver has no built-in check (TMSi).
- When Charmeleon is installed, the impedance button starts it instead of the built-in check, or brings it to the front if it is already running. Charmeleon is found in this order:
  1. the `AcquisitionServer_CharmeleonPath` configuration token;
  2. the installer's uninstall registry key (`Charmeleon_is1`);
  3. `%ProgramFiles%\Charmeleon`.
- Charmeleon is started with:
  - **the driver's montage**, as the full path of the file the driver reads;
  - **the driver's amplifier**: `--amplifier tmsi --protocol <protocol> [--device <device>]`, or `--amplifier eego`. Charmeleon then opens only that kind of amplifier (and that device), never another one on the same PC;
  - `--save-on-close --result <file>`. When Charmeleon closes, it saves the montage without asking and writes which file it ended with.
- **When that Charmeleon closes,** the driver takes its montage over: it reads the file again, and switches to the new file if it was saved under another name.
- **Play closes every running Charmeleon first.** It sends each one `WM_CLOSE`, so that the amplifier is released cleanly and Charmeleon 2.2 saves its montage, and terminates any instance still running after 3 s. The montage is then taken over before the driver starts.

**Consequences**
- Impedances are checked in Charmeleon's own window, with its montage view, instead of the server's list.
- Montage edits made in Charmeleon reach the recording without choosing the file again.
- This needs **Charmeleon 2.2** or later. An older Charmeleon ignores the extra arguments: it opens the first amplifier it finds, eego before TMSi, and does not report back. The server's log then says the driver keeps its montage, and unsaved edits in it are lost on Play, as before.
- Without Charmeleon installed, eego falls back to its built-in impedance check. TMSi then has no impedance check.

### 2.7 Channel names sent to the client

**Files**
- `sdk/openvibe/include/openvibe/acquisition-messaging/ovMsg.h`
- `sdk/openvibe/include/openvibe/acquisition-messaging/ovMsgSerializer.h`
- `extras/applications/platform/acquisition-server/abstractions/writer/AbstractWriter.h`
- `extras/applications/platform/acquisition-server/abstractions/writer/AbstractWriter.cpp`
- `extras/applications/platform/acquisition-server/core/Acquisition.cpp`
- `extras/plugins/processing/acquisition/src/box-algorithms/BoxAcquisitionClient.cpp`
- `extras/applications/platform/acquisition-server/test/OvMsgSerializerTest.cpp` (new)

**What it does**
- The `OV_MSG_ACQUISITION_CONFIG` message now carries the channel names, each ended by `\0`, after the three counts. The wire format is `[type][size][channelCount][sampleRate][sampleChunk][names…]`.
- The serializer writes and reads this format field by field, instead of copying the in-memory struct.
- `AbstractWriter::writeAcquisitionConfig()` takes the names as a new argument. This changes the API.
- The Acquisition client box labels its signal channels with the received names. A channel without a name, or a server that sends no names, gets numbered from 1.
- The client now frees the arrays `deserializeOvMsg()` allocates for every message. Upstream leaked them for the whole recording.

**Consequences**
- Channel names typed in the acquisition server appear in the Signal display, in the channel selectors and in every file writer (BrainVision, GDF, CSV, …). Upstream shows 1, 2, 3, ….
- **Compatibility:** a UG client reading an upstream server's message (20 bytes, no names) falls back to numbers; this case is covered by a unit test. The upstream client reads the three counts the same way, so it should take a UG server's message, but this has not been tested.
- Writer plugins built against upstream's `AbstractWriter` need recompiling because the signature changed.
- Memory use of the Designer and Scenario Player no longer grows during long recordings.

### 2.8 Channel name hints and editing

**Files**
- `extras/applications/platform/acquisition-server/abstractions/driver/AbstractDriver.h` (`channelHints()`, `channelLocks()`)
- `extras/applications/platform/acquisition-server/core/Acquisition.h` (`getDriverChannelHints()`)
- `extras/applications/platform/acquisition-server/ui/AcquisitionController.h`
- `extras/applications/platform/acquisition-server/ui/AcquisitionController.cpp` (`channelHints` property)
- `extras/applications/platform/acquisition-server/ui/qml/Main.qml`
- `extras/applications/platform/acquisition-server/ui/qml/ChannelRename.qml`

**What it does**

Drivers can supply one hint per channel name entry. The channel names panel shows it as greyed placeholder text while the entry is empty. TMSi supplies `BIP1`, `AUX1`, … and device names, and eego supplies `BIP1`, `BIP2`, …; other drivers supply none.

Drivers can also lock entries (`AbstractDriver::channelLocks()`): the entries their montage decides (2.3). A locked entry is greyed and read-only, with a tooltip saying why. Enter and Tab skip it, and loading names from a file leaves it alone.

Editing a name updates that entry in place, so the list keeps its scroll position. Upstream rebuilt the list after every edit, which sent it back to the top. Enter and Tab move on to the next entry and Shift+Tab to the previous one, scrolling it into view with its text selected.

**Consequences**
- The empty entries are self-explanatory.
- A hint is never recorded as a name: only typed text records a channel.
- A series of names (e.g. the additional channels) can be typed in one go without reaching for the mouse.
- Leaving an entry without changing it no longer saves the session.

### 2.9 BrainVision Format file writer (UG) (new box)

**Files**
- `extras/contrib/plugins/processing/file-io/src/box-algorithms/ovpCBoxAlgorithmBrainampFileWriterUG.h` (new)
- `extras/contrib/plugins/processing/file-io/src/box-algorithms/ovpCBoxAlgorithmBrainampFileWriterUG.cpp` (new)
- `extras/contrib/plugins/processing/file-io/src/ovp_defines.h` (new class IDs)
- `extras/contrib/plugins/processing/file-io/src/ovp_main.cpp` (registers the box)

**What it does**

This is a new box next to upstream's "BrainVision Format file writer (Gipsa)", which is unchanged. It sits in *File reading and writing/BrainVision Format* and is at version 1.3.

Settings:

| Setting | Default | Notes |
|---|---|---|
| Output directory | `${Path_Documents}/data` | Created if it does not exist |
| Header filename | `record-[$core{date}-$core{time}].vhdr` | A filename with a full path ignores the directory |
| Binary format | `IEEE_FLOAT_32` | Gipsa's default is `INT_16` |
| Montage (electrode positions) | empty | A Charmeleon montage. Boxes saved before version 1.3 lack it and record no positions |

Behaviour:
- `.eeg` and `.vmrk` files are written next to the `.vhdr`.
- The full path is logged when recording starts.
- **Only hardware triggers** (`OVTK_StimulationId_Label(1..255)`) are written to the `.vmrk`. They are named as BrainVision Recorder names them: `Mk2=Stimulus,S  1,…`, `S 12`, `S233`.
- All other stimulations (OpenViBE's own, e.g. experiment start or end) are left out.
- **Electrode positions**, with a montage set. The header gets a `[Coordinates]` section, `Ch<n>=1,<theta>,<phi>` in BrainVision's convention (theta negative on the left), and the montage's name under `[Comment]`.
  - Channels are matched to electrodes by name. A channel the montage does not place on the head (EOG, ECG) gets `0,0,0`.
  - Positions come from the montage's `Theta`/`Phi` when it has them (Charmeleon writes these for the equidistant layout). Otherwise they come from the head map: theta = 22.5° × ring, phi = angle.

**Consequences**
- Files load in BrainVision Analyzer, EEGLAB, MNE and FieldTrip with marker names as the original Recorder produced them, `S  1` … `S255`.
- The Gipsa writer writes the full stimulation ID instead, e.g. `S33025` for trigger 1. Its files are not compatible with analysis scripts that expect Recorder marker names.
- By default, recordings go to `Documents\data` instead of the Designer's working directory, so they no longer end up inside the program folder.
- Software stimulations are not stored in the UG writer's files. Record them with another writer if they are needed.
- With a montage, EEGLAB, MNE and BrainVision Analyzer load the electrode layout with the data.

### 2.10 Kernel: `Path_Documents` token and Foldername setting type

**File:** `sdk/kernel/src/kernel/ovkCKernelContext.cpp`

**What it does**
- Adds the configuration token `${Path_Documents}`. On Windows it is the user's Documents folder as the shell reports it (`SHGetKnownFolderPath`), which follows a OneDrive or relocated Documents folder. Elsewhere it is `$HOME/Documents`.
- Registers `OV_TypeId_Foldername` ("Foldername") in the type manager. Upstream does not register it, so a box with a folder setting failed to load with "Foldername setting type cannot be recognized".

**Consequences**
- Any box or configuration can now use `${Path_Documents}`.
- Folder settings work in all boxes, not only in the UG writer.

### 2.11 Signal display shows trigger values

**File:** `extras/plugins/processing/simple-visualization/src/box-algorithms/CSignalDisplay.cpp`

**What it does:** stimulations in the label range (`OVTK_StimulationId_Label(1..255)`) are shown with their trigger value. Other stimulations keep their name or `Id …`.

**Consequence:** a hardware trigger shows as `1`, `12`, `233`. Upstream shows `OVTK_StimulationId_Label_01`, or a raw ID.

### 2.12 Designer link creation fix

**File:** `designer/applications/platform/designer/src/DesignerScenario.cpp`

**What it does**
- After clicking a first port, a second click on a port of the same kind (another output, or the same port again) starts a new link from that port.
- Every failed link attempt now ends link creation.

**Consequence:** in upstream, clicking an output and then another output tried to link a box to itself and failed with "Input index out of range". Link creation then stayed pending, so every later click failed the same way. That no longer happens.

### 2.13 Path parameter: re-choosing a file reloads it

**Files**
- `extras/applications/qml/OVParameters/PathParameter.qml`
- `extras/applications/qml/OVParameters/src/parameterPathObject.h`
- `extras/applications/qml/OVParameters/src/parameterPathObject.cpp`

**What it does:** `ParameterPathObject` gains `choose()`, which applies a value even when it equals the current one. The file dialog now uses it.

**Consequence:** choosing the same montage file again after editing it in Charmeleon reloads it. Upstream ignored the choice because the path had not changed. This affects every path parameter in the Qt applications, not only montages.

### 2.14 Session settings

**Files**
- `extras/applications/platform/acquisition-server/settings/SessionManager.h`
- `extras/applications/platform/acquisition-server/settings/SessionManager.cpp`

**What it does:** the stored session gains the keys `driftCorrectionPolicy` and `driftToleranceMs`. The driver parameters stored for TMSi and eego follow their new settings.

**Consequences**
- A UG session file opened by upstream ignores the extra keys.
- An upstream session opened by the UG fork falls back to the defaults for missing values. This includes eego's masks, which no longer exist.

### 2.15 Build (pixi)

**Files**
- `pixi.toml`
- `pixi.lock`

**What it does**
- CMake `3.25.*` becomes `>=3.20`, and Ninja `1.10.0.*` becomes any version.
- `build` runs `cmake --build build` instead of `ninja -C build`.
- Qt is found through the `CMAKE_PREFIX_PATH` user environment variable instead of a fixed path. Set it once per machine:

  ```bash
  setx CMAKE_PREFIX_PATH D:\Qt\6.9.3\msvc2022_64
  ```

**Consequences**
- The build works on machines with Qt installed elsewhere and with newer CMake or Ninja.
- The lock file differs from upstream. A fresh `pixi install` may resolve newer tool versions than upstream's CI uses.

### 2.16 Driver messages in the server (new)

**Files**
- `extras/applications/platform/acquisition-server/abstractions/driver/AbstractDriver.h` and `.cpp` (`note()`, `setNote()`, `setStateListener()`)
- `extras/applications/platform/acquisition-server/core/Acquisition.h`
- `extras/applications/platform/acquisition-server/ui/AcquisitionController.h` and `.cpp` (`driverNote`, `driverNoteLevel`, `channelLocks`, `driverStateChanged`)
- `extras/applications/platform/acquisition-server/ui/qml/DriverNote.qml` (new), `Main.qml`
- `CDriverTMSi.*` and `CDriverEEGO.*`

**What it does**
- A driver can set a **note**: a text at a level (info, warning, error). The server shows it in a box under the driver settings, with a blue, orange or red edge. It is hidden while there is nothing to say.
- The TMSi and eego drivers use it for:
  - what their montage did (2.3);
  - **why Play failed**: no device connected, a device that refuses access (e.g. held by Charmeleon), no channel named, an unsupported sampling rate, the TMSi library missing.
- A driver may change its note, or its locks, on any thread. The server picks the change up through a listener, queued onto the UI thread.

**Consequences**
- A failed Play is no longer silent: upstream drivers can only print to the console, which the server does not show.
- Drivers that set no note look as in upstream.

---

## 3. All modified files

`A` = added, `M` = modified. The section column refers to the sections above.

| Status | File | Section |
|---|---|---|
| M | `designer/applications/platform/designer/src/DesignerScenario.cpp` | 2.12 |
| M | `extras/applications/platform/acquisition-server/CMakeLists.txt` | 2.1, 2.4, 2.6 |
| M | `extras/applications/platform/acquisition-server/abstractions/CMakeLists.txt` | 2.3 |
| M | `extras/applications/platform/acquisition-server/abstractions/driver/AbstractDriver.cpp` | 2.16 |
| M | `extras/applications/platform/acquisition-server/abstractions/driver/AbstractDriver.h` | 2.4, 2.6, 2.8, 2.16 |
| A | `extras/applications/platform/acquisition-server/abstractions/driver/CharmeleonMontage.cpp` | 2.3 |
| A | `extras/applications/platform/acquisition-server/abstractions/driver/CharmeleonMontage.h` | 2.3 |
| M | `extras/applications/platform/acquisition-server/abstractions/writer/AbstractWriter.cpp` | 2.7 |
| M | `extras/applications/platform/acquisition-server/abstractions/writer/AbstractWriter.h` | 2.7 |
| A | `extras/applications/platform/acquisition-server/builtin/drivers/tmsi/CDriverTMSi.cpp` | 2.1, 2.3, 2.5, 2.6, 2.16 |
| A | `extras/applications/platform/acquisition-server/builtin/drivers/tmsi/CDriverTMSi.h` | 2.1, 2.3, 2.6 |
| M | `extras/applications/platform/acquisition-server/core/Acquisition.cpp` | 2.4, 2.6, 2.7 |
| M | `extras/applications/platform/acquisition-server/core/Acquisition.h` | 2.4, 2.6, 2.8, 2.16 |
| A | `extras/applications/platform/acquisition-server/core/DriftCorrector.cpp` | 2.4 |
| A | `extras/applications/platform/acquisition-server/core/DriftCorrector.h` | 2.4 |
| M | `extras/applications/platform/acquisition-server/settings/SessionManager.cpp` | 2.4, 2.14 |
| M | `extras/applications/platform/acquisition-server/settings/SessionManager.h` | 2.4, 2.14 |
| A | `extras/applications/platform/acquisition-server/test/CharmeleonMontageTest.cpp` | 2.3 |
| M | `extras/applications/platform/acquisition-server/test/CMakeLists.txt` | 2.4 |
| A | `extras/applications/platform/acquisition-server/test/DriftCorrectorTest.cpp` | 2.4 |
| A | `extras/applications/platform/acquisition-server/test/OvMsgSerializerTest.cpp` | 2.7 |
| M | `extras/applications/platform/acquisition-server/ui/AcquisitionController.cpp` | 2.4, 2.6, 2.8, 2.16 |
| M | `extras/applications/platform/acquisition-server/ui/AcquisitionController.h` | 2.4, 2.6, 2.8, 2.16 |
| A | `extras/applications/platform/acquisition-server/ui/Charmeleon.cpp` | 2.6 |
| A | `extras/applications/platform/acquisition-server/ui/Charmeleon.h` | 2.6 |
| M | `extras/applications/platform/acquisition-server/ui/qml/ChannelRename.qml` | 2.8 |
| A | `extras/applications/platform/acquisition-server/ui/qml/DriverNote.qml` | 2.16 |
| M | `extras/applications/platform/acquisition-server/ui/qml/Main.qml` | 2.4, 2.8, 2.16 |
| M | `extras/applications/qml/OVParameters/PathParameter.qml` | 2.13 |
| M | `extras/applications/qml/OVParameters/src/parameterPathObject.cpp` | 2.13 |
| M | `extras/applications/qml/OVParameters/src/parameterPathObject.h` | 2.13 |
| A | `extras/contrib/plugins/processing/file-io/src/box-algorithms/ovpCBoxAlgorithmBrainampFileWriterUG.cpp` | 2.9 |
| A | `extras/contrib/plugins/processing/file-io/src/box-algorithms/ovpCBoxAlgorithmBrainampFileWriterUG.h` | 2.9 |
| M | `extras/contrib/plugins/processing/file-io/src/ovp_defines.h` | 2.9 |
| M | `extras/contrib/plugins/processing/file-io/src/ovp_main.cpp` | 2.9 |
| M | `extras/contrib/plugins/server-drivers/CMakeLists.txt` | 2.2 |
| M | `extras/contrib/plugins/server-drivers/eemagine-eego/src/CDriverEEGO.cpp` | 2.2, 2.3, 2.5, 2.6, 2.16 |
| M | `extras/contrib/plugins/server-drivers/eemagine-eego/src/CDriverEEGO.h` | 2.2, 2.3, 2.6 |
| M | `extras/plugins/processing/acquisition/src/box-algorithms/BoxAcquisitionClient.cpp` | 2.5, 2.7 |
| M | `extras/plugins/processing/simple-visualization/src/box-algorithms/CSignalDisplay.cpp` | 2.11 |
| M | `pixi.lock` | 2.15 |
| M | `pixi.toml` | 2.15 |
| M | `sdk/kernel/src/kernel/ovkCKernelContext.cpp` | 2.10 |
| M | `sdk/openvibe/include/openvibe/acquisition-messaging/ovMsg.h` | 2.7 |
| M | `sdk/openvibe/include/openvibe/acquisition-messaging/ovMsgSerializer.h` | 2.7 |
| A | `UG.md` | this document |
| A | `montages.md` | 2.3: the montage how-to |

---

## 4. Overall consequences and caveats

- **Platform.** The TMSi driver and the Charmeleon integration are Windows-only. On Linux and macOS the fork builds and behaves like upstream, apart from the eego build, the writer, the drift correction and the fixes.
- **Mixing versions.** Use the UG acquisition server with the UG Designer or Scenario Player. Mixing with upstream should work (see 2.7) but loses the channel names and the trigger placement fix, and has not been tested.
- **Scenarios.** Scenarios using the Gipsa writer are unaffected. To get Recorder-style markers and the output directory setting, replace that box with the UG writer.
- **Recorded data.** With drift correction on (off by default; "Let the driver decide" turns it on for TMSi and eego), recordings may contain repeated or dropped samples at correction points. The log reports how many.
- **Merging upstream.** The fork changes shared code that upstream may also change, so these are the likely merge conflicts:
  - acquisition message format and serializer
  - `AbstractWriter` signature
  - `Acquisition`, `AcquisitionController`, `SessionManager`
  - `DesignerScenario::clickPort`
  - the kernel context
  - `pixi.toml` and `pixi.lock`

  New files (TMSi driver, drift corrector, Charmeleon, UG writer) merge cleanly.
- **Charmeleon.** The montage round trip and the choice of amplifier need Charmeleon 2.2 (see 2.6). Charmeleon is a separate repository and installer.
- **Tests.** The acquisition server unit tests (`test-acquisition-server-core`, 40 tests) pass. They include new tests for the drift corrector, the config message and the montage rules.
- **Tested without an amplifier:**
  - the montage note and locked entries;
  - the error under the settings when Play fails;
  - the round trip with Charmeleon 2.2 in demo mode: the server starts it, it saves on closing, and the driver takes the montage over;
  - the fallback for an older Charmeleon.
- **Not yet verified on hardware:**
  - the TMSi and eego channel lists, montage mapping, channel hints, and channel names in the Signal display;
  - Charmeleon opening the driver's device;
  - TMSi impedances on BIP and AUX channels.
- **Not yet run in Designer:** the electrode positions the UG writer writes.
