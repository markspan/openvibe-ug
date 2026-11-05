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

**Logs.** Neither application opens a console window; what they print is in `%APPDATA%\OpenVIBE-4.0\log\` (see 2.17).

**The P300 speller** has its own how-to: [speller.md](speller.md). It covers the task, setting it up and running it, the port of its display to Qt, and the setup with Python scripts.

**Montages** have their own how-to: [montages.md](montages.md). It covers why TMSi montages are tailor-made and eego montages semi-fixed, how to make one in Charmeleon, and how the server and the BrainVision writer use it.

## Base and scope

| | |
|---|---|
| Upstream | `origin/main` at `4891073` ("Merge branch 'development' into 'main'", 2026-10-09) |
| Fork | `main`, 28 commits on top of that base (`87afc64` onwards) |
| Upstream commits not in the fork | none at the time of writing |
| Size | 172 files, about 24,100 lines added and 1,810 removed |

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
| Lost samples | Drift shown; nothing shows how close the acquisition is to losing samples | Amplifier buffer level, free server buffers and lost samples shown during an acquisition (TMSi, eego) |
| Server threads | Three threads poll in a loop, keeping CPU cores busy, also at idle | They wait for work; the amplifier is read at multimedia priority |
| Drift correction | Settings declared in the UI code, never shown and never applied | None; the settings are gone |
| Console windows | The server opens one; Designer uses that of whatever started it | None; output goes to a log file |
| Trigger timing | Triggers dated at the block they arrive in | Every sample is timestamped; each trigger is dated at its own sample |
| TCP Tagging timing | Markers dated when the server reads them, and sent with the next chunk | Dated at the time in the tag, and sent with the chunk that holds their sample |
| Hardware triggers | n/a (TMSi) / flank-decoded (eego) | Sent as `OVTK_StimulationId_Label(value)`, value 1–255, not inverted |
| Channel names in scenarios | Not sent; the client labels channels 1, 2, … | Sent to the Acquisition client box and shown in the Signal display and file writers |
| Signal display | Trigger shown by stimulation name or `Id 33025`, as small rotated text | Hardware trigger shown as its value (`1`, `12`, …), in a coloured flag on the marker |
| BrainVision writer | "(Gipsa)" box writes every stimulation's full ID as a marker | Extra "(UG)" box writes only hardware triggers, as `S  1` … `S255`, into a chosen directory, and the server's montage with its electrode positions |
| P300 speller | Display box not ported to Qt and not built, so 6 of the 11 speller scenarios can not run; the Lua online scripts also stop with an error | Ported, and the error repaired; markers are sent once the flash is on screen; also a set of scenarios with Python scripts |
| Player timing | Designer's player runs in rounds with a 50 ms sleep between them, so stimuli can be up to about 60 ms off | 1 ms sleep |
| Kernel tokens | no `Path_Documents`; the Foldername setting type is unregistered | `${Path_Documents}` added; `Foldername` setting type registered |
| Designer | Clicking two outputs could try to link a box to itself and wedge link creation | Second click on a port of the same kind restarts the link; failures end link creation |
| Path parameters | Re-choosing the same file in the file dialog does nothing | Re-choosing the same file applies it again (reloads an edited montage) |
| Client memory | Acquisition client leaked every received message's arrays | Arrays freed |
| Client box in scenarios | 31 scenarios, among them all of the speller, use the legacy box, which gets nothing from this server | They use the current `Acquisition client`, on port 8088 |
| Quiet connections | A client silent for 5 s is no longer read; a server silent for 5 s loses its client | They keep the connection |
| Late TCP markers | A marker that reaches the server after its chunk has gone lands on the next chunk's first sample | It lands on its own sample, as the Acquisition client sends markers at least 100 ms behind the signal, which itself is not delayed |
| LSL Output | Samples and markers get the moment of sending as their time, so a marker can be a chunk off; a stream ID that changes at each start | They keep the server's times; markers within 0.26 ms of their dates, and a client recovers after a restart |
| File settings on Windows | A file picked with the dialog gets backslashes, which a box loses, so it can't open the file | Forward slashes, so the box opens it |
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

The driver reports how full the TMSi library's buffer is and how many samples it lost (see 2.4), and hands impedance checking to Charmeleon (see 2.6).

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
- The driver reports how much data waited at each read, and counts lost samples from the sample counter (see 2.4). It uses Charmeleon for impedances when it is installed.
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
- **Without an amplifier connected,** the driver does not know which channels are EEG, so the montage is placed provisionally and only the entries it names are locked. Play applies it again, to the amplifier's real channels.
  - **TMSi** assumes the layout a Refa has: the list's channels are EEG, except the last eight, which are BIP1–4 and AUX1–4. These show their hints, a montage's BIP and AUX channels land on them, and names typed for them are kept.
  - **eego** counts every entry as EEG, in channel order.
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
- The rules have unit tests (16 in `CharmeleonMontageTest.cpp`).

### 2.4 Acquisition threads, buffers and lost samples (reworked)

**Files**
- `extras/applications/platform/acquisition-server/core/Acquisition.h` and `.cpp`
- `extras/applications/platform/acquisition-server/core/CircularBuffer.h` and `.cpp`
- `extras/applications/platform/acquisition-server/core/net-api/CNetAcqControlAPI.h` and `.cpp`
- `sdk/modules/network/Server/CServer.cpp`
- `extras/applications/platform/acquisition-server/abstractions/driver/AbstractDriver.h` (`DeviceBuffer`, `takeDeviceBuffer()`)
- `CDriverTMSi.*` and `CDriverEEGO.*`
- `extras/applications/platform/acquisition-server/ui/AcquisitionController.h` and `.cpp` (`buffers`)
- `extras/applications/platform/acquisition-server/ui/qml/BufferControl.qml` and `LevelBar.qml` (new), replacing `DriftControl.qml` and `DriftBar.qml` (removed); `Main.qml`
- `extras/applications/platform/acquisition-server/CMakeLists.txt`
- `extras/applications/platform/acquisition-server/test/CircularBufferTest.cpp` (new)

**What it does**

Samples are lost only when the amplifier's own buffer overflows. The server reads the amplifier on one thread (the reader) into a pool of 10 buffers. A second thread hands them to the clients (Designer), and a third cleans them for reuse. When the clients are slow, the pool runs out, the reader stops reading, and the amplifier's buffer fills. This section makes that chain cheaper and visible:

- **Threads that wait.** Upstream's processing and cleaning threads poll the buffer queues without pause. So does the network control thread (`CNetAcqControlAPI::run()`), even with nothing connected. The network server's receive loop (`CServer::loopDataFromClient()`) spins too, as soon as a client (Designer) has sent anything. Each kept a CPU core busy. All four now wait until there is work.
- **Reader priority.** The reader thread registers with the Windows multimedia scheduler (MMCSS) as a "Pro Audio" task at high priority, and opts out of Windows 11's power throttling, which may slow down minimised programs. It waits whenever there is nothing to read, so it takes no more CPU than before; it gets it sooner.
- **Empty reads.** A driver that returns at once with nothing (the generic oscillator, LSL) would keep that high-priority reader spinning, so the reader pauses 1 ms after a quick, empty read. Drivers that wait for data themselves (TMSi, eego) take longer than that and are not slowed down. An empty buffer is kept for the next read instead of going round the cleaning thread.
- **Buffers panel.** Where upstream shows the drift, the server shows, twice a second, during an acquisition:
  - **Amplifier buffer.** TMSi: how full the TMSi library's buffer is (`GetBufferInfo`), as a percentage of its 10 seconds. eego: the SDK does not say how full its buffer is, so the panel shows how much data waited at each read. For both, a reader that is held up counts as a growing backlog.
  - **Server buffers:** how many of the 10 are free. The reader and the processing thread hold one each, so 8 free is normal.
  - **Lost samples.** TMSi: the library's overflow count, times its buffer size. eego: gaps in the amplifier's sample counter channel, counted exactly. Lost samples also show in red in the panel's header and are logged as an error.

  Each value is the worst since the previous update; the one in brackets is the worst since Play.
- **Drift correction removed.** Earlier versions of this fork added a 3.x-style drift correction (`DriftCorrector`), with "Drift correction" and "Drift tolerance (ms)" settings under Preferences. Repeating and dropping samples hides exactly what this section shows, and the settings were confusing, so they are gone. Upstream's drift measurement (`DriftCalculator`) still runs but is no longer shown.
- **Fixed in passing:**
  - `Acquisition::stop()` destroyed the buffer handler before the circular buffer, whose destructor uses it;
  - `CNetAcqControlAPI::pushRequest()` added requests to the queue without its lock;
  - `CircularBuffer::setRunning()` could miss waking a thread that was about to wait.

**Consequences**
- The server's CPU use with the generic oscillator went from about 2.4 cores while playing (1 core at idle) to about 0.02 cores while playing and 0 at idle. More of a modest PC is left for Designer, which writes the recording.
- A stall shows as a rising amplifier-buffer level before anything is lost; a loss, if any, is counted and shown.
- Recordings contain exactly the samples the amplifier delivered: the server repeats or drops none.
- Sessions saved with the old drift correction keys still load; the keys are ignored (2.14).
- The circular buffer's waiting and free count have unit tests (5 in `CircularBufferTest.cpp`).
- **Not yet verified on hardware:** the TMSi buffer level and overflow count, and the eego backlog and sample-counter check. **Not yet verified end to end:** a Designer recording through the changed network server.

### 2.5 Exact trigger placement

**Files**
- `CDriverTMSi.cpp` and `CDriverEEGO.cpp` (they now implement `getData()` with per-sample timestamps)
- `extras/plugins/processing/acquisition/src/box-algorithms/BoxAcquisitionClient.cpp`

**What it does**
- Both drivers give every sample its own timestamp and date each trigger with its sample's timestamp.
- The Acquisition client box places each stimulation on the last sample stamped at or before the stimulation's date. Among samples that share a stamp, it picks the first one.

**Consequences**
- A trigger lands on the sample where the amplifier saw it, not at the start or end of the block it arrived in. Without this, its position could be off by up to one block.

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

### 2.7 Channel names and montage sent to the client

**Files**
- `sdk/openvibe/include/openvibe/acquisition-messaging/ovMsg.h`
- `sdk/openvibe/include/openvibe/acquisition-messaging/ovChannelInfo.h` (new)
- `sdk/openvibe/include/openvibe/acquisition-messaging/ovMsgSerializer.h`
- `extras/applications/platform/acquisition-server/abstractions/writer/AbstractWriter.h`
- `extras/applications/platform/acquisition-server/abstractions/writer/AbstractWriter.cpp`
- `extras/applications/platform/acquisition-server/core/Acquisition.h` and `.cpp`
- `extras/applications/platform/acquisition-server/abstractions/driver/AbstractDriver.h` (`montageSetting()`)
- `extras/plugins/processing/acquisition/src/box-algorithms/BoxAcquisitionClient.h` and `.cpp`
- `extras/applications/platform/acquisition-server/test/OvMsgSerializerTest.cpp` (new)

**What it does**
- The `OV_MSG_ACQUISITION_CONFIG` message now carries the channel names, each ended by `\0`, after the three counts. The wire format is `[type][size][channelCount][sampleRate][sampleChunk][names…]`.
- The serializer writes and reads this format field by field, instead of copying the in-memory struct.
- After the names, when a montage named the channels, come the montage's file name and one position per channel, `<theta> <phi>` in degrees or empty (`ovChannelInfo.h` defines the format, in one place for both sides). The server reads them from the driver's montage (`montageSetting()`, implemented by TMSi and eego) when the acquisition starts, matched to the channels by name.
- `AbstractWriter::writeAcquisitionConfig()` takes the names and montage (`ovChannelInfo`) as a new argument. This changes the API.
- The Acquisition client box passes the montage on to the scenario as two configuration tokens of the run, `${AcquisitionServer_Montage}` and `${AcquisitionServer_ElectrodePositions}`, before it sends the signal header. The UG BrainVision writer reads them (2.9). Unlike a stream, they survive boxes between the two.
- The Acquisition client box labels its signal channels with the received names. A channel without a name, or a server that sends no names, gets numbered from 1.
- The client now frees the arrays `deserializeOvMsg()` allocates for every message. Upstream leaked them for the whole recording.

**Consequences**
- Channel names typed in the acquisition server appear in the Signal display, in the channel selectors and in every file writer (BrainVision, GDF, CSV, …). Upstream shows 1, 2, 3, ….
- The montage is chosen once, in the server. `${AcquisitionServer_Montage}` can also be used in a scenario's settings, e.g. in a file name.
- **Compatibility:** a UG client reading an upstream server's message (20 bytes, no names) falls back to numbers; this case is covered by a unit test. A client from before the montage reads the names and ignores the rest; a server from before it sends no montage. The upstream client reads the three counts the same way, so it should take a UG server's message, but this has not been tested.
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

This is a new box next to upstream's "BrainVision Format file writer (Gipsa)", which is unchanged. It sits in *File reading and writing/BrainVision Format* and is at version 1.4.

Settings:

| Setting | Default | Notes |
|---|---|---|
| Output directory | `${Path_Documents}/data` | Created if it does not exist |
| Header filename | `record-[$core{date}-$core{time}].vhdr` | A filename with a full path ignores the directory |
| Binary format | `IEEE_FLOAT_32` | Gipsa's default is `INT_16` |

Version 1.3 had a fourth setting, *Montage (electrode positions)*. It is gone: the montage is the acquisition server's (2.7). Scenarios saved with it still open, and it is ignored.

Behaviour:
- `.eeg` and `.vmrk` files are written next to the `.vhdr`.
- The full path is logged when recording starts.
- **Only hardware triggers** (`OVTK_StimulationId_Label(1..255)`) are written to the `.vmrk`. They are named as BrainVision Recorder names them: `Mk2=Stimulus,S  1,…`, `S 12`, `S233`.
- All other stimulations (OpenViBE's own, e.g. experiment start or end) are left out.
- **Electrode positions**, when the acquisition server named the channels with a montage. The header gets a `[Coordinates]` section, `Ch<n>=1,<theta>,<phi>` in BrainVision's convention (theta negative on the left), and the montage's name under `[Comment]`.
  - Channels are matched to electrodes by name. A channel the montage does not place on the head (EOG, ECG) gets `0,0,0`.
  - Positions come from the montage's `Theta`/`Phi` when it has them (Charmeleon writes these for the equidistant layout). Otherwise they come from the head map: theta = 22.5° × ring, phi = angle. The server works them out (`loadCharmeleonMontage()`, 2.3); the writer only turns them into BrainVision's convention.

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

**Files:** `extras/plugins/processing/simple-visualization/src/box-algorithms/CSignalDisplay.cpp`, `extras/plugins/processing/simple-visualization/qml/StimulationOverlay.qml`

**What it does:** stimulations in the label range (`OVTK_StimulationId_Label(1..255)`) are shown with their trigger value. Other stimulations keep their name or `Id …`.

The label sits in a flag at the top of the marker line:
- The background is the marker's colour, the contour a darker shade of it, and the text bold and black. Black has a contrast of at least 6.8:1 on every colour of the palette.
- Flags that would cover each other's code step down, over up to 4 rows.
- At the right edge, a flag slides back so that its code stays visible.

**Consequence:** a hardware trigger shows as `1`, `12`, `233`. Upstream shows `OVTK_StimulationId_Label_01`, or a raw ID, as small rotated text. A long stimulation name makes a long flag, which covers more of the top channels.

**Older scenarios.** Upstream renamed the box's settings `Horizontal ruler` and `Vertical ruler` to `Bottom ruler` and `Left ruler`, but 35 of its own scenarios, with 67 of these boxes, still had the earlier names, and failed on Play with `Missing setting 'Bottom ruler'`. Those scenarios now have the new names. A scenario of your own from before the rename fails in the same way: replace the two names in its file, or delete the box and add it again.

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

**What it does:** the driver parameters stored for TMSi and eego follow their new settings. Sessions saved by earlier versions of the fork may carry `driftCorrectionPolicy`, `driftCorrection` and `driftToleranceMs`. These are ignored, and the next save drops them.

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

### 2.17 No console windows; output in a log file (new)

**Files**
- `sdk/openvibe/include/openvibe/ConsoleLog.hpp` (new: `redirectConsoleToLogFile()`)
- `extras/applications/platform/acquisition-server/main.cpp`, `CMakeLists.txt` (`WIN32_EXECUTABLE`)
- `designer/applications/platform/designer/src/main.cpp`

**What it does**
- The acquisition server is built as a windowed application, as Designer already was, so it opens no console window.
- What either application prints (the console log, and driver messages such as `[TMSi] …`) goes to `%APPDATA%\OpenVIBE-4.0\log\`:
  - `openvibe-acquisition-server-console.log`
  - `openvibe-designer-console.log`

  These sit next to the kernel's own `openvibe-<application>.log`. Each start keeps the previous run's file as `….previous.log`. The file is written unbuffered and can be read while the application runs.
- When the application is started with its output redirected (to a file or a pipe), the output stays there.
- Designer no longer attaches to the console of the program that started it.

**Consequences**
- No command window next to either application, also when it is started from a terminal or a launcher script. Start the `.exe` (or the Start Menu shortcut) rather than a `.cmd`: a launcher script still shows its own window.
- After a recording that went wrong, the previous run's messages are still there.
- Started from a terminal, the applications print nothing there; read the log file, or redirect the output.

### 2.18 P300 speller display, ported to Qt (new)

[`speller.md`](speller.md) is the how-to, and has the details of the port: the task, the setup, the code, the timing and what has been tested.

**Files**
- `speller.md` (new)
- `extras/plugins/processing/simple-visualization/`: the box, `CP300SpellerView` and `P300SpellerState` (new), `qml/P300SpellerVisualization.qml` (new), the tests
- `designer/applications/platform/designer/src/PlayerBridge.cpp`
- `extras/applications/platform/acquisition-server/builtin/drivers/tagging/TaggingDriver.cpp`, `TagDate.h` (new), `abstractions/writer/AbstractWriter.cpp`, `test/TcpTagTimingTest.cpp` (new)
- `extras/applications/demos/p300/bci-examples/p300-speller-python/` and `p300-speller-xDAWN-python/` (new: scenarios and scripts)
- `extras/contrib/plugins/processing/python3/share/ovstimulationbox.py` (new)

**What it does**
- The display box was GTK code, left out of upstream's build. It now runs on Qt, with the same box ID and settings, so the speller scenarios open as they are. The Identifier Card and Magic Card displays are still not ported.
- Designer's player sleeps 1 ms between rounds instead of 50 ms. A stimulus can only change at the start of a round.
- Next to the Lua scenarios there is a set that uses Python scripts.
- `OVTK_StimulationId_LabelEnd` is added to the Lua and Python stimulation codes. Upstream's P300 Lua scripts use it, and stopped with an error without it: the online scenarios at the first flash.

**Consequences**
- **Every scenario in Designer now runs in shorter rounds.** Flashes come within about a frame of the time asked for, where they could be up to 60 ms off. Measured without a window, the CPU use hardly changes. A normal recording with this change has not been tested.
- **A TCP Tagging marker is dated at the time it carries.** Upstream's `TaggingDriver` drops it, and dates a marker when it reads it. A marker that gets to the server before its sample waits for the chunk that holds that sample (`AbstractWriter`). This holds for every program that sends tags. A tag whose time can't be from this PC is dated on arrival, as before.
- Drag the box into the scenario's Window Manager before Play, as for any visualization in this Designer: the layout is not saved with the scenario.
- Hardware triggers use the same codes as the speller's rows and columns, so keep them out of a speller session.

### 2.19 Scenarios on the current Acquisition client; quiet connections

[`speller.md`, 3.6](speller.md#36-the-scenarios-and-the-server) has the details.

**Files**
- 31 demo and tutorial scenarios (`*.xml`) with an Acquisition client box
- `sdk/modules/network/Server/ConnectedClient.cpp`, `sdk/modules/network/Client/CClient.cpp`
- `extras/plugins/server-extensions/tcp-tagging-service/test/test_tcptaggingservice.cpp`

**What it does**
- All 31 scenarios with `Acquisition client (Legacy)`, which connects to this server and gets nothing, without an error, now have the current `Acquisition client`, on port 8088. Links from its two remaining outputs moved along, and links from the other three are gone.
- The network library no longer takes a socket's 5 s timeout for a lost connection, in the server's read loop and in the client's. Upstream's server stopped reading a client after 5 s of silence, so a TCP tagging client that paused that long, as the speller does before its first marker, lost all its tags. Upstream's client closed the connection after 5 s without data, so a scenario started before the server stayed without a signal.

**Consequences**
- A scenario of your own with the legacy box needs the same change: delete the box and add `Acquisition client`, then make its links again. The legacy box is still there, for a server of the previous kind.
- The Signal display in `erp-1-monitor-signal` and `network-acquisition` no longer gets channel units from the box, which has no such output.
- New recordings have no experiment information.
- A client that stops playing is now heard when it asks to stop its stream, where the server had stopped reading it.
- Only the speller's Lua and Python sets have run through the whole task, without an amplifier. Of the 31, 24 got a signal when opened against the server; the other 7 stop for other reasons, listed in `speller.md`: unported boxes, no trained filter, and one file Designer can't open, as before.

### 2.20 Late TCP markers get their own sample

[`speller.md`, 3.3](speller.md#33-timing) has the details.

**Files**
- `extras/plugins/processing/acquisition/src/box-algorithms/StimulationDelay.h` (new), `BoxAcquisitionClient.h` and `.cpp`
- `extras/plugins/processing/acquisition/test/` (new), `extras/plugins/processing/acquisition/CMakeLists.txt`

**What it does**
- The Acquisition client box sends each chunk of signal as it comes, and the marker chunk with the same times later, by the number of chunks that covers 100 ms. Until then a marker that comes late can still go to the chunk that holds its sample. Upstream sends them together, and places a marker within the chunk it came with.
- Markers are now placed in `process()`, where the scenario's time is known, and no longer on the network thread. So a marker that comes before the box has sent its header can't get a wrong date.

**Consequences**
- All stimulations from the box, hardware triggers too, reach the scenario 100 ms or more later, by a whole chunk when a chunk is longer than that. The signal is not delayed, and epochs longer than this are not either.
- Each stimulation is still dated within its chunk, so boxes need no change. A Lua script not in filter mode that sends a stimulation on with the date it came with can see that date as too old, and the box then drops it with a warning. No such warning appeared in the speller's runs, but a script of your own might do this.
- The markers of the last moments before stopping are not sent.
- Measured without an amplifier only. Before this change, about a quarter of the flash markers landed up to 30 ms late; with it, all landed on their own sample.

### 2.21 LSL Output keeps the times of samples and markers

**Files**
- `extras/applications/platform/acquisition-server/builtin/writers/lsl-output/LSLOutputWriter.cpp` and `.h`
- `AbstractWriter.h` and `Acquisition.cpp`: a writer now gets the montage as well as the channel names
- `extras/applications/platform/acquisition-server/test/lsl-output/` (new)
- `pixi.toml`, `pixi.lock`, `conda/*`, `CMake/FindThirdPartyLSL.cmake`: liblsl 1.17.5

**What it does**
- **Times.** A sample and a marker go out with the time the server gave them, converted to LSL's clock. Upstream uses the moment of sending for both, which gives all samples of a chunk one time and puts a marker at the end of its chunk: up to 125 ms from its sample at 256 Hz in chunks of 32. As LSL carries the time with the data, a marker that comes late needs no delay here, unlike in the Acquisition client (2.20).
- **Stream type and channels.** The signal's type is `EEG`, which clients search for, where upstream has `signal`. With a montage, a channel it places on the head has type `EEG` and a `location`, any other has type `AUX`, and the montage's name is under `cap`. Without one, a channel has only its label, as a driver doesn't tell an EEG channel from a bipolar one.
- **No unit.** Upstream's `unknown` is gone, and no unit is claimed. The drivers differ: eego's SDK delivers volts and the driver passes them on, and the others are not verified.
- **Locations** are in millimetres, on a head of an assumed 85 mm radius, as the montage gives directions only. No head was measured.
- **Stream ID.** It is made of the computer's name and the stream's name, where upstream draws a random one at each start. So a client can recover the stream after the server restarts.
- **Fixes.** `send()` returned nothing for any message but samples, which is undefined behaviour and happened whenever a client connected. The writer now registers from its `.cpp`, as a second file including the header would register it again.
- **liblsl 1.17.5**, from 1.16.2. It no longer sends a garbage sample when an outlet is destroyed, which this writer does at each stop. 1.17.7 is the latest stable version, and conda-forge has 1.17.5.

**Measured** on the real server with the Generic Oscillator, at 256 Hz in chunks of 32, with an LSL client on the same PC. It sent 80 TCP tags dated 0, 5, 40 and 150 ms back:
- All 80 markers arrived, within 0.26 ms of their dates.
- No two samples shared a time. Successive times differ by 1.9 to 5.9 ms, as they are interpolated between the driver's reads, and average 3.906 ms, which is the sample period.
- A chunk reached the client 2 ms after its last sample's time.
- After the server was killed and restarted, the open client got data again by itself, with the same stream ID, 5.6 s of stamps later.

**Consequences**
- A client that compares LSL times with its own stimulus clock sees a correct signal time for the first time, so a correction made for the old offset has to go.
- A marker stream is still `int32`, so a stimulation code above 31 bits doesn't fit, as before, and its duration is not sent.
- Not tested with an amplifier, with a client on another PC, or with LabRecorder. With an amplifier the stamps depend on how its blocks of samples are read.

### 2.22 A picked file reaches the box

**Files**
- `extras/applications/qml/OVParameters/src/parameterPathObject.cpp`
- `extras/applications/qml/OVParameters/test/` (new), `extras/applications/qml/OVParameters/CMakeLists.txt`

**What it does**
- A path setting now holds forward slashes. Upstream stores a picked file with Windows's backslashes. A box's setting goes through the kernel's token expansion, where a backslash is an escape character, so `C:\a\b.ov` reached the box as `C:ab.ov`, and it could not open a file picked with the file dialog.
- A Windows path typed or pasted into the field gets the same treatment.

**Consequences**
- A scenario saved earlier with backslashes in a path still has them: pick the file again.
- Measured with the speller's training scenario: with backslashes the Generic stream reader stops with `Error opening file`; with forward slashes it trains. The file dialog itself was not clicked through: the test gives the control the kind of address the dialog gives.

---

## 3. All modified files

`A` = added, `M` = modified, `D` = removed. The section column refers to the sections above.

| Status | File | Section |
|---|---|---|
| M | `CMake/FindThirdPartyLSL.cmake` | 2.21 |
| M | `conda/env_linux.yaml` | 2.21 |
| M | `conda/env_osx.yaml` | 2.21 |
| M | `conda/env_osx_arm64.yaml` | 2.21 |
| M | `conda/env_windows.yaml` | 2.21 |
| M | `conda/recipe/meta.yaml` | 2.21 |
| M | `designer/applications/platform/designer/src/DesignerScenario.cpp` | 2.12 |
| M | `designer/applications/platform/designer/src/PlayerBridge.cpp` | 2.18 |
| M | `designer/applications/platform/designer/src/main.cpp` | 2.17 |
| M | `extras/applications/platform/acquisition-server/CMakeLists.txt` | 2.1, 2.4, 2.6, 2.17 |
| M | `extras/applications/platform/acquisition-server/abstractions/CMakeLists.txt` | 2.3 |
| M | `extras/applications/platform/acquisition-server/abstractions/driver/AbstractDriver.cpp` | 2.16 |
| M | `extras/applications/platform/acquisition-server/abstractions/driver/AbstractDriver.h` | 2.4, 2.6, 2.7, 2.8, 2.16 |
| A | `extras/applications/platform/acquisition-server/abstractions/driver/CharmeleonMontage.cpp` | 2.3 |
| A | `extras/applications/platform/acquisition-server/abstractions/driver/CharmeleonMontage.h` | 2.3 |
| M | `extras/applications/platform/acquisition-server/abstractions/writer/AbstractWriter.cpp` | 2.7 |
| M | `extras/applications/platform/acquisition-server/abstractions/writer/AbstractWriter.h` | 2.7, 2.21 |
| A | `extras/applications/platform/acquisition-server/builtin/drivers/tmsi/CDriverTMSi.cpp` | 2.1, 2.3, 2.4, 2.5, 2.6, 2.16 |
| A | `extras/applications/platform/acquisition-server/builtin/drivers/tmsi/CDriverTMSi.h` | 2.1, 2.3, 2.4, 2.6, 2.7 |
| M | `extras/applications/platform/acquisition-server/builtin/writers/lsl-output/LSLOutputWriter.cpp` | 2.21 |
| M | `extras/applications/platform/acquisition-server/builtin/writers/lsl-output/LSLOutputWriter.h` | 2.21 |
| M | `extras/applications/platform/acquisition-server/core/Acquisition.cpp` | 2.4, 2.6, 2.7, 2.21 |
| M | `extras/applications/platform/acquisition-server/core/Acquisition.h` | 2.4, 2.6, 2.7, 2.8, 2.16 |
| M | `extras/applications/platform/acquisition-server/core/CircularBuffer.cpp` | 2.4 |
| M | `extras/applications/platform/acquisition-server/core/CircularBuffer.h` | 2.4 |
| M | `extras/applications/platform/acquisition-server/core/net-api/CNetAcqControlAPI.cpp` | 2.4 |
| M | `extras/applications/platform/acquisition-server/core/net-api/CNetAcqControlAPI.h` | 2.4 |
| M | `extras/applications/platform/acquisition-server/main.cpp` | 2.17 |
| M | `extras/applications/platform/acquisition-server/settings/SessionManager.cpp` | 2.14 |
| M | `extras/applications/platform/acquisition-server/settings/SessionManager.h` | 2.14 |
| A | `extras/applications/platform/acquisition-server/test/CharmeleonMontageTest.cpp` | 2.3 |
| A | `extras/applications/platform/acquisition-server/test/CircularBufferTest.cpp` | 2.4 |
| A | `extras/applications/platform/acquisition-server/test/OvMsgSerializerTest.cpp` | 2.7 |
| M | `extras/applications/platform/acquisition-server/ui/AcquisitionController.cpp` | 2.4, 2.6, 2.8, 2.16 |
| M | `extras/applications/platform/acquisition-server/ui/AcquisitionController.h` | 2.4, 2.6, 2.8, 2.16 |
| A | `extras/applications/platform/acquisition-server/ui/Charmeleon.cpp` | 2.6 |
| A | `extras/applications/platform/acquisition-server/ui/Charmeleon.h` | 2.6 |
| M | `extras/applications/platform/acquisition-server/ui/qml/ChannelRename.qml` | 2.8 |
| A | `extras/applications/platform/acquisition-server/ui/qml/BufferControl.qml` | 2.4 |
| D | `extras/applications/platform/acquisition-server/ui/qml/DriftBar.qml` | 2.4 |
| D | `extras/applications/platform/acquisition-server/ui/qml/DriftControl.qml` | 2.4 |
| A | `extras/applications/platform/acquisition-server/ui/qml/DriverNote.qml` | 2.16 |
| A | `extras/applications/platform/acquisition-server/ui/qml/LevelBar.qml` | 2.4 |
| M | `extras/applications/platform/acquisition-server/ui/qml/Main.qml` | 2.4, 2.8, 2.16 |
| M | `extras/applications/qml/OVParameters/CMakeLists.txt` | 2.22 |
| M | `extras/applications/qml/OVParameters/PathParameter.qml` | 2.13 |
| M | `extras/applications/qml/OVParameters/src/parameterPathObject.cpp` | 2.13, 2.22 |
| M | `extras/applications/qml/OVParameters/src/parameterPathObject.h` | 2.13 |
| A | `extras/applications/qml/OVParameters/test/CMakeLists.txt` | 2.22 |
| A | `extras/applications/qml/OVParameters/test/ParameterPathObjectTest.cpp` | 2.22 |
| A | `extras/contrib/plugins/processing/file-io/src/box-algorithms/ovpCBoxAlgorithmBrainampFileWriterUG.cpp` | 2.9 |
| A | `extras/contrib/plugins/processing/file-io/src/box-algorithms/ovpCBoxAlgorithmBrainampFileWriterUG.h` | 2.9 |
| M | `extras/contrib/plugins/processing/file-io/src/ovp_defines.h` | 2.9 |
| M | `extras/contrib/plugins/processing/file-io/src/ovp_main.cpp` | 2.9 |
| M | `extras/contrib/plugins/server-drivers/CMakeLists.txt` | 2.2 |
| M | `extras/contrib/plugins/server-drivers/eemagine-eego/src/CDriverEEGO.cpp` | 2.2, 2.3, 2.4, 2.5, 2.6, 2.16 |
| M | `extras/contrib/plugins/server-drivers/eemagine-eego/src/CDriverEEGO.h` | 2.2, 2.3, 2.4, 2.6, 2.7 |
| M | `extras/plugins/processing/acquisition/CMakeLists.txt` | 2.20 |
| M | `extras/plugins/processing/acquisition/src/box-algorithms/BoxAcquisitionClient.cpp` | 2.5, 2.7, 2.20 |
| M | `extras/plugins/processing/acquisition/src/box-algorithms/BoxAcquisitionClient.h` | 2.7, 2.20 |
| A | `extras/plugins/processing/acquisition/src/box-algorithms/StimulationDelay.h` | 2.20 |
| A | `extras/plugins/processing/acquisition/test/CMakeLists.txt` | 2.20 |
| A | `extras/plugins/processing/acquisition/test/StimulationDelayTest.cpp` | 2.20 |
| M | `extras/applications/platform/acquisition-server/abstractions/writer/AbstractWriter.cpp` | 2.18 |
| A | `extras/applications/platform/acquisition-server/builtin/drivers/tagging/TagDate.h` | 2.18 |
| M | `extras/applications/platform/acquisition-server/builtin/drivers/tagging/TaggingDriver.cpp` | 2.18 |
| A | `extras/applications/platform/acquisition-server/test/TcpTagTimingTest.cpp` | 2.18 |
| M | `extras/applications/platform/acquisition-server/test/CMakeLists.txt` | 2.21 |
| A | `extras/applications/platform/acquisition-server/test/lsl-output/CMakeLists.txt` | 2.21 |
| A | `extras/applications/platform/acquisition-server/test/lsl-output/LSLOutputWriterTest.cpp` | 2.21 |
| A | `extras/applications/demos/p300/bci-examples/p300-speller-python/` (5 scripts, 5 scenarios and a README) | 2.18 |
| A | `extras/applications/demos/p300/bci-examples/p300-speller-xDAWN-python/` (6 scenarios, and a README) | 2.18 |
| M | `extras/contrib/plugins/processing/python3/share/StimulationsCodes.py` | 2.18 |
| A | `extras/contrib/plugins/processing/python3/share/ovstimulationbox.py` | 2.18 |
| M | `extras/plugins/processing/stimulation/share/lua-stimulator-stim-codes.lua` | 2.18 |
| M | `extras/plugins/processing/simple-visualization/CMakeLists.txt` | 2.18 |
| M | `extras/plugins/processing/simple-visualization/doc/Doc_BoxAlgorithm_P300SpellerVisualization.dox-part` | 2.18 |
| A | `extras/plugins/processing/simple-visualization/qml/P300SpellerVisualization.qml` | 2.18 |
| M | `extras/plugins/processing/simple-visualization/src/box-algorithms/CBoxAlgorithmP300SpellerVisualization.cpp` | 2.18 |
| M | `extras/plugins/processing/simple-visualization/src/box-algorithms/CBoxAlgorithmP300SpellerVisualization.hpp` | 2.18 |
| A | `extras/plugins/processing/simple-visualization/src/box-algorithms/CP300SpellerView.cpp` | 2.18 |
| A | `extras/plugins/processing/simple-visualization/src/box-algorithms/CP300SpellerView.h` | 2.18 |
| M | `extras/plugins/processing/simple-visualization/src/box-algorithms/CSignalDisplay.cpp` | 2.11 |
| M | 35 demo and tutorial scenarios (`*.xml`) with a Signal display | 2.11 |
| M | 31 demo and tutorial scenarios (`*.xml`) with an Acquisition client box | 2.19 |
| M | `extras/plugins/server-extensions/tcp-tagging-service/test/test_tcptaggingservice.cpp` | 2.19 |
| A | `extras/plugins/processing/simple-visualization/src/box-algorithms/P300SpellerState.cpp` | 2.18 |
| A | `extras/plugins/processing/simple-visualization/src/box-algorithms/P300SpellerState.h` | 2.18 |
| M | `extras/plugins/processing/simple-visualization/src/main.cpp` | 2.18 |
| M | `extras/plugins/processing/simple-visualization/test/CMakeLists.txt` | 2.18 |
| A | `extras/plugins/processing/simple-visualization/test/P300SpellerStateTest.cpp` | 2.18 |
| A | `extras/plugins/processing/simple-visualization/test/scenarios-tests/P300-speller-lua-test.xml` | 2.18 |
| A | `extras/plugins/processing/simple-visualization/test/scenarios-tests/P300-speller-python-check.py` | 2.18 |
| A | `extras/plugins/processing/simple-visualization/test/scenarios-tests/P300-speller-python-test.xml` | 2.18 |
| A | `extras/plugins/processing/simple-visualization/test/scenarios-tests/P300-speller-test.xml` | 2.18 |
| M | `pixi.lock` | 2.15, 2.21 |
| M | `pixi.toml` | 2.15, 2.21 |
| M | `sdk/kernel/src/kernel/ovkCKernelContext.cpp` | 2.10 |
| M | `sdk/modules/network/Client/CClient.cpp` | 2.19 |
| M | `sdk/modules/network/Server/CServer.cpp` | 2.4 |
| M | `sdk/modules/network/Server/ConnectedClient.cpp` | 2.19 |
| A | `sdk/openvibe/include/openvibe/ConsoleLog.hpp` | 2.17 |
| A | `sdk/openvibe/include/openvibe/acquisition-messaging/ovChannelInfo.h` | 2.7 |
| M | `sdk/openvibe/include/openvibe/acquisition-messaging/ovMsg.h` | 2.7 |
| M | `sdk/openvibe/include/openvibe/acquisition-messaging/ovMsgSerializer.h` | 2.7 |
| A | `UG.md` | this document |
| A | `montages.md` | 2.3: the montage how-to |
| A | `speller.md` | 2.18: the speller how-to |

---

## 4. Overall consequences and caveats

- **Platform.** The TMSi driver and the Charmeleon integration are Windows-only. On Linux and macOS the fork builds and behaves like upstream, apart from the eego build, the writer, the buffer display and the fixes.
- **Mixing versions.** Use the UG acquisition server with the UG Designer or Scenario Player. Mixing with upstream should work (see 2.7) but loses the channel names and the trigger placement fix, and has not been tested.
- **Scenarios.** Scenarios using the Gipsa writer are unaffected. To get Recorder-style markers and the output directory setting, replace that box with the UG writer.
- **Recorded data.** Recordings contain exactly the samples the amplifier delivered. The server repeats or drops none, and samples the amplifier lost are counted and shown (2.4).
- **Merging upstream.** The fork changes shared code that upstream may also change, so these are the likely merge conflicts:
  - acquisition message format and serializer
  - `AbstractWriter` signature
  - `Acquisition`, `CircularBuffer`, `AcquisitionController`, `SessionManager`
  - the network control API and `CServer`
  - Designer's and the server's `main.cpp`
  - `DesignerScenario::clickPort`
  - the kernel context
  - `pixi.toml` and `pixi.lock`

  New files (TMSi driver, Charmeleon, UG writer, buffer display) merge cleanly.
- **Charmeleon.** The montage round trip and the choice of amplifier need Charmeleon 2.2 (see 2.6). Charmeleon is a separate repository and installer.
- **Tests.** The acquisition server unit tests (`test-acquisition-server-core`, 53 tests) pass. They include new tests for the circular buffer, the config message and the montage rules. So do the P300 speller tests (`test-p300-speller`, 12 tests), for the letter grid, what the display shows and when a frame showed it.
- **Tested without an amplifier:**
  - the montage note and locked entries;
  - the error under the settings when Play fails;
  - the round trip with Charmeleon 2.2 in demo mode: the server starts it, it saves on closing, and the driver takes the montage over;
  - the fallback for an older Charmeleon;
  - the buffers panel and the server's CPU use, with the generic oscillator;
  - both applications starting without a console window and writing their log files;
  - the P300 speller display in Designer: the stimulator's flashes, targets and selections on screen, and the markers reaching a stand-in for TCP Tagging (see [speller.md](speller.md)).
- **Not yet verified on hardware:**
  - the TMSi and eego channel lists, montage mapping, channel hints, and channel names in the Signal display;
  - Charmeleon opening the driver's device;
  - TMSi impedances on BIP and AUX channels;
  - the TMSi buffer level and overflow count, and the eego sample-counter check.
- **Not yet verified end to end:** a Designer recording through the changed network server (2.4), and a P300 speller session: recording, training and online spelling with an amplifier and the real acquisition server (2.18).
- **Not yet run in Designer:** the montage name and electrode positions the UG writer writes, now that they come from the server (2.7, 2.9). The message format and the montage reading have unit tests.
