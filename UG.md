# OpenViBE UG: differences from upstream OpenViBE

This repository (`markspan/openvibe-ug`) is a fork of the Inria OpenViBE 4.0 repository
(`https://gitlab.inria.fr/openvibe/openvibe`). It adapts OpenViBE to the EEG setup of the UG lab:

- TMSi and eemagine eego amplifiers
- Charmeleon for montages and impedance checks
- BrainVision files as the recording format

This document lists every change against upstream, what each change does and what it means in use.

**In the code**, every place where the fork differs from upstream is marked with a comment that starts with `UG fork:`, in C++, QML, CMake and TOML alike. A new file says so at its top, and so does a file rewritten as a whole (the P300 speller box, the eego driver). A new file ported or copied from another, the TMSi driver from the 3.x driver and the BrainVision writer (UG) from the Gipsa writer, also marks what differs from that original. The files marked 2.23 in section 3 say it once, above their includes (see 2.23). Files that can not carry a comment, or that a program rewrites, carry none: scenarios (`*.xml`, which Designer writes anew on every save), documentation (`*.dox-part`), `pixi.lock` and test input data. Section 3 lists them all the same. To list the tags:

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
| Fork | `main`, 93 commits on top of that base (`87afc64` onwards) |
| Upstream commits not in the fork | none at the time of writing |
| Size | 394 files, about 37,700 lines added and 4,400 removed (of which 3,000 a test input) |

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
| Samples from a backlog | A block from the SDK that takes several reads is dated too late, by up to the backlog, triggers included (TMSi, eego) | Each sample is dated by its place in the block |
| TCP Tagging timing | Markers dated when the server reads them, and sent with the next chunk | Dated at the time in the tag, and sent with the chunk that holds their sample |
| Hardware triggers | n/a (TMSi) / flank-decoded (eego) | Sent as `OVTK_StimulationId_Label(value)`, value 1–255, not inverted |
| Trigger lines switching | A value caught while the trigger lines switch is an extra trigger with a wrong code (eego) | A value counts once it has held for two samples, and is dated at the first sample of the change (TMSi, eego) |
| Designer configuration | A `designer.conf` that is not found is skipped without a word; the user's settings are then not loaded, and every session save fails with `${Designer_CustomConfigurationFile} is empty` | The missing file is named in the log, and the user configuration file falls back to `${Path_UserData}/openvibe-designer.conf` |
| Run time on screen | Only in Designer's action bar, hidden behind the visualisation windows while a scenario runs | Also in a small window above all others, at the top left of Designer's screen, while a scenario runs; black dot while running, amber while paused; a red dot and a red "REC" square while the BrainVision writer (UG) writes; raises itself every second so that it stays on top |
| Comments on the canvas | Shown, edited, moved and deleted, but a new one can not be made: nothing starts it | Edit > Add comment, or Ctrl+Shift+C: the text dialog opens at once |
| Boxes older than their plugin | A warning per box on opening ("should be updated"); the box has to be deleted and added again, losing its settings and links. Boxes whose inputs, outputs or settings the user can add or change can not be updated at all | Updated when the scenario opens, keeping settings, links and what the user added or changed; one Undo step, and the scenario counts as changed until saved |
| Crashes in Designer | QML's garbage collector could delete settings and parameter objects that Designer still used, and Designer crashed later, e.g. on opening a box's settings; the 2D topographic map read its sample points while the window resized them, and crashed | Those objects stay Designer's; the map's points are guarded while they are resized, interpolated and drawn, and a bad point no longer reads outside the spline table |
| Qt on Windows | 6.9.3, whose support has ended | 6.12.0, the long-term-support version; the installer workflow fetches it with its own script, checked against Qt's SHA-256 |
| MATLAB box | Without MATLAB at build time (the GitHub build), an empty `openvibe-plugins-matlab.dll` was installed, and Designer warned at every start: "No 'plugin object descriptor' found" | Such a build leaves the DLL out; a build on a PC with MATLAB finds any release in `C:\Program Files\MATLAB` (was: R2015b only) and builds the box, whose DLL can be copied into an installed OpenViBE |
| Adding ports to a box | On a box that accepts any stream type (MATLAB Scripting, Identity, Signal Merger, Generic Stream Writer, Classifier Trainer and about 15 more), "+" in the box's settings failed: "Could not add input port" | The port is added, as a Signal; its card offers every stream type |
| Copying from the log | Not possible: an entry can not be selected | Click an entry to select it, Ctrl+click to add or remove one, Shift+click for a range; Ctrl+C, or the right-click menu, copies them as text (`[ Error ] message`, one per line) |
| MATLAB example | Three tutorials: a signal zeroed on a key, an FFT filter, a sine generator | A fourth squares the EEG channels of a signal and passes the others on, to test the MATLAB box; a channel's type is taken from its name, as the Signal display does |
| MATLAB box in Designer | Failed at the first call after Play ("An error occurred while updating the box clock"): the engine was opened on Designer's window thread and called from its play thread | Every engine call runs on one thread of the box's own; when MATLAB is not registered under the name the engine asks for, the log says so and gives the command that registers it |
| MATLAB type IDs | `OV_define.m` filled the log with `flintmax` warnings at every Play, and its 64-bit type IDs came out rounded | The IDs are read exactly, without warnings |
| Left ruler help | The ruler's "±5" did not say that a lane is centred on the channel's middle, not on 0 | Hovering over "Left ruler" explains the range |
| Lane statistics | A lane's ruler gave its scale but not what the channel did | Hovering over a lane's bracket shows min, max and range on screen, and what the lane shows; tooltips wrap |
| Dominant frequency | A lane's tooltip said nothing of the rhythm in the signal | It names the dominant frequency of the last 4 s, when one peak from 6 to 100 Hz has at least 3 times the power of the frequencies around it |
| Impedances in BrainVision files | Not recorded; TMSi impedances only in Charmeleon | A scenario with the BrainVision writer (UG) has the server measure the impedances when it starts (the signal stops for about 4 s), and the header lists them as BrainVision Recorder does; eego and TMSi, on the client's Channel localisation link |
| LSL times | The LSL driver dates samples and markers when it reads them, and passes marker 7 on as stimulation 7, which the BrainVision writer leaves out; LSL Export dates everything from its first chunk, as if that came at time 0 | Both keep the times LSL asks for: the driver its sender's, on this PC's clock, with markers 1 to 255 as triggers; LSL Export each time on LSL's clock by its age in the scenario |
| LSL streams across a restart | Closed and opened again at every Play, so a LabRecorder recording sees them go and come back | Kept open when the acquisition starts again with the same streams, as for the impedance measurement; LabRecorder sees a pause |
| Apology in error messages | Always "We do most humbly apologise:" (2.23) | One of several, at random, aware of the errors before it: "And, sorry again:" right after another error, "I am afraid this is the 3rd time:" for a repeated kind |
| Program descriptions | "OpenViBE Designer for Win32", in Task Manager and a file's Details, though the build is 64-bit | "OpenViBE Designer for Windows (64-bit)", and so for every program and DLL with a version resource |
| Window titles and recordings | `OpenViBE Designer`, `Openvibe Acquisition Server`; a BrainVision header says the Vision Recorder made it | With the version after the name, e.g. `OpenViBE Designer 4.0.1-ug.6`, so a screenshot says which build ran; the BrainVision header (UG) names the OpenViBE version that wrote it |
| Acquisition server icon | The same as Designer's | Its own, in the same style: three channels merging into one |
| Editing a comment | The editor opens at the comment's place on the canvas taken as a place on the screen, often off the screen | It opens in the middle of Designer's window |
| Installer on GitHub | none; the installer is built on Inria's GitLab runners | A GitHub Actions workflow builds the NSIS installer and a ZIP, by hand or on a version tag, and attaches them to a release |
| Channel names in scenarios | Not sent; the client labels channels 1, 2, … | Sent to the Acquisition client box and shown in the Signal display and file writers |
| Signal display | Trigger shown by stimulation name or `Id 33025`, as small rotated text | Hardware trigger shown as its value (`1`, `12`, …), in a coloured flag on the marker; with Colour per channel, each channel name has the colour of its trace before it |
| Signal display scaling | Global scaling shares one centre, so channels with a DC offset leave their lanes and the ruler reads ±1.2e+5; the scale refreshes on a timer, with hysteresis; settings that do not apply stay editable; lines not antialiased | Every channel centred on its own signal, also without automatic scaling, so no Vertical Offset; dense traces shaded by density; a "By type" mode (EEG, EOG, digital); the scale worked out once per page and rounded to 1, 2 or 5; settings that do not apply greyed out; antialiased lines; Select all, Deselect all and Invert selection for the channel list; the settings panel can be hidden |
| BrainVision writer | "(Gipsa)" box writes every stimulation's full ID as a marker | Extra "(UG)" box writes only hardware triggers, as `S  1` … `S255`, into a chosen directory, and the server's montage with its electrode positions |
| LSL streams | Seen through the acquisition server's LSL driver: one data stream and one marker stream, numbers only, timed when they arrive | "LSL display (UG)" box: any number of data streams of any rate, stacked on one time axis by their LSL times, and number and text markers on all; streams ticked while it runs. "LSL XDF writer (UG)" connected to it records the ticked streams as LabRecorder does, a file each time Record is pressed, of each stream only the channels shown |
| BrainVision file integrity | eego data in volt in a microvolt file; TMSi BIP and AUX channels unscaled; markers counted from the scenario's start, not the file's first sample; the header written only at the end | eego in microvolt and its stream order checked; markers on their own sample; files written out at every step; a script that checks a recording |
| P300 speller | Display box not ported to Qt and not built, so 6 of the 11 speller scenarios can not run; the Lua online scripts also stop with an error | Ported, and the error repaired; markers are sent once the flash is on screen; also a set of scenarios with Python scripts |
| Player timing | Designer's player runs in rounds with a 50 ms sleep between them, so stimuli can be up to about 60 ms off | 1 ms sleep |
| Kernel tokens | no `Path_Documents`; the Foldername setting type is unregistered | `${Path_Documents}` added; `Foldername` setting type registered |
| Designer | Clicking two outputs could try to link a box to itself and wedge link creation | Second click on a port of the same kind restarts the link; failures end link creation |
| Path parameters | Re-choosing the same file in the file dialog does nothing | Re-choosing the same file applies it again (reloads an edited montage) |
| Client memory | Acquisition client leaked every received message's arrays | Arrays freed |
| Server restarted under a running scenario | Connected clients keep the first settings; blocks larger than they expect are cut, the rest dropped without a word (a block size of 64 after 4 keeps one sample in 16), other channels are misread | Clients get the new settings; every sample is kept whatever the block size; another rate or other channels stop the scenario with an error that says so |
| Second acquisition server | Starts on the port the first one uses, and Designer connects to either | Says the port is in use, and quits |
| Client box in scenarios | 31 scenarios, among them all of the speller, use the legacy box, which gets nothing from this server | They use the current `Acquisition client`, on port 8088 |
| Quiet connections | A client silent for 5 s is no longer read; a server silent for 5 s loses its client | They keep the connection |
| Late TCP markers | A marker that reaches the server after its chunk has gone lands on the next chunk's first sample | It lands on its own sample, as the Acquisition client sends markers at least 100 ms behind the signal, which itself is not delayed |
| LSL Output | Samples and markers get the moment of sending as their time, so a marker can be a chunk off; a stream ID that changes at each start | They keep the server's times; markers within 0.26 ms of their dates, and a client recovers after a restart |
| File settings on Windows | A file picked with the dialog gets backslashes, which a box loses, so it can't open the file | Forward slashes, so the box opens it; a box also reads backslashes typed in a file or folder setting as separators |
| Error messages | `{Error description} : {Invalid channel [25]}`, and the log panel cuts long lines off | An apology first and developer details last; the Channel Selector explains what is wrong and what to check; long lines wrap |
| Build | Fixed CMake 3.25 / Ninja 1.10 versions, `ninja -C build` | Any CMake ≥ 3.20 and any Ninja, `cmake --build build`; Qt located through `CMAKE_PREFIX_PATH` |

---

## 2. Changes in detail

### 2.1 TMSi driver (new)

**Files**
- `extras/applications/platform/acquisition-server/builtin/drivers/tmsi/CDriverTMSi.h` (new)
- `extras/applications/platform/acquisition-server/builtin/drivers/tmsi/CDriverTMSi.cpp` (new)
- `extras/applications/platform/acquisition-server/CMakeLists.txt` (adds the driver on Windows)
- `documentation/TMSiSDK.chm` (new): TMSi's reference of the SDK the driver calls, for whoever works on the driver

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
- The EEG and bipolar channels are sent in microvolt, as the TMSi driver sends them; the SDK, and upstream, give volt. Start checks that the stream lists the channels in the order they were named (see 2.27).
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
  - **Amplifier buffer.** Shown as the time of data waiting, in milliseconds, with the worst since Play. TMSi: how many samples each read found waiting, which is exact, as a read empties the TMSi library's buffer; and how full the library says its buffer is (`GetBufferInfo`), whichever is more. `GetBufferInfo` gives whole percents of the 10 s buffer, so it reads 0 below 100 ms, which is where a working acquisition stays; the panel first showed only that percentage, and so always 0 %. The bar shows the part of the 10 s used. eego: the SDK does not say how full its buffer is, so the panel shows how much data waited at each read. For both, a reader that is held up counts as a growing backlog.
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
- `extras/applications/platform/acquisition-server/abstractions/driver/CharmeleonDriver.h` and `.cpp` (2.41: `charmeleonArguments()`, `charmeleonMontage()`, `takeOverCharmeleonMontage()`)
- `extras/applications/platform/acquisition-server/CMakeLists.txt`
- `CDriverTMSi.*` and `CDriverEEGO.*`

**What it does**
- Drivers derived from `CharmeleonDriver` (2.41) hand impedances to Charmeleon; eego and TMSi are. The impedance button is then enabled even when the driver has no built-in check (TMSi).
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
- `extras/applications/platform/acquisition-server/abstractions/driver/AbstractDriver.h` and `.cpp` (`channelInfo()`), `CharmeleonDriver.cpp` (2.41)
- `extras/plugins/processing/acquisition/src/box-algorithms/BoxAcquisitionClient.h` and `.cpp`
- `extras/applications/platform/acquisition-server/test/OvMsgSerializerTest.cpp` (new)

**What it does**
- The `OV_MSG_ACQUISITION_CONFIG` message now carries the channel names, each ended by `\0`, after the three counts. The wire format is `[type][size][channelCount][sampleRate][sampleChunk][names…]`.
- The serializer writes and reads this format field by field, instead of copying the in-memory struct.
- After the names, when a montage named the channels, come the montage's file name and one position per channel, `<theta> <phi>` in degrees or empty (`ovChannelInfo.h` defines the format, in one place for both sides). The driver gives them when the acquisition starts (`AbstractDriver::channelInfo()`: the names; `CharmeleonDriver` adds the montage, matched to the channels by name).
- `AbstractWriter::writeAcquisitionConfig()` takes the names and montage (`ovChannelInfo`) as a new argument. This changes the API.
- The Acquisition client box sends the positions on a third output, *Channel localisation* (`OV_TypeId_ChannelLocalisation`, the stream type OpenViBE has for this), once, with the signal header: one row per channel the montage places, by name, x, y, z on a unit sphere in OpenViBE's frame (x to the nose, y to the left ear, z up). The UG BrainVision writer takes it on its third input (2.9). Upstream's box had this output prepared, commented out. Until 2.41 the montage went to the writer as two configuration tokens of the run instead (see 2.41 for why that changed).
- The Acquisition client box labels its signal channels with the received names. A channel without a name, or a server that sends no names, gets numbered from 1.
- The client now frees the arrays `deserializeOvMsg()` allocates for every message. Upstream leaked them for the whole recording.

**Consequences**
- Channel names typed in the acquisition server appear in the Signal display, in the channel selectors and in every file writer (BrainVision, GDF, CSV, …). Upstream shows 1, 2, 3, ….
- The montage is chosen once, in the server. A recording scenario connects the client's *Channel localisation* output to the writer's third input.
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

This is a new box next to upstream's "BrainVision Format file writer (Gipsa)", which is unchanged. It sits in *File reading and writing/BrainVision Format* and is at version 1.5: a third input, *Channel localisation*, takes the electrode positions (2.41).

Settings:

| Setting | Default | Notes |
|---|---|---|
| Output directory | `${Path_Documents}/data` | Created if it does not exist |
| Header filename | `record-[$core{date}-$core{time}].vhdr` | A filename with a full path ignores the directory |
| Binary format | `IEEE_FLOAT_32` | Gipsa's default is `INT_16` |

Version 1.3 had a fourth setting, *Montage (electrode positions)*. It is gone: the montage is the acquisition server's (2.7), and its positions come in on the third input. Scenarios saved with it still open, and it is ignored.

Behaviour:
- `.eeg` and `.vmrk` files are written next to the `.vhdr`.
- The full path is logged when recording starts.
- **Only hardware triggers** (`OVTK_StimulationId_Label(1..255)`) are written to the `.vmrk`. They are named as BrainVision Recorder names them: `Mk2=Stimulus,S  1,…`, `S 12`, `S233`.
- All other stimulations (OpenViBE's own, e.g. experiment start or end) are left out.
- **Electrode positions**, from the third input: the Acquisition client's *Channel localisation* output when the acquisition server named the channels with a montage. The header gets a `[Coordinates]` section, `Ch<n>=1,<theta>,<phi>` in BrainVision's convention (theta negative on the left), and a line under `[Comment]` saying how many channels have a position.
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
- A new link that replaces another redraws the links. An input takes one link, so a link made to an input that already has one replaces it; Designer then rebuilt its list of links but did not tell the canvas. This is the same change as upstream's open merge request !187 ("fix box links refresh").

**Consequence:** in upstream, clicking an output and then another output tried to link a box to itself and failed with "Input index out of range". Link creation then stayed pending, so every later click failed the same way. That no longer happens.

After a link replaced another, upstream's canvas went on showing the old link and not the new one, until something else changed the links (another link, a deletion, an undo, reopening the scenario). The scenario itself, and the file saved, had the new link. Deleting a link in that state could remove a different one, as the canvas deletes a link by its place in the list. Now the canvas shows the new link at once.

### 2.13 Path parameter: re-choosing a file reloads it

**Files**
- `extras/applications/qml/OVParameters/PathParameter.qml`
- `extras/applications/qml/OVParameters/src/parameterPathObject.h`
- `extras/applications/qml/OVParameters/src/parameterPathObject.cpp`

**What it does:** `ParameterPathObject` gains `choose()`, which applies a value even when it equals the current one. The file dialog now uses it.

**Consequence:** choosing the same montage file again after editing it in Charmeleon reloads it. Upstream ignored the choice because the path had not changed. This affects every path parameter in the Qt applications, not only montages.

### 2.14 Session settings

**Files:** none any more. `SessionManager.h` and `.cpp` are back to upstream's, since the drift correction settings they stored are gone (2.4).

**What it does:** the server stores each driver's parameters as the driver lists them, so the TMSi and eego entries follow their new settings without a change here. Sessions saved by earlier versions of the fork may carry `driftCorrectionPolicy`, `driftCorrection` and `driftToleranceMs`. These are ignored, and the next save drops them, as a save writes the session anew.

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
- A scenario saved earlier with backslashes in a path still has them. Since 2.41 a box reads them as separators on Windows anyway.
- Measured with the speller's training scenario: with backslashes the Generic stream reader stops with `Error opening file`; with forward slashes it trains. The file dialog itself was not clicked through: the test gives the control the kind of address the dialog gives.

### 2.23 Error messages that explain themselves, politely

**Files**
- `sdk/openvibe/include/openvibe/ovAssert.h`
- `sdk/plugins/processing/signal-processing/src/box-algorithms/basic/CBoxAlgorithmChannelSelector.cpp`
- `sdk/kernel/src/kernel/player/ovkCSimulatedBox.cpp`, `ovkCScheduler.cpp`, `ovkCPlayer.cpp`
- `designer/applications/platform/designer/src/PlayerBridge.cpp`
- `extras/applications/qml/OVControls/Logger.qml`
- `sdk/plugins/processing/classification/src/box-algorithms/CBoxAlgorithmClassifierTrainer.cpp`
- The other files marked 2.23 in section 3, which reword their errors. Each says so once, with a `UG fork:` comment above its includes, rather than at each message.

**What it does**
- Every error raised through the `OV_ERROR` macros, about 750 places, has one new format, set in `ovAssert.h`: an apology (until 2.53 always "We do most humbly apologise:"; since 2.54 one of several, see there), the message, then the details for developers in brackets. Upstream's format was `{Error description} : {...}, {Error type} : {...}, {Error location} : {...}`.
- The Channel Selector's three messages about its settings say which setting is wrong, what it asks for, how the box looked for it, and which channels the signal has. Upstream says `Invalid channel [25]`.
- About 155 further errors about settings and files, in 60 files, say which setting or file is meant, what its value is, what is expected and what to check. These are the errors of the types `BadSetting`, `BadConfig`, `BadFileRead`, `BadFileWrite` and `BadFileParsing`, plus the CSV boxes' failure to open a file, a missing settings override file, a missing configuration file, and a classifier file that can't be read. Upstream's CSV boxes say `Could not open the file` without naming it.
- The Classifier trainer stops at once on an empty filename. Upstream checks the wrong setting, the multiclass strategy, so it trains first and fails on saving.
- The messages that follow from a box's error say so, and the last says that the first error names the cause. Upstream's line after a failed `process()` named the wrong function, `processInput`.
- Designer's log panel wraps a long message. Upstream cuts it off at the panel's edge, where the advice is.

**Consequences**
- Errors of other types keep their short wording in the new format, such as `I am so very sorry: Invalid input type [...]`. There are about 600 of these.
- About 1,500 messages are logged directly at error level, without the macros, and are unchanged. Among them is `The file [] could not be opened for writing...` from the CSP Spatial Filter Trainer.
- A tool that reads `{Error description}` from a log needs adapting. Nothing in this repository does.
- Measured by running Designer with the box's tutorial scenario: an unknown position, an unknown name, an impossible range and an empty selection each give the new message, and a valid list runs without errors. The log panel was checked with a channel that doesn't exist.
- Of the further errors, 18 kinds were triggered in Designer with the repository's scenarios and bad settings, among them filter frequencies, decimation, file paths and tokens. The others were built but not triggered. The skeleton generator's six are not built here at all.

### 2.24 Samples keep their times when a block comes in pieces

**Files**
- `extras/applications/platform/acquisition-server/abstractions/driver/BlockStamps.h` (new), `abstractions/CMakeLists.txt`
- `CDriverTMSi.cpp`, `CDriverTMSi.h`, `CDriverEEGO.cpp`, `CDriverEEGO.h`
- `extras/applications/platform/acquisition-server/test/BlockStampsTest.cpp` (new)

**What it does**
- A sample is stamped from when its block came from the amplifier's SDK, and its place in that block: the block's last sample at that moment, the others one sample period earlier for each sample after them.
- Before, the TMSi and eego drivers counted back from the moment a read finished. A read takes at most one chunk, at least 20 ms. So when the SDK had more waiting, the block was handed over in several reads, and all of it but the very last sample came out too late. The samples of the later reads got times 1 ns apart. The error grew to the size of the backlog, which can be seconds after the server falls behind.
- A trigger takes its sample's time, so it had the same error.

**Consequences**
- No change when a block fits in one read, which is the usual case.
- Inside OpenViBE a trigger already landed on its own sample, so this shows in LSL Output and recordings with LSL: against other streams, and after an XDF loader evens out the signal's times.
- Tested with `BlockStampsTest`: a block handed over in pieces of any size gets the same times as one handed over whole. Not run with an amplifier, and not tested with a backlog on purpose.
- Delays in fetching a block still shift its times. That is jitter, not this bug, and is not changed.

### 2.25 A trigger value caught while the lines switch is no trigger

**Files**
- `extras/applications/platform/acquisition-server/abstractions/driver/SteadyTrigger.h` (new), `abstractions/CMakeLists.txt`
- `CDriverTMSi.cpp`, `CDriverTMSi.h`, `CDriverEEGO.cpp`, `CDriverEEGO.h`
- `extras/applications/platform/acquisition-server/test/SteadyTriggerTest.cpp` (new)

**What it does**
- The lines of a trigger port never switch at quite the same moment. Now and then the amplifier takes a sample in between, and reads a value that was never sent. In the lab, with a TMSi, pulse 110 came in as 108 for one sample (the line for 2 still off) and then 110. That gave an extra trigger 108, half a millisecond before the real one: 4 times in about 4,950 pulses, and once in 535.
- Now a trigger value counts only once it has held for two samples in a row. A value that lasts one sample is passed over, both on the way up and on the way down.
- A trigger is dated at the first sample where the channel changed, so 0, 108, 110, 110 gives one trigger 110, at the sample of the 108. That is the earliest sample where the pulse shows.
- The drivers keep their own meaning of a trigger: TMSi sends the new value, eego the lines that came on. Before, eego made two triggers of the example: 108, and then 2.
- The drivers decide in `getData()`, where the stamps are known. A change that starts in one read and settles in the next is dated at its sample in the first read, and the Acquisition client still places it there (2.20).

**Consequences**
- No extra triggers with wrong codes. Codes sent back to back, without returning to 0, are still each a trigger, as long as each lasts two samples.
- A pulse shorter than two samples is lost: under 1 ms at 2048 Hz, under 8 ms at 256 Hz. Pulses of 10 ms are safe at any rate the drivers offer.
- A trigger leaves the server one sample later than before, with the same date.
- Tested with `SteadyTriggerTest`. Not yet run with an amplifier.

### 2.26 Signal display: scaling by page and by type, antialiased lines, channel selection buttons, a panel that hides, repairs

**Files**
- `extras/plugins/processing/simple-visualization/src/SignalScaling.h` (new), `CSignalTraceItem.cpp`, `CSignalTraceItem.h`, `CSignalDisplayController.cpp`, `CSignalDisplayController.h`, `CTraceBuffer.cpp`, `CTraceBuffer.h`, `SDisplaySettings.h`, `defines.hpp`, `main.cpp`, `box-algorithms/CSignalDisplay.cpp`, `box-algorithms/CSignalDisplay.h`, `CMakeLists.txt`
- `extras/plugins/processing/simple-visualization/qml/SignalDisplay.qml`, `qml/AmplitudeScaleBar.qml`, `qml/TimeRuler.qml`, `doc/Doc_BoxAlgorithm_SignalDisplay.dox-part`
- `extras/applications/qml/OVParameters/src/parameterObject.h`, `parameterObject.cpp`, `extras/applications/qml/OVParameters/MultiSelectParameter.qml`
- `extras/plugins/processing/simple-visualization/test/SignalScalingTest.cpp` (new), `test/TraceBufferTest.cpp` (new), `test/CMakeLists.txt`
- 59 demo, tutorial and test scenarios (`*.xml`) with a Signal display

**What it does**
- **Centring.** In every automatic mode each channel is centred on the middle of its own signal. Upstream's Global mode gave all channels one centre, the middle of all of them together. A channel with a DC offset, as an amplifier without a high-pass filter gives, was then drawn outside its lane, and the shared range spanned the offsets, so the ruler showed numbers like ±1.2e+5. Global now shares the scale only.
- **By type**, a new scaling mode: the EEG channels share one scale, the EOG channels one and the digital channels one. Other channels (ECG, EMG, GSR, unnamed BIP and AUX inputs) keep a scale of their own. The kind comes from the channel name: names with EOG, and HEO and VEO, are EOG; Trigger, SampleCounter, Digi, DIG, Saw, Status and Marker are digital; ECG, EMG, GSR, Resp, BIP, AUX and the like are other; everything else is EEG.
- **Once per page.** An automatic scale is worked out again at every new page, from the page just drawn: in Scan mode when the cursor starts again at the left, in Scroll mode once per Time Scale. It then fits that page at once. While the first page fills, twice a second. Upstream refreshed it on a timer, every 5 s by default, and moved it only part of the way, with hysteresis. The setting "Scale refresh interval (secs)" is gone from the box and the panel.
- **Round numbers.** A scale is rounded up to 1, 2 or 5 times a power of ten, after 5 % headroom, and the ruler shows it without trailing zeros: ±50, ±0.5, ±200.
- **Greyed out.** Vertical Scale is used only when the scaling is None (and Vertical Offset was, until 2.39), and Channels per page is not used in Multiview. The panel now shows them greyed out and they can not be changed then. For this, a parameter object has an `enabled` property, true unless set otherwise.
- **Antialiasing.** The traces are drawn into a layer with 4× multisampling, so the lines are smooth. A layer is a texture, so it is left out when the scrolling content is taller than 8192 device pixels; the lines are then drawn as before.
- **Channel selection.** The Channels list has three buttons above it: Select all and Deselect all side by side, Invert selection under them, white with a blue outline. Each sets the whole selection at once. The channels are listed in two columns, row by row (Fp1 Fp2, F3 F4, …), with check boxes in the same style: white with a blue outline, and a blue check. They are part of the shared multi-select control (`MultiSelectParameter.qml`), so any other multi-select list gets them too.
- **Hiding the panel.** A gear button next to the settings panel hides it, so the traces get the whole width, and shows it again. The panel keeps its width meanwhile. It is shown when playback starts.
- **Label width.** The column with the channel labels was 96 px wide whatever the labels, so short names such as `Cz` stood far from the amplitude ruler. It is now as wide as the longest label, name or unit, up to twice that; longer labels are cut short.

Repairs, found by reading the display's code through:
- **Times after a long chunk.** A chunk longer than the displayed window keeps only its last samples, but upstream counted only those. Every later sample was then dated too early by the samples left out, and the time ruler and the markers with it. It happens when the Time Scale is shorter than a chunk, such as 0.1 s at 256 Hz, which is 26 samples, with chunks of 32. Now all samples count, and the kept ones go to the slots they would have reached.
- **First chunks dropped.** The ring took the stream's shape at the next screen update, up to 16 ms after the header, and dropped the chunks that came before. It now takes it at once, on the player thread. That also stops the player thread from reading the Time Scale, which the panel changes.
- **Undefined behaviour.** For a Signal input the box cast its decoder to `TStreamedMatrixDecoder`, a class `TSignalDecoder` does not derive from. It worked because the two happen to be laid out alike. The box now casts to the decoder it made.
- **Colours.** With Colour per channel and a channel hidden, the traces took their colour by row and the Multiview legend by channel, so they disagreed. Both go by channel now, and a channel keeps its colour when others are hidden.
- **Time ruler.** It made a new list of ticks at every screen update, and so rebuilt all of them 60 times a second. It now counts the ticks and moves them.
- **One source of truth.** The names of the modes and scalings were spelled in `main.cpp` and again in the panel; both now take them from `SDisplaySettings`. The amplitude ruler took the share of a lane a trace may fill from a copy of the value; it now reads it from the trace.
- **Channels with the same name** were shown and hidden together, as the selection goes by name. A name that is taken now gets the channel's number: `EOG`, `EOG (12)`.
- **Units before the header.** Channel units that came before the signal's header were dropped; another box feeds that input, so it can happen. They are now kept until the channels are known.
- **Multiview's amplitude bracket** showed the first channel's scale, also when the others had another one (Per channel, By type). It now shows a number only when all channels share the scale, and "varies" otherwise.
- **Unknown setting values.** A Display Mode or Auto vertical scale the box does not know, from a hand-edited scenario or one made with a newer build, was used as it was: it scaled as Per channel while the panel showed None. The box now stops with an error that names the setting.
- **Infinite samples.** A channel with an infinite sample gave its whole group the fallback scale of ±1. Such a channel now keeps its previous scale, and its group is scaled without it.
- **Antialiasing on wide screens.** The size limit of the antialiasing layer checked only the height; it now checks the width too.
- Removed unused code (`CTraceBuffer::clear`, `channelAt`, `visibleCount`); a change of scaling no longer works the scale out twice.

**Scenarios.** Designer asks to update a box whose stored prototype hash (attribute `0x4e7b798a, 0x183beafb`) differs from that of the box as built. All 118 Signal display boxes in the repository's 59 scenarios had a hash that matched no version of the box, so Designer flagged them already upstream. Their "Scale refresh interval (secs)" setting is now removed and their hash set to that of the current box, `(0x749ffce0, 0x5335cebd)`. That hash was computed with the kernel's own `SBoxProto`. Nothing else in them differed from the current box.

**Consequences**
- Scenarios outside the repository that still have "Scale refresh interval (secs)" play as before: settings are read by name, so it is ignored. Designer marks such a box as one to update; updating it in Designer removes the setting.
- "By type" is in the box setting and the panel. A box with "None", "Global" or "Per channel" keeps its value.
- A channel whose name says nothing (`Channel 3`) counts as EEG. Under By type, a recording that names its EOG channel `BIP1` scales it on its own, not with the EOG.
- A new page can change the scale in a jump, once per page. That is the intent: within a page the scale stays put.
- Unchanged from upstream: the display keeps at most 512 markers. With more in the window, such as 90 pulses a second over 10 s, the oldest ones, on the left, are not drawn.
- In the Designer settings dialog, settings that do not apply are not greyed out; only the panel next to the display does that.
- Tested with `test-signal-display` (14 tests: the scaling, and the ring after long chunks). The whole test suite passes, and the Simple-visualization scenario plays without QML errors. Not yet looked at on screen.

### 2.27 BrainVision recordings from eego and TMSi: the integrity of the files

**Files**
- `extras/contrib/plugins/server-drivers/eemagine-eego/src/CDriverEEGO.cpp`, `CDriverEEGO.h`
- `extras/applications/platform/acquisition-server/builtin/drivers/tmsi/CDriverTMSi.cpp`, `CDriverTMSi.h`
- `extras/contrib/plugins/processing/file-io/src/box-algorithms/BrainVisionUG.h` (new), `ovpCBoxAlgorithmBrainampFileWriterUG.cpp`, `ovpCBoxAlgorithmBrainampFileWriterUG.h`
- `extras/contrib/plugins/processing/file-io/scripts/check_brainvision.py` (new)
- `extras/contrib/plugins/processing/file-io/test/BrainVisionUGTest.cpp` (new), `test/CMakeLists.txt`, `test/scenarios-tests/BrainVision-UG-Write-test.xml` and `-input.csv` (new)

The path checked: eego amplifier, a Charmeleon montage with part of the EEG channels, hardware triggers, the acquisition server, the Acquisition client box, and the BrainVision Format file writer (UG). Every step was read for what reaches the files. These were wrong:

- **Volt in a microvolt file.** The eego SDK gives volt, and the driver passed them on, as upstream's did. The writer declares no unit, which BrainVision means as microvolt, and the TMSi driver does deliver microvolt. So an eego file read a million times too small. The driver now sends the EEG and bipolar channels in microvolt. Trigger and SampleCounter are sent as they are.
- **Names on the wrong data.** The driver names the channels in the order it asks the SDK for them, then `Trigger` and `SampleCounter`, and checked only that the stream had as many channels. The SDK does not promise the order. Start now checks the kind of every channel of the stream against the names: the selected referential and bipolar channels in order, then the trigger, then the sample counter. If it differs, nothing is recorded, and the server says why under the settings. When the SDK numbers the stream's channels differently from the amplifier, only the kinds can be checked, and the log says so.
- **Markers off their samples.** The writer took a marker's position from its date counted from the scenario's start. The Acquisition client starts its signal when the server's configuration comes in, so the data file starts later than that, and every marker lay too late by that time. And it rounded down: a chunk's start is a sum of durations rounded down to OpenViBE's fixed point, which at 500 Hz puts a sample's date just short of its time, and the marker one sample early. A marker's position is now counted from the first sample in the file, and rounded to the nearest sample. A marker that comes before any signal waits for it; one dated before the first sample is left out, with a warning.
- **Files that end short.** The writer kept the `.vhdr` in memory until the end, so a crash left it empty. It now writes all three files out at every step. A file that can not be written, e.g. on a full disk, stops the recording with an error; before, the files silently ended short of what they claim.
- **16-bit formats.** INT_16 and UINT_16 truncated the values, and a value out of range, such as an amplifier's DC offset, wrapped around. They are now rounded and held at the format's limits, with a warning. IEEE_FLOAT_32, the default, is not affected.
- **Smaller.** A comma in a channel name is coded as `\1`, as BrainVision has it. The files say `Codepage=UTF-8`, which is what is written; they said ANSI. A second signal header stops the recording, where it was appended to the `.vhdr`.

The TMSi path, checked the same way afterwards:

- **Unit exponent.** The SDK gives each channel's values in its unit times 10 to the channel's exponent. The driver ignored the exponent, as the 3.x driver it was ported from did. EXG channels come in 1e-6 V, so EEG was in microvolt and is unchanged. A BIP or AUX channel with another exponent was recorded a thousand or a million times off. Every channel in volt now goes out in microvolt, and the log names each channel that needed scaling. An analog channel in another unit is recorded as the device gives it, and the log says so. The volt unit is taken to be the SDK's unit number 1; if that were wrong, nothing would be scaled, as before, and the log would call every analog channel "not in volt".
- **Out of range.** A sample out of the amplifier's range, e.g. from an electrode come loose, is recorded as 0, as before, which the recording does not show. The log now counts them.
- **Part of a sample.** A block from the SDK that is not a whole number of samples lost its partial sample silently; the log now says so.
- **Right already:** the names and the data come from one list of the device's channels, so a name can not land on another channel's data; the montage subset and the triggers are as for eego.

These were checked and found right:
- **Montage subset.** Only EEG channels the montage names are recorded, under the montage's names. Their positions go to `[Coordinates]` by name, and other channels get `0,0,0` (2.3, 2.7, 2.9).
- **Triggers.** Dated at their own sample (2.24), without values caught while the lines switch (2.25), and placed by the client on the last sample stamped at or before them (2.5, 2.20). eego's code is the lines that came on, so a pulse must return to 0 between codes, or a code that only switches lines off gives no marker.
- **Samples.** The server sends whole chunks only, so the client never pads one with zeros. No sample is repeated or dropped between the server and the file. Samples the amplifier lost are counted (2.4), but the file has no gap for them; the SampleCounter channel shows where they were.

**Checking a recording.** `check_brainvision.py` checks a recording itself:

```bash
pixi run python extras/contrib/plugins/processing/file-io/scripts/check_brainvision.py recording.vhdr
```

It checks the header, that the data is a whole number of samples without NaN, and that EEG is in microvolt. It checks that every stimulus marker sits on the sample where the recorded trigger channel switches on its code, and that every switch has its marker. It also checks that the sample counter counts every sample. The trigger channel is eego's `Trigger` or the name given to TMSi's digital channel, and the sample counter eego's `SampleCounter` or TMSi's sawtooth channel, when that is recorded; `--trigger` and `--counter` name them otherwise. It reads TMSi and eego codes alike, and ends with exit code 1 when something is wrong.

**Consequences**
- eego recordings, and everything else downstream of the eego driver (Signal display, filters, LSL Output), are now in microvolt. Scenarios or scripts that compensated for volt need changing.
- Marker positions in UG BrainVision files are exact, also when the Acquisition client started late, and at every sampling rate.
- Tested with `test-brainvision-ug` (6 tests: positions, names, 16-bit values) and `BrainVision-UG-Write`. That test sends a signal with a 25 mV DC offset, a Trigger channel and a SampleCounter, and 10 hardware triggers, through the writer, and `check_brainvision.py` then checks the file. The checker itself was tried on damaged copies: a marker one sample late, a data file cut short, and data in volt. It found each. The whole test suite passes, except `run_TagStream` while an installed acquisition server holds its port.
- With TMSi, record the sawtooth channel (give it a name, e.g. `Saw`) to have the check find lost samples.
- **Not verified with an amplifier:** the eego microvolt and the stream order check, and the TMSi scaling of BIP and AUX channels. The stream order check refuses to record if the SDK lists the channels in another order than asked. That would show at the first Play, with the reason under the settings.
- The SampleCounter is a 32-bit float in the server, so beyond 16.7 million samples (2.3 h at 2048 Hz) it no longer counts exactly. The checker then reports jumps as warnings, not errors.

---

### 2.28 LSL display (new box)

**Files**
- `extras/plugins/processing/simple-visualization/src/box-algorithms/CLSLDisplay.cpp`, `CLSLDisplay.h` (new): the box
- `extras/plugins/processing/simple-visualization/src/CLSLDisplayController.cpp`, `.h`, `CLSLReader.cpp`, `.h`, `TracePalette.h` (new)
- `extras/plugins/processing/simple-visualization/qml/LSLDisplay.qml`, `qml/SettingsPanel.qml` (new), `qml/SignalDisplay.qml`
- `CTraceBuffer.cpp`, `CTraceBuffer.h`, `CSignalTraceItem.cpp`, `CSignalTraceItem.h`, `CSignalDisplayController.cpp`, `box-algorithms/CSignalDisplay.cpp`, `box-algorithms/CSignalDisplay.h`, `defines.hpp`, `main.cpp`, `CMakeLists.txt` (links LSL)
- `extras/plugins/processing/simple-visualization/box-tutorials/lsl-display.xml` (new)
- `extras/plugins/processing/simple-visualization/test/LSLReaderTest.cpp` (new), `test/TraceBufferTest.cpp`, `test/CMakeLists.txt`

**What it does**

"LSL display (UG)", under *Visualization/Basic*, shows LabStreamingLayer streams from the network; the LSL XDF writer (2.29) connected to it records them. The acquisition server's LSL driver can not do this: a driver carries one stream, at one rate.

- **No acquisition server and no inputs.** The box finds the streams itself. Put it in a scenario on its own (`box-tutorials/lsl-display.xml`) and press Play.
- **Streams are chosen while it runs.** The panel lists the streams on the network under *Streams*, brought up to date every second, as "name (type, 32 ch, 2048 Hz)" or "name (type, text markers)". Tick a stream to show it, untick it to close it. A stream that goes away stays listed while it is ticked, and comes back by itself when it does.
- **Data streams**, any regular rate and any number type, are stacked in panels, each with its name above it. A panel's height goes by the channels it shows, so lanes are alike across streams. The panels share one time axis and the time ruler at the bottom.
- **Marker streams**, numbers or text, are drawn as flags on every panel, at their LSL times. A stream of text, or one without a regular rate, counts as markers. Of a marker stream with several channels, the first is the code, as in the two-channel sync sender.
- **The options are the Signal display's:** Display Mode, Auto vertical scale (Per channel, By type, Global, None), Time Scale, Vertical Scale, Multiview, Colour per channel, Trace shading (2.40), and the rulers. They apply to every stream. Each data stream adds its own channel selection, *Channels: name*, with Select all, Deselect all and Invert selection. The box settings are where the panel starts.

How streams of different rates line up:
- Each sample is placed by its LSL time, corrected to this computer's clock and smoothed, as LabRecorder's files are when loaded. Every stream's ring is locked to one clock (`CTraceBuffer::setTimeOrigin()`). In Scan mode a time is at the same x in every panel, whatever the stream's rate or start. In Scroll mode each stream is drawn as far behind the newest as it lags, so that the panels scroll together.
- A stream's samples do not come exactly at its nominal rate. When the count of samples and their LSL times drift two samples apart, the ring follows the times: a gap is drawn with the last values, and samples that come early overwrite the last ones. So the times stay right for hours.
- Every ring starts at the grid point of its first sample on the shared clock, and every chunk is measured on that grid. So the time of a slot is the same in every stream, and each sample lies less than two samples, plus half a sample of grid, from its date. A test runs two streams for an hour: 500 Hz running 500 ppm fast in chunks of 20, its first date 15 ms off; and 128 Hz running 0.1 % slow in chunks of 4. At the end, each still draws a time on the sample of that time.

Why the streams' cursors differ, and move against each other:
- A stream's cursor is where its newest sample is, which is its date of that sample, not the time now. Each panel's header shows how far that is behind now, e.g. "42 ms behind". The distance between two cursors is the difference between their delays.
- **The delay depends on how the sender sends.** A stream pushed in chunks of 32 samples at 512 Hz arrives up to 62 ms after its first sample's date; one pushed sample by sample arrives within milliseconds. Chunks of different streams come at their own moments, so their cursors jump against each other.
- **The dates settle over the first minutes.** LSL corrects each sample's date to this computer's clock and smooths it (clock sync and dejitter), as `load_xdf` does when it loads a LabRecorder file. Online, the clock offset is measured again every few seconds, and the smoothing learns the stream's real rate over about a minute and a half. The dates, and with them the cursors, shift a little until it has. `load_xdf` fits the whole recording at once, so its times can differ from the display's by that much in the first minute.
- None of this moves the data: every sample is drawn at its own date. A marker sent at the same moment as a sample in two streams stands on that sample in both, wherever the cursors are.

For this, the Signal display's parts became shared, each in one place:
- `SettingsPanel.qml` is the panel with its gear button, used by both displays.
- `TracePalette.h` holds the trace colours.
- `addDisplaySettings()` and `readDisplaySettings()`, with `DISPLAY_SETTING_NAMES`, hold the box settings. The Signal display's settings are unchanged, so its prototype and the scenarios' hashes are too.

**Consequences**
- Seeing LSL streams no longer needs the acquisition server or a scenario per stream.
- The streams ticked are not saved with the scenario: they are ticked again after Play. Their uids change with every start of the sending program anyway.
- There is no Channels per page: the panels share the window's height.
- At most 2048 markers are kept.
- Tested with `test-lsl-reader` (3 tests), which sends its own LSL streams on this computer: a data stream comes in with its labels, units and times; number and text markers with their times; a stream unticked is closed. Also tested with `test-signal-display` (19 tests, among them the time-locked rings: rates lining up, gaps, early samples, jitter, and the hour of two drifting streams). The box plays in Designer without errors, and the whole test suite passes. **Not yet looked at on screen**, with real streams.

---

### 2.29 LSL XDF writer (new box)

**Files**
- `extras/plugins/processing/simple-visualization/src/box-algorithms/CLSLXDFWriter.cpp`, `CLSLXDFWriter.h` (new): the box
- `extras/plugins/processing/simple-visualization/src/CLSLRecorder.cpp`, `.h`, `XDFWriter.cpp`, `.h` (new)
- `box-algorithms/CLSLDisplay.cpp`, `CLSLDisplay.h`, `CLSLDisplayController.cpp`, `CLSLDisplayController.h`, `defines.hpp`, `main.cpp`, `CMakeLists.txt`
- `extras/plugins/processing/simple-visualization/box-tutorials/lsl-display-xdf.xml` (new), `box-tutorials/lsl-display.xml`
- `extras/plugins/processing/simple-visualization/test/LSLRecorderTest.cpp` (new), `test/CMakeLists.txt`

**What it does**

"LSL XDF writer (UG)", under *File reading and writing/XDF*, records LSL streams into an XDF file, as LabRecorder does. Connect it to the LSL display's new output, *LSL streams* (`box-tutorials/lsl-display-xdf.xml`).

- **The display's Record button** starts a recording of the streams ticked, to a new file, and stops it again ("Stop recording 1:23"). The button sits at the top right of the plot, so it stays at hand when the settings panel is hidden; it can not be pressed while nothing is ticked.
- **Only the channels shown are recorded.** Of each data stream, the file holds the channels ticked under its *Channels: name*. Its header in the file gives their number and only their descriptions, so that `load_xdf` and pyxdf load them as a stream of that many channels. It notes which channels of the stream they are, under `desc/openvibe_ug/recorded_channels`, counted from 1 (e.g. `1,3`), with the stream's own channel count. A data stream with no channel ticked is left out; a marker stream is recorded whole, and so is a stream that is ticked but not shown yet when Record is pressed.
- **While recording, the Streams list and the channel selections are locked** (greyed out), so that a file holds the same channels from start to end. The options can still be changed: they only change what is drawn.
- **One file per recording.** The first gets the name set, the next ones in the same Play `-2`, `-3`, ... before the extension. Nothing is written until Record is pressed; Stop in Designer ends a recording too.

- **The file is LabRecorder's format.** A FileHeader; for each stream a StreamHeader with the stream's whole LSL info, channel labels included. Then Samples chunks with every sample and its own time stamp, in the stream's own type (float, double, integers, text); a ClockOffset every 5 s; and a StreamFooter with the first and last time stamp, the sample count and the clock offsets. Boundary chunks every 10 s.
- **The time stamps are the senders' own,** as LabRecorder keeps them: the writer reads the streams through inlets of its own, without LSL's corrections. `load_xdf` and pyxdf correct them from the clock offsets, and smooth them, when they load the file, as they do LabRecorder's files. The display's inlets do apply the corrections, as they draw at once.
- **Written out chunk by chunk,** so a crash loses no more than the last moments. A file that can not be written, e.g. on a full disk, stops the recording with an error.
- **Settings:** Output directory, `${Path_Documents}/data` by default and made if needed; Filename, `lsl-[$core{date}-$core{time}].xdf` by default. A filename with a full path replaces the directory.

How the boxes connect: the display's output is a new stream type, *LSL streams*, registered by the plugin. It is a streamed matrix whose header lists the streams to record, one row each: the stream's LSL uid, with `#` and its channels counted from 0 when not all of them (`uid#0,2`), while Record is on; none (one empty label) otherwise. It is sent again on every change, and the writer opens a file when the list fills and closes it when it empties. Only the writer's input takes it, so Designer connects nothing else to it.

**Consequences**
- Seeing and recording LSL streams together needs one scenario and no other program. LabRecorder remains the tried tool; this writer's files load as LabRecorder's do.
- The display's prototype gained the output, so a scenario with the LSL display from before this has it outdated; Designer offers to update it.
- Tested with `test-lsl-recorder` (3 tests). It records three streams the test sends itself: 3 float channels at 200 Hz with labels, of which only Fp1 and Pz; an int16 stream; and text markers. Then it reads the file back with a parser of its own: every sample and time stamp as sent, the two channels with their descriptions and the note `1,3`, the footers, a clock offset. The same file loaded in pyxdf 1.17 and in MATLAB with `load_xdf` (EEGLAB's xdfimport 1.2): all 400, 100 and 3 samples, the EEG as 2 channels labelled Fp1 and Pz, the markers 0.25 s after the first sample as sent, the footers and clock offsets. Both tutorials load without being out of date, and the writer waits for Record without writing a file. **Not yet tried:** the Record button with streams ticked; that needs the display on screen.

### 2.30 No sample dropped when the server's settings change under a running scenario

**Files**
- `extras/plugins/processing/acquisition/src/box-algorithms/SampleBlocks.h` (new), `BoxAcquisitionClient.cpp`, `BoxAcquisitionClient.h`
- `extras/applications/platform/acquisition-server/core/Acquisition.cpp`
- `extras/plugins/processing/acquisition/test/SampleBlocksTest.cpp` (new), `test/CMakeLists.txt`
- `extras/applications/platform/acquisition-server/test/AcquisitionConfigTest.cpp` (new)

**What went wrong**

The server tells a client its settings (rate, channels, "Sample count per sent block") once, when the client connects. Restarted with other settings, it did not tell the clients still connected, and the Acquisition client box took only the first settings it got. It wrote each block into one chunk of the size it knew, and dropped what did not fit, without a word. With the block size raised from 4 to 64 under a running scenario, the box kept the first 4 samples of every 64: one sample in 16. The Signal display took 15 to 16 s to fill a page of 1 s, and its ruler still read 1 s a page, as it counts the samples it gets. A recording would have held the same sixteenth. Other channels would have been worse: the box read each block with the old channel count, mixing the channels up, and past the end of a smaller block.

**What it does now**
- **The server sends its settings to every connected client** each time it sets up an acquisition, not only to a client that connects.
- **The client never drops a sample.** `SampleBlocks` cuts the server's blocks, of any size, into the chunks of the scenario's signal stream: blocks of 64 into 16 chunks of 4, blocks of 4 gathered into chunks of 32. A stimulation goes with the chunk that holds the last sample of the block it came with, never an earlier one. A changed block size only gets a warning in the log.
- **A change the stream can not take stops the scenario, with an error that says so:** another sampling rate, another channel count, other channel names or another montage. A signal stream keeps its header from start to end, and no box can take one that changes midway. The message gives both settings and says to start the scenario again. A block whose values are not a whole number of samples of the stream's channels stops it too, also from a server without this change, which does not send the new settings.
- Settings that come before the stream has begun replace the earlier ones; samples received under those earlier ones are left out, with a warning.

**Consequences**
- A recording or display can no longer be silently thinned out or scrambled by changing the server's settings during a run. A changed block size goes on without loss; any other change stops the scenario.
- Tested with `test-sample-blocks` (4 tests): blocks of 64 into chunks of 4 keep all 128 samples, in order, with their stamps; blocks of 4 wait to fill a chunk of 32; a stimulation waits for the chunk with its block's last sample; a block of other channels is refused, not misread. `AcquisitionConfigTest` (server core, 67 tests in all): after the acquisition is set up again with blocks of 64, the connected client is sent the new settings; it fails without the change. **Not yet tried:** the whole path, server restarted under a running scenario in Designer.

### 2.31 A second acquisition server refuses a port already in use

**Files**
- `sdk/modules/network/Server/CServer.cpp`, `CServer.h`
- `extras/applications/platform/acquisition-server/main.cpp`

**What went wrong**

The network server binds its port with `SO_REUSEADDR`, which on Windows lets a second program bind a port another one listens on. A second acquisition server, started by mistake, therefore ran next to the first without a word, and Designer connected to either one. Channels renamed in one server did not reach the scenario, which read the other.

**What it does now**
- On Windows the server binds its port with `SO_EXCLUSIVEADDRUSE` instead of `SO_REUSEADDR` (2.41), so `bind()` fails when another program listens on it; the server then does not listen, and says so in the console log. Elsewhere it keeps `SO_REUSEADDR`, which there never lets two programs listen on one port. This holds for the acquisition port (8088) and the TCP tagging port (15361). Until 2.41 the server probed the port with a connection first.
- The acquisition server then shows "Port 8088 is already in use, most likely by another OpenViBE acquisition server that is still running", and quits.
- A socket whose `bind()` or `listen()` failed is closed; upstream kept it open.

**Consequences**
- Only one server per port. Two servers side by side need different ports (Preferences).
- Tried with the probe (before 2.41): a second server started while one ran on 8088 found both ports in use, showed the message and quit. The exclusive bind is checked in 2.41.

---

### 2.32 Designer finds its user configuration file without designer.conf

**Files**
- `designer/applications/platform/designer/src/OVDesigner.cpp` (`loadDesignerConfiguration()`)

**What it does**
- Designer reads `designer.conf` from its data directory, `<install>/share/openvibe/applications/designer/`. That file sets `${Designer_CustomConfigurationFile}`, the user configuration file that holds the user's own tokens and the list of scenarios to reopen. The kernel skips a configuration file that does not exist without saying so. Upstream, a `designer.conf` that was not found therefore left that token unset: the user's file was not loaded, and every save of a scenario, every scenario opened and Designer closing logged `${Designer_CustomConfigurationFile} is empty, the session can not be saved`.
- Now a missing `designer.conf` is an error in the log that names the path that was tried, and asks whether `OV_PATH_DATA` or `OV_PATH_ROOT` points elsewhere.
- When the token is unset after loading, Designer sets it to `${Path_UserData}/openvibe-designer.conf`, the value in `designer.conf-base`, and says so in a warning. The user's file is then loaded and the session saved there, at `%APPDATA%\OpenVIBE-4.0\openvibe-designer.conf`.
- The user's file is loaded only when it exists. On a first run it does not, and the first session save creates it.

**Consequences**
- The scenario file itself was always saved; only the session (the scenarios to reopen) was lost. Both now work.
- The usual cause is a copy of `bin` alone to another computer: Designer looks for its data in `..\share\openvibe` next to `bin`. Copy the whole installation (`pixi run install` writes it to `dist\`).
- The default is written in two places, `designer.conf-base` and `OVDesigner.cpp`. A comment next to the constant says to keep them in step.
- Designer still runs on its built-in defaults without `designer.conf`: its other tokens, such as `Designer_DefaultWorkingDirectory` and the help URLs, are not set. The error says which file to restore.
- Not yet built or run on Windows.

---

### 2.33 Run time above all windows while a scenario runs (new)

**Files**
- `designer/applications/platform/designer/RuntimeOverlay.qml` (new), `Main.qml`, `CMakeLists.txt`
- `designer/applications/platform/designer/src/OVDesigner.h`, `OVDesigner.cpp` (`runtimeOverlayEnabled`)
- `designer/applications/platform/designer/share/designer.conf-base` (`Designer_RuntimeOverlay`)

**What it does**
- While a scenario plays, is paused, steps or fast-forwards, its run time is also shown in a small dark window at the top left corner of the screen Designer is on, above every other window: Designer's visualisation windows and other programs alike. It reads `00:12:34.567`, as the action bar does.
- A dot before the time is black (with a light ring) while time runs, red while a recording is written (2.44), and amber while paused. When the scenario stops, the window goes.
- The window takes no focus, lets every click through to whatever lies under it, has no taskbar button, and stays when Designer is minimised.
- It shows the scenario of the current tab, as the action bar does.
- `Designer_RuntimeOverlay = false`, in `designer.conf` or in the user's `openvibe-designer.conf`, turns it off. It is read when Designer starts.
- While the BrainVision writer (UG) records, a square after the time says so (2.44).
- The window raises itself again every second while it shows. Windows keeps it among the windows that stay on top, but a window that asks for the same later, a full-screen stimulus window for one, would otherwise go above it. Raising takes no focus.

**Consequences**
- With the participant's display on the same screen as Designer, the participant sees the time too. Put Designer on the experimenter's screen, or turn the overlay off.
- A program that takes the screen in exclusive full-screen mode, as some games and stimulus programs on DirectX do, can cover it. Windowed and borderless full-screen programs, and OpenViBE's own windows, can not.
- Checked in a headless Qt 6 with a stand-in for Designer: hidden on Stop, shown on Play and Pause with the right dot, at 16 px from the corner, with the flags above. Not yet run in Designer on Windows.

---

### 2.34 Windows installer built on GitHub (new)

**Files**
- `.github/workflows/windows-installer.yml` (new)

**What it does**
- A GitHub Actions workflow, "Windows installer", builds OpenViBE UG on a Windows runner and packages it with CPack: `openvibe-<version>.exe` (the NSIS installer) and `openvibe-<version>.zip`. It follows the GitLab Windows pipeline: Qt from the official build (6.12.0 since 2.50), everything else from the pixi environment, then `pixi run ci-build` and `pixi run ci-package`.
- **By hand:** Actions tab, "Windows installer", "Run workflow", on any branch. The installer and ZIP are an artifact of the run, kept 30 days.
- **On a tag:** pushing a tag such as `4.0.1` or `4.0.1-ug.1` runs it too, and attaches both files to a GitHub release named after the tag. A tag with a `-` makes a pre-release.
- **Release notes:** the release's description lists the subject of every commit since the previous version tag, with a link to the full comparison. GitHub's generated notes would list merged pull requests, and the fork commits to `main` directly. A release that already has a description, written by hand before the build finishes, keeps it.
- The version in the file names and in Designer comes from `git describe`, as in a local build; the workflow fetches the whole history for that.

**Consequences**
- A build takes in the order of an hour or more: the whole of OpenViBE, then the documentation, then CPack. The workflow stops after 6 hours.
- The installer contains what `cmake --install` puts in `dist\`: the programs, the pixi environment's DLLs, Qt, the Python runtime and the Visual C++ runtime. Amplifier software such as Charmeleon is not in it.
- The tag must read `MAJOR.MINOR.PATCH`, optionally with `-something`. Other tags do not start it, and CMake would refuse them anyway.
- Qt is installed with the modules qtcharts, qtgraphs, qtquick3d, qtshadertools and qt5compat. Designer's Icon control imports `Qt5Compat.GraphicalEffects`: the 4.0.1-ug.2 installer lacked qt5compat, and its Designer ran without a window.

---

### 2.35 Box pictures in the documentation

**Files**
- `extras/applications/developer-tools/plugin-inspector/src/CBoxQmlRenderer.cpp`

**What it does**
- The documentation build (`doc` target, also run before packaging) draws a picture of every box with Designer's `Box.qml`. `Box.qml` has two required properties for metaboxes, `isMetabox` and `metaboxId`, which the renderer did not set. Qt then refuses to create the box, so every picture was skipped, with two `Required property ... was not initialized` errors per box in the log.
- The renderer now sets them: `isMetabox` false and `metaboxId` empty, as the pictures are of box algorithms.

**Consequences**
- The box pages of the documentation have their pictures again, and the packaging log loses some 500 error lines.
- Found in the first run of the installer workflow (2.34). Not yet checked in a new run.

---

### 2.36 Adding a comment in Designer

**Files**
- `designer/applications/platform/designer/MenuModel.qml`, `ShortcutManager.qml`, `Main.qml`
- `designer/applications/platform/designer/OVVisualization/PipelineCanvas.qml` (`addComment()`, `_placementPoint()`)
- `designer/applications/platform/designer/src/DesignerScenario.h`

**What it does**
- The legacy Designer put yellow comment boxes on the canvas. The new one shows them, and they can be edited (double-click), moved (drag), deleted (click, then Delete) and copied with boxes. But a new one could not be made: the canvas had an unfinished mode in which a click opened the comment dialog, switched on only by the signal `newComment`, which nothing sent. `DesignerScenario::addComment()`, which stores the comment, was in place.
- Now **Edit > Add comment**, or **Ctrl+Shift+C**, opens the comment dialog at once. OK puts the comment, centred, under the pointer when it is over the canvas, else in the middle of the view, as one undoable step. Cancel adds nothing. The comment can be dragged where it belongs afterwards.
- The dialog starts empty each time. It is reused, and used to offer the previous comment's text.
- Removed: the click mode (`_mouseAreaComment`), the signal `newComment` and the `Connections` that waited for it. A first version of this change used the click mode, and choosing the menu entry seemed to do nothing: the only sign of the mode was the pointer's shape, and its own check for leaving the canvas ignored pan and zoom, so it could switch itself off before the click.
- Not held back while a text field has the focus, unlike Cut, Copy and Paste: in that first version the box search field in the toolbox, when it kept the focus, also made the menu entry do nothing.

**Consequences**
- The text may hold simple HTML, as before: `<b>bold</b>`, `<i>italic</i>`, `<font color="red">…</font>`. Line breaks are kept as typed.
- Tested headless with the real `CommentEditDialog.qml` and the canvas's own functions: the dialog opens, empty, and OK returns the text and the place. `qmllint` finds no new problems. Not yet run in Designer.

---

### 2.37 The acquisition server's own icon

**Files**
- `share/ov-acquisition-server-icon.ico` (new), drawn by `share/tools/make_acquisition_server_icon.py` (new)
- `CMakeLists.txt` (`OV_ACQUISITION_SERVER_ICON_FILE`), `extras/applications/platform/acquisition-server/CMakeLists.txt`

**What it does**
- The acquisition server had Designer's icon, so the two were hard to tell apart in the taskbar and the Start Menu. It now has its own, in the same style: the white tile with rounded corners, the pink to blue gradient, the same stroke width and round nodes, and the faint glow. Designer's shows one trace with a spike between two nodes; the server's shows three channels coming in on the left, the middle one with Designer's spike, merging into one node on the right.
- It is in the executable (taskbar, Explorer, the installer's Start Menu shortcut) and is the window icon. Sizes 16, 24, 32, 48, 64, 128 and 256 px.
- The icon is drawn by a script rather than by hand, so it can be changed and redrawn: `pip install pillow`, then `python share/tools/make_acquisition_server_icon.py`. Its measurements are those of Designer's icon, at the top of the script.
- The build now rebuilds an executable's resources when its icon changes (`OBJECT_DEPENDS` on the generated `.rc`), for the server and for Designer. The resource compiler reads the icon, but Ninja only tracks `#include`s, so a changed icon left the old one in the executable until a clean build.
- The file is laid out as Designer's: classic 32-bit bitmaps up to 128 px and PNG at 256 px. The first version stored every size as PNG, Pillow's default, which the resource compiler, the Windows shell and Qt's icon reader do not all take for the small sizes.

**Consequences**
- The installer itself, and Designer, keep `ov-icon.ico`.
- In the resources the icon keeps the alias `ov-icon.ico`, so `main.cpp` is unchanged.
- Looked at side by side with Designer's icon at 16, 32, 48 and 256 px. Not yet built into the server.

---

### 2.38 The comment editor opens on the screen

**Files**
- `designer/applications/platform/designer/OVVisualization/CommentEditDialog.qml` (`openOver()`), `Comment.qml`, `PipelineCanvas.qml`

**What it does**
- Double-clicking a comment opens its editor, a window of its own. Designer gave that window the comment's position on the canvas, but a window's position is on the screen. A comment to the right of or below the first screenful of the scenario, or any comment once the view was panned, put the editor off the screen, out of reach.
- The editor now opens in the middle of Designer's window and takes the keyboard, both for editing a comment and for adding one (2.36).

**Consequences**
- The editor no longer appears next to its comment; it always appears where Designer is.
- Tested headless: with Designer's window at (100, 80), 800 by 600, the editor of a comment at (5000, 3000) on the canvas opens at (300, 180), inside it. Not yet run in Designer.

### 2.39 No Vertical Offset: every channel centred on its own signal

**Files**
- `extras/plugins/processing/simple-visualization/src/CSignalTraceItem.cpp`, `CSignalTraceItem.h`, `CSignalDisplayController.cpp`, `CSignalDisplayController.h`, `CLSLDisplayController.cpp`, `CLSLDisplayController.h`, `SDisplaySettings.h`, `box-algorithms/CSignalDisplay.cpp`, `box-algorithms/CSignalDisplay.h`
- `extras/plugins/processing/simple-visualization/doc/Doc_BoxAlgorithm_SignalDisplay.dox-part`
- The 59 scenarios of 2.26 and the two LSL display tutorials (`box-tutorials/lsl-display.xml`, `lsl-display-xdf.xml`)

**What it does**
- With Auto vertical scale None, the Signal display and the LSL display drew every channel around one fixed value, the Vertical Offset. A channel with a DC offset, as an amplifier without a high-pass filter gives, then left its lane unless the offset was set for it, and one value can not suit channels with different offsets.
- Now every channel is centred on its own signal in every mode, None included: the middle of a lane is the middle of what the channel spans on the page, worked out again for every page as an automatic scale is (2.26). None only keeps the scale fixed: Vertical Scale, the same for every channel.
- The setting "Vertical Offset" is gone from both boxes and from their panels. It is one list for both (`addDisplaySettings()`), so the two boxes stay alike.

**Scenarios.** All 120 boxes in the repository (118 Signal displays in 59 scenarios, 2 LSL displays) had the offset at 0. The setting is removed from them, and their prototype hash set to that of the current box: `(0x8cb5ea19, 0x160b59cb)` for the Signal display, `(0x9ceddf45, 0x8a942865)` for the LSL display. The hashes were computed as the kernel's `SBoxProto` does; the same computation gives the stored hashes of the boxes as they were.

**Consequences**
- With None, a trace no longer stays at one height from page to page: it is centred again at every new page, so a slow drift shows as a jump at the page boundary, not as a trace that wanders out of its lane.
- Scenarios outside the repository that still have "Vertical Offset" play as before, without it: settings are read by name, so it is ignored. Designer marks such a box as one to update; updating it removes the setting.
- A value of the offset other than 0 in such a scenario is lost; there is nothing to take its place.
- Tested: the build, the documentation page (seven settings, Multiview the seventh), and the test suite, which passes except `run-command-with-spaces`, a test of the Run Command box that fails now and then without this change too. Designer opens `signal-display.xml`, `lsl-display.xml` and `erp-1-monitor-signal.xml` without asking to update a box; the old `signal-display.xml` it flags twice. Not yet looked at on screen.

### 2.40 Trace shading: dense traces shaded by density, or see-through

**Files**
- `extras/plugins/processing/simple-visualization/src/TraceShading.h` (new), `CSignalTraceItem.cpp`, `SDisplaySettings.h`, `CSignalDisplayController.cpp`, `CSignalDisplayController.h`, `CLSLDisplayController.cpp`, `CLSLDisplayController.h`, `CMakeLists.txt`
- `extras/plugins/processing/simple-visualization/test/TraceShadingTest.cpp` (new), `test/CMakeLists.txt`
- `extras/plugins/processing/simple-visualization/doc/Doc_BoxAlgorithm_SignalDisplay.dox-part`

**What it does**
- With more samples than pixel columns, as with a long Time Scale, a trace is drawn as the lowest and highest sample of each column, so that no spike is lost. Upstream drew that opaque: a noisy signal became a solid band with hard edges, and the antialiasing (2.26) can not soften it, as a one-pixel line on a column's centre has no partly covered edge.
- A new entry in the panel of the Signal display and the LSL display, **Trace shading**, chooses how traces are drawn:
  - **Density shading** (the default): each pixel column of a dense trace is split into sub-columns, one per whole sample in it and at most four, as many as the samples of the antialiasing layer. They are drawn see-through, so that a pixel all of them pass is 95 % opaque and one only some of them pass is lighter: the trace is darker where it passes more often, as on an analogue oscilloscope, and a rare spike shows, lighter. A trace with fewer samples than pixels is drawn opaque, as before.
  - **Transparent**: every trace 60 % opaque, whatever its density, so that overlapping traces, as in Multiview, show through each other.
- The number of sub-columns goes by the spacing of the samples, not by the length of the piece of trace drawn, which in Scan mode changes with every chunk; otherwise the shading would flicker.
- Like Colour per channel, the entry is in the panel only, not a box setting: it is not saved with the scenario, and the boxes' prototypes are unchanged.

**Consequences**
- With Density shading, a dense trace draws up to four times the points it did: still a number that goes by the screen's width, not by the number of samples.
- The shading of a trace changes in steps as the Time Scale grows: from one sample per pixel to four, the sub-columns go from one to four.
- Tested with `test-signal-display` (6 new tests: the sub-columns, the opacity, the names) and the Simple-visualization scenario. Not yet looked at on screen.

### 2.41 Fixes from a review of the fork: no Charmeleon in the driver interface, one panel, explicit positions

A review of all of the fork's code, on 18 November 2026 ([`review-2026-11-18.md`](review-2026-11-18.md)), found duplicated logic, a Linux and macOS build that no longer compiled, hidden state between boxes and two workarounds where the cause could be fixed. This section fixes those.

**Files**
- `extras/applications/platform/acquisition-server/abstractions/driver/CharmeleonDriver.h` and `.cpp` (new), `AbstractDriver.h` and `.cpp`, `CharmeleonMontage.h` and `.cpp`, `abstractions/CMakeLists.txt`
- `extras/applications/platform/acquisition-server/builtin/drivers/tmsi/CDriverTMSi.h` and `.cpp`, `extras/contrib/plugins/server-drivers/eemagine-eego/src/CDriverEEGO.h` and `.cpp`
- `extras/applications/platform/acquisition-server/core/Acquisition.h` and `.cpp`, `ui/AcquisitionController.cpp`, `ui/Charmeleon.h` and `.cpp`
- `sdk/openvibe/include/openvibe/acquisition-messaging/ovChannelInfo.h`, `extras/plugins/processing/acquisition/src/box-algorithms/BoxAcquisitionClient.h` and `.cpp`, `extras/contrib/plugins/processing/file-io/src/box-algorithms/ovpCBoxAlgorithmBrainampFileWriterUG.h` and `.cpp`, `extras/applications/platform/acquisition-server/test/OvMsgSerializerTest.cpp`
- `sdk/modules/network/Server/CServer.cpp`, `sdk/modules/network/AbstractionLayer/NAL.h`
- `sdk/toolkit/include/toolkit/box-algorithms/ovtkTBoxAlgorithm.h`, six file boxes in `sdk/plugins/processing/file-io/src/box-algorithms/` (their messages)
- `extras/plugins/processing/simple-visualization/src/DisplayPanel.h` and `.cpp` (new), `qml/TracePanel.qml` (new), `CSignalDisplayController.*`, `CLSLDisplayController.*`, `CSignalTraceItem.*`, `SDisplaySettings.h`, `SignalScaling.h`, `qml/SignalDisplay.qml`, `qml/LSLDisplay.qml`, `CMakeLists.txt`, `test/SignalScalingTest.cpp`
- The 31 scenarios with an Acquisition client box, and `BrainVision-UG-Write-test.xml`

**What it does**
- **Charmeleon out of the driver interface; one copy of the montage code.** `AbstractDriver` had five Charmeleon functions, and `Acquisition` seven more that passed them on; the TMSi and eego drivers each had their own copy of the montage handling (applying it, the channel list, the hints, recording only named channels, the error note), about 90 lines each. All of it is now in `CharmeleonDriver`, a driver base class in the abstractions library. TMSi and eego derive from it and supply only their channels, whether they are acquiring, the arguments for Charmeleon and, for TMSi, the layout it assumes without a device. `AbstractDriver` has one generic function instead, `channelInfo()`: the names and, for a driver that knows them, the montage and positions sent to the clients. The server's own second reading of the montage file (`describeChannels()`) is gone with it. The acquisition server finds a Charmeleon driver with `dynamic_cast`. One `trimWhitespace()` replaces three copies.
- **The acquisition server compiles on Linux and macOS again.** `AcquisitionController.cpp` included `<Windows.h>` and `QWinEventNotifier` without a guard, for the watcher that notices Charmeleon closing. The watcher is now `Charmeleon::Watcher`, in `Charmeleon.cpp`, behind a platform test; it never signals where Charmeleon is not started.
- **The montage reaches the writer as a stream.** It went from the Acquisition client to the BrainVision writer (UG) as two configuration tokens of the run: state shared by every box of the player, invisible in the scenario, overwritten by a second client, and read by a writer fed from a file. The client now has a *Channel localisation* output, OpenViBE's own stream type for electrode positions, and the writer a matching input (version 1.5). The conversion between Charmeleon's angles and OpenViBE's frame (x to the nose, y to the left ear, z up, as `share/openvibe/electrode_sets/electrode_set_standard_cartesian.txt`) is in `ovChannelInfo.h`, tested both ways. The header's montage line now gives the number of positions instead of the montage's file name, which the stream does not carry.
- **Exclusive port binding.** The server set `SO_REUSEADDR`, which on Windows lets two servers share a port, and so probed the port with a connection before binding (2.31), with a race between two servers starting together and up to 0.3 s more at start-up. On Windows it now sets `SO_EXCLUSIVEADDRUSE`, the standard way, and `bind()` itself refuses a taken port (`NETWORK_ERROR_ADDRESS_IN_USE`). The probe is gone.
- **Backslashes in paths.** A backslash in a file or folder setting was lost in token expansion, where it is an escape character; the file boxes' errors explained this ("Paths need forward slashes..."). On Windows `FSettingValueAutoCast`, through which boxes read their settings, now reads a backslash in a Filename or Foldername setting as a forward slash before the expansion. The explanation is gone from the six messages.
- **One display panel, one trace block, one refresh.** The Signal display and the LSL display built the same nine panel entries by hand, with help texts that had begun to differ; their QML repeated the same trace block (rulers, names, the antialiasing layer, cursor, markers); and both had the "work the scale out once per page" logic, each with its own constant. Now:
  - `CDisplayPanel` builds the shared entries, bound to the display's `SDisplaySettings`, greys out Vertical Scale by one rule, and tells the display what a change affects (`EDisplayChange`). *Colour per channel* became a display setting, like *Trace shading*.
  - `TracePanel.qml` is the trace with what goes around it, used by both views.
  - `CSignalTraceItem::refresh()` is the screen update both displays share: the scale for a new page, by the rule in `CPageRescale` (`SignalScaling.h`, tested), the layout, the redraw.

**Consequences**
- **Recording scenarios need one more link** for electrode positions: Acquisition client, output 3, to BrainVision writer (UG), input 3. Without it the recording has no positions (montages.md, 7.1). The repository's scenarios have no client and writer together, so none needed it.
- The 31 scenarios with an Acquisition client box carry the new output and the box's current prototype hash. Their stored hash was that of upstream's full prototype, with all five outputs that upstream has since commented out, so Designer had flagged them as out of date. The writer's test scenario has the new input.
- A plugin driver built against the fork's `AbstractDriver` needs rebuilding: the Charmeleon functions moved to `CharmeleonDriver`.
- On Windows a backslash in a file or folder setting can no longer escape a `$` there; nobody is known to have done so.
- Tested: the build, the whole test suite, Designer opening the changed scenarios without asking to update a box. **Not yet checked on hardware:** the Charmeleon round trip with the base class, and a second server refused by the exclusive bind. The montage did not reach a BrainVision file through the new link: fixed in 2.45.

### 2.42 Smaller fixes from the review

The smaller findings of the review of 2.41, each fixed where it was found.

**Files**
- `extras/applications/platform/acquisition-server/CMakeLists.txt`, `test/CMakeLists.txt`, `core/Acquisition.cpp`, `sdk/kernel/CMakeLists.txt`, `sdk/kernel/src/kernel/ovkCKernelContext.cpp`
- `extras/applications/platform/acquisition-server/abstractions/writer/AbstractWriter.h` and `.cpp`, `abstractions/driver/AbstractDriver.h`, `builtin/drivers/tmsi/CDriverTMSi.cpp`, `extras/contrib/plugins/server-drivers/eemagine-eego/src/CDriverEEGO.cpp`, `sdk/openvibe/include/openvibe/acquisition-messaging/ovMsgSerializer.h`
- `extras/applications/platform/acquisition-server/ui/AcquisitionController.h` and `.cpp`, `ui/qml/BufferControl.qml`, `ui/qml/DriverNote.qml`, `ui/qml/LevelBar.qml`, `extras/applications/qml/OVDesignSystem/Theme.qml`
- `extras/applications/platform/acquisition-server/abstractions/driver/CharmeleonMontage.h` and `.cpp`, `ui/Charmeleon.cpp`
- `designer/applications/platform/designer/src/DesignerScenario.cpp`, `extras/plugins/processing/simple-visualization/src/CLSLReader.cpp`, `sdk/modules/network/Server/CServer.cpp`

**What it does**
- **Libraries are linked in CMake.** `Avrt` (the multimedia scheduler) and `Shell32`/`Ole32` (`${Path_Documents}`) were pulled in with `#pragma comment(lib, ...)` in the source; the build files now say so.
- **The writer counts its waiting stimulations itself.** `AbstractWriter` used the message's own stimulation count to hold those still waiting for their sample (2.21), and so had to swap it for the count sent around each send. They now have a counter of their own. `init()` also frees the arrays of the previous run; it runs at every Play, and upstream allocated them anew each time without freeing the old ones.
- **One place backs off after an empty read:** `Acquisition::read()`. The TMSi and eego drivers slept a millisecond themselves as well; `AbstractDriver::read()` now says a driver need not.
- **Typed values for the server's QML.** The buffer levels (2.4) were a `QVariantMap` of ten keys named in a comment; they are a value type now, `BufferLevels`, with a property per value, so QML reads them as before but by declared name. The driver note's level (2.16) is an enum, `AcquisitionController.NoteLevel`, not the strings "info", "warning" and "error". The theme has status colours (`statusColors`: ok, warning, error), which the note, the level bars and the lost-sample count use, in place of the CPU meter's colours.
- **Charmeleon.** Closing it at Play waits on the processes themselves, so the wait ends as soon as they have gone, rather than in steps of 200 ms. Its command line quotes each argument by Windows's rules, so that a path ending in a backslash or holding a quote reaches it whole. `pathToUtf8()` and `isRegularFile()` are in `CharmeleonMontage.h` once, not copied into `Charmeleon.cpp`. The kept copy of a montage (2.3) is written next to it and renamed over it, so that it is never half written.
- **Designer:** every way out of a second click on a port ends link creation through one guard, instead of a call on each of eight paths (2.12).
- **eego:** the amplifier type is tested with a substring search, not a regular expression.
- **LSL display:** a text marker's colour comes from a hash that is the same in every run and on every computer; a numeric marker that is not a number no longer goes through an undefined conversion.
- **Network server** (upstream): an IPv6 address was written into a 16-byte `struct sockaddr`, too small for it (MSVC warning C4789); it now has a `sockaddr_storage`.
- Files new to the fork no longer carry `UG fork:` tags line by line, except those that mark what differs from the code they were ported from (see the start of this file).

**Left as they are**
- Designer's player loop wakes every millisecond (2.18). Waiting exactly until the next scheduler step needs the kernel's time to reach, which `IPlayer` does not expose; guessing it from outside goes wrong around pause, step and fast-forward, and the wake-ups cost next to nothing.
- `CharmeleonMontage` words its summaries in English inside the driver library: there is one language, and the tests read them.
- Designer started from a terminal prints to its log file, not to the terminal (2.17); the stimulations stay about 100 ms behind the signal (speller.md, 3.3). Both are by design.

**Consequences**
- None in use. A writer plugin built against the fork's `AbstractWriter` needs rebuilding, as its layout changed.
- Tested: the build and the whole test suite. **Not looked at on screen:** the buffer levels and driver note with their new colours.

### 2.43 Colour per channel: the colour before each name

**Files**
- `extras/plugins/processing/simple-visualization/qml/ChannelGutter.qml`, `qml/SignalDisplay.qml`, `qml/LSLDisplay.qml`
- `extras/plugins/processing/simple-visualization/src/CSignalDisplayController.h` and `.cpp`, `src/CLSLDisplayController.h`
- `extras/plugins/processing/simple-visualization/doc/Doc_BoxAlgorithm_SignalDisplay.dox-part`

**What it does**
- With *Colour per channel* on, a short thick line in the colour of a channel's trace is drawn just before its name, in the Signal display and the LSL display. With it off, the names are as before.
- Multiview's legend shows the same mark, in place of upstream's round dot.
- The gutter widens by the mark's width when the option is on, so that the names are not cut shorter. The controllers tell the QML whether the option is on (`perChannelColors`), as they do for Multiview.

**Consequence:** in a stacked display, which trace is which channel can be read off the colours, also when the lanes are narrow or the traces overlap.

### 2.44 "Recording" in the run time overlay

**Files**
- `sdk/openvibe/include/openvibe/RecordingStatus.hpp` (new)
- `extras/contrib/plugins/processing/file-io/src/box-algorithms/ovpCBoxAlgorithmBrainampFileWriterUG.h` and `.cpp`
- `designer/applications/platform/designer/src/PlayerBridge.h`, `OVDesigner.h`, `OVDesigner.cpp`, `RuntimeOverlay.qml`

**What it does**
- While a BrainVision Format file writer (UG) in the running scenario writes samples to its file, the run time overlay (2.33) turns its dot from black to red and shows a square after the time with **REC** in its middle: red, or amber while the scenario is paused, as nothing is written then. The square is as high as it is wide, a little wider than the word "Recording" in small bold type, so it can be seen from across the room. A red dot reads as recording, so it is red only then. Without such a writer, or before its first sample, the overlay shows the time alone, as before.
- The writer says so itself, so the square means that samples reach the file, not only that a writer is in the scenario. A writer whose signal input has no link, or that gets no signal because the acquisition server does not stream, never says it records.
- How: from its first sample until it stops, the writer counts itself in a token of its run's configuration, `Player_Recording` (`RecordingStatus.hpp`). The player makes that configuration anew for every run, so the count never outlives the run. Designer reads it for the scenario of the current tab, as often as the run time. Another writer can take part by calling `RecordingStatus::set()` in the same way.

**Consequences**
- The overlay shows the scenario of the current tab (2.33). A recording in a scenario in another tab is not shown.
- With the overlay turned off (`Designer_RuntimeOverlay = false`) there is no square either.
- The other writers (CSV, GDF, the Gipsa writer, the LSL XDF writer) do not report it.

### 2.45 Electrode positions reach the BrainVision header

**Files**
- `extras/contrib/plugins/processing/file-io/src/box-algorithms/ovpCBoxAlgorithmBrainampFileWriterUG.h` and `.cpp`
- `extras/contrib/plugins/processing/file-io/test/scenarios-tests/BrainVision-UG-Write-test.xml`, `test/CMakeLists.txt`, `scripts/check_brainvision.py`

**The fault:** with the Acquisition client linked to the writer's third input (2.41), the positions never reached the `.vhdr`. The writer warned that the localisation came after the signal had started. The client sends the positions with the signal's header, in the same round, but the kernel hands a box its inputs one at a time and runs its `process()` after each, so the writer wrote the header from the signal on its first input before the third input was handed over. 2.41 had assumed the opposite, and its scenario test had nothing on that input.

**What it does**
- When the positions come after the `.vhdr` is written, the writer writes the `.vhdr` again, with a `[Coordinates]` section. It is a small text file that describes the whole recording, so nothing else changes. Positions from any other box, at any time, are taken in the same way.
- The writer's scenario test now feeds the third input from the *Electrode localisation file reader*, with OpenViBE's standard electrode set, which sends the positions in the same round as the signal's header, as the client does. `check_brainvision.py --expect-positions 2` requires positions for Fp1 and Cz. Without the fix the test fails ("No [Coordinates] section").

**Consequence:** recordings made with the montage link get their electrode positions. Recordings made since 4.0.1-ug.4 with the link have none; their `.vhdr` can not be repaired from the recording itself.

### 2.46 After a run, the scenario can be edited again

**Files:** `designer/applications/platform/designer/src/PlayerBridge.h`, `PlayerBridge.cpp`, `OVDesigner.cpp`, `DesignerScenario.cpp`

**The fault:** Designer refuses to change a scenario while it has a player, that is while it runs; it then ignored Delete, Cut, Paste and Undo without a word. Two of upstream's faults could leave a player behind once a run was over:
- A run that ended on its own (a Player Controller box, a box that stopped with an error, the acquisition server going away) left its thread finished but not joined. The next Play then started no thread: nothing ran, and the player it had made was never released.
- When a run ended, Designer released the player of the tab shown at that moment, not that of the scenario that ran. A run that ended while another tab was shown kept its player.

Selecting a box and pressing Delete then closed the box's settings, as the selection was dropped, but left the box in place.

**What it does**
- A finished player thread is joined before the next run starts its own.
- When a run ends, the player of the scenario that ran is released.
- Delete while a scenario runs says so in the log: "The scenario is running, so nothing is deleted; stop it first."

**Consequences**
- After a run that stopped by itself, Play runs the scenario again, and boxes can be deleted, cut and pasted, and changes undone, as after a run stopped with Stop.
- Not reproduced on screen: the cause is read from the code and matches what was seen, but the report did not say whether the scenario had run before.

### 2.47 The version in the window titles and the BrainVision header

**Files**
- `designer/applications/platform/designer/Main.qml`, `src/OVDesigner.h`, `src/OVDesigner.cpp` (`version`)
- `extras/applications/platform/acquisition-server/ui/qml/Main.qml`, `ui/AcquisitionController.h`, `ui/AcquisitionController.cpp` (`version`)
- `extras/contrib/plugins/processing/file-io/src/box-algorithms/ovpCBoxAlgorithmBrainampFileWriterUG.cpp`, `scripts/check_brainvision.py`, `test/CMakeLists.txt`

**What it does**
- The title bars read `OpenViBE Designer 4.0.1-ug.6` and `OpenViBE Acquisition Server 4.0.1-ug.6`. Upstream showed the name alone, and spelt the server's `Openvibe`.
- The `.vhdr` written by the BrainVision Format file writer (UG) names it too, in its second line, a comment (`; Data created by OpenViBE 4.0.1-ug.6, BrainVision Format file writer (UG)`), and in the free text of `[Comment]`, which BrainVision Analyzer shows (`Recorded with OpenViBE 4.0.1-ug.6`). The second line said the Vision Recorder had made the file, as the Gipsa writer it was ported from did. Readers skip both places, so the file reads as before.
- The version is the kernel's `${Application_Version}`, which the build takes from the git tag (`CMake/OvVersion.cmake`): the same version that the log and saved scenarios carry. A build between two tags shows how far it is from the last, e.g. `4.0.1-ug.6-2-gabc1234`.

**Consequences**
- The version is read when CMake configures. `pixi run build` configures every time; a bare `cmake --build build` keeps the version of the last configure.
- Without a kernel the title is the name alone.
- `check_brainvision.py` reports the version it finds; `--expect-version` makes another one an error. The writer's test requires the build's own version.
- Not tried in BrainVision Analyzer, EEGLAB or MNE.

### 2.48 Boxes older than their plugin are updated when a scenario opens

**Files**
- `designer/applications/platform/designer/src/DesignerScenario.h`, `DesignerScenario.cpp` (`updateOutdatedBoxes()`), `OVDesigner.cpp`
- `sdk/kernel/src/kernel/scenario/ovkCBoxUpdater.h`, `ovkCBoxUpdater.cpp`, `ovkCScenario.cpp`
- `sdk/toolkit/src/algorithms/scenario-io/ovtkCAlgorithmScenarioImporter.cpp`

**The fault:** a scenario keeps, for every box, a stamp of the plugin version it was made with. When the plugin has changed since (a setting or an input added or removed, as this fork did to the Signal display, the Acquisition client and the BrainVision writer (UG)), opening the scenario warned "Box ... should be updated" for each such box. The Qt Designer offered no way to do so but to delete the box and add it again, losing its settings and links. The kernel can update a box itself, keeping both, but refused every box whose inputs, outputs or settings the user can add or change ("its prototype is too complex"): the Signal display, Identity, the multiplexers and many more.

**What it does**
- When Designer opens a scenario, it updates each box that is older than its plugin, through the kernel's own update: settings keep their values, links stay where they were, a new setting gets its default, and a setting the plugin no longer has is dropped if it held its default. A setting the plugin dropped that held another value stays, marked deprecated, and the kernel warns.
- The log says which boxes were updated: "Updated to the current version of their plugin: Signal display. Save the scenario to keep this; Undo takes it back." The update is one Undo step, and the scenario counts as changed, so that saving keeps it and the next opening is quiet.
- Boxes whose inputs, outputs or settings the user can add or change are updated too. Where the user can add them, the box keeps its own, in its own order, with their names, types and values; where the user can change their type, the box keeps the type the user chose. The rest of the box (its version stamp, supported types, flags) is brought up to date.
- Only a box whose author asks for it (`BoxFlag_ManualUpdate`) is left alone; the log then says so ("Not updated: ... To update one, delete it and add it again.").
- The importer's warning per box is now a trace, as Designer says what it did.

**Consequences**
- Opening a scenario made with an older version changes it in memory. Closing without saving leaves the file as it was; the update happens again at the next opening.
- A box where the user can add inputs does not get an input that a newer plugin added to its prototype; add it by hand if needed.
- Tested on a copy of a lab scenario: its Signal display was updated, with its settings and links unchanged and two settings the plugin no longer has (at their defaults) dropped; opened again after saving, nothing was reported. Also on a demo scenario whose Identity box (3 inputs and 3 outputs added by the user) was marked outdated: its inputs, outputs and all 16 links of the scenario stayed the same.

### 2.49 Two crashes found in the lab's crash dumps

**Files**
- `designer/applications/platform/designer/src/BoxObject.cpp`, `DesignerScenario.cpp`
- `extras/applications/platform/acquisition-server/ui/AcquisitionController.cpp`, `extras/plugins/processing/evaluation/src/box-algorithms/CROCCurveView.cpp`
- `designer/plugins/visualization/simple-visualization/src/box-algorithms/topographicMap2DDisplay/CTopographicMap2DView.hpp`, `.cpp`, `CTopographicMapDatabase.hpp`, `.cpp`
- `designer/plugins/visualization/simple-visualization/src/algorithms/spline.cpp`, `CAlgorithmSphericalSplineInterpolation.hpp`, `.cpp`

Designer quit without a word three times on the lab PC. The crash dumps Windows kept, read with the `.pdb` files of the same commit, showed two faults, both upstream's.

**1. Objects that QML deleted while Designer still used them.** Designer hands a box's parameters, its settings file path, and the scenario's parameters to QML with `QJSEngine::newQObject()`. For an object without a parent, that gives QML ownership: its garbage collector deletes the object once no QML item refers to it, for example after the box's settings close, while Designer keeps the pointer. The next read went to freed memory. In the dump of 16:31, QML read `settingsFilePath` (`BoxObject.cpp`), and the call jumped into the heap.
- These objects are now marked as Designer's own (`QJSEngine::CppOwnership`) before they are handed over, as the Signal display and the LSL display already did. The same is done in the acquisition server's parameter lists and the ROC curve's, which had the same fault.

**2. The 2D topographic map read its sample points while the window resized them.** The map interpolates on the player's thread, over sample points that its view computes on the interface thread, and draws into an image that QML reads. When the window is resized (or the view or projection changed), the view frees and rebuilds those points and the image, with nothing to stop the player's thread reading them at that moment. In the dump of 16:51, the interpolation computed a table index from a coordinate that was not a number, `int(NaN)`, and read 16 GB before the table.
- A lock now guards the sample points, their values and the image: the view takes it while it resizes them, the box while it interpolates and draws, and QML gets a copy of the image taken under it.
- The spline routines keep their table index within the table, so a bad coordinate gives a bad frame, not a crash.

**Also, in the same map:** the lab scenario sent the map 65 channels, PO8 twice (a Signal merger of the channel selections `1:64` and `64`). Two electrodes at the same position leave the spline without a solution, so the map stayed empty and the log had "Spline coefficients computation failed!" on every frame, 2,376 times. The map now says once which channels share a position ("Channels 64 (PO8) and 65 (PO8) have the same electrode position, so the map can not be drawn. Leave one of them out of the signal the map gets."), and the failure is logged once a run. The scenario goes on, so a recording in it is not interrupted.

**Consequences**
- Tested: the dumps' addresses resolve, with this commit's `.pdb` files, to `BoxObject.cpp:732` (16:31) and `spline.cpp:279` via `CTopographicMapDatabase.cpp:138` (16:51). The topography test scenario, with a channel given twice, plays without fault, with one warning naming the two channels and one about the spline. With the map playing, Designer's windows were resized 687 times in 40 s without a crash; the test input is short, so that shows little about the race itself. Not yet run on the lab PC.
- The crashes of October (Qt fatal errors, an `ntdll` heap fault, the eego driver dividing by zero in the acquisition server) were in older builds, two of them in another folder; not examined.

### 2.50 Qt 6.12 on Windows

**Files**
- `pixi.toml`, `README.md`, `CMakeLists.txt`
- `.github/workflows/windows-installer.yml`, `.github/install-qt.py` (new)

Upstream builds with Qt 6.9.3. Qt 6.9 is out of support: 6.9.3 was its last release, so no bug or security fix will come for it. Qt 6.12 is the current long-term-support version, supported until 2031-09-30; open-source users get its fix releases until Qt 6.13 is out. Qt Charts, which Designer's displays and the ROC curve still use, is deprecated since Qt 6.10 in favour of Qt Graphs, and 6.12 has the Graphs features a move would need.

- `Qt6_DIR` in `pixi.toml`, the example in its comment, and the README name Qt 6.12.0. No code had to change.
- The installer workflow installs Qt 6.12.0, but no longer with install-qt-action: aqtinstall 3.3.0, the newest release and the one that action runs, does not read the layout Qt's online repository has from 6.11 on (one folder per compiler), and stops with "Failed to download checksum for the file 'Updates.xml'". A fix is merged in aqtinstall, not yet released.
- `.github/install-qt.py` does what the Maintenance Tool does for the packages needed: it reads `Updates.xml` of `qt6_6120_msvc2022_64` on download.qt.io, downloads every archive the Qt package and the five add-ons name (download.qt.io sends it to a mirror), checks each against the SHA-256 that download.qt.io serves itself, unpacks it where the package says, and writes `bin/qt.conf`. The workflow caches the result, as before.
- The GoogleTest suites now list their tests when `ctest` runs, not right after they are linked (`CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST` in `CMakeLists.txt`). Right after linking, Qt's DLLs are not yet next to the test programs: Designer's `windeployqt` copies them there later. In a new build folder the build then stopped at `test-parameter-path`: "DLL not found" (`0xc0000135`) in the workflow, where install-qt-action had put Qt's `bin` on `PATH` and the new step does not, and an entry point missing (`0xc0000139`) on a PC with another Qt on `PATH`, such as Qt Creator's. The tests themselves are unchanged: in a new build folder the build goes through, and `ctest` finds the same 452 tests, of which 451 pass, as before.

**Consequences**
- A build folder configured with Qt 6.9.3 keeps it, because CMake keeps `Qt6_DIR` in its cache. After installing Qt 6.12.0, delete `build` (or configure once with `cmake --fresh`). On a PC that points CMake at Qt with `CMAKE_PREFIX_PATH`, change that variable too.
- Tested with Qt 6.12.0 from the Maintenance Tool, in a separate build folder: it builds without a Qt deprecation warning, and 451 of 452 tests pass, as with 6.9.3 (`uoTimeTest`, a timing test, fails with both). Designer opened empty and played the 24 tutorial scenarios of the two visualisation plugins, 15 s each, and the acquisition server opened: no QML warning or error with either Qt version.
- `install-qt.py` installed the same Qt as the Maintenance Tool, in 52 s: the same 13,693 files, byte for byte, except `bin/qt.conf`, where the Maintenance Tool also names its documentation and examples folders. The Maintenance Tool's install also has add-ons it adds by itself (Qt Quick Timeline, and on this PC Qt MCP), which OpenViBE does not use.
- Not tested: box settings and other dialogs opened by hand, where Qt 6.12 is stricter (a QML component whose required property is not set is now an error, not an incomplete object); the lab PC; the workflow itself, until it runs.
- Linux (Qt 6.8) and macOS (Qt 6.9) take Qt from conda-forge, unchanged.

### 2.51 The MATLAB box only where MATLAB was found

**Files**
- `extras/plugins/processing/matlab/CMakeLists.txt`
- `extras/cmake-modules/FindThirdPartyMatlab.cmake`

The MATLAB Scripting box is the only box of `openvibe-plugins-matlab.dll`, and it is compiled in only when CMake finds MATLAB (`TARGET_HAS_ThirdPartyMatlab`). The DLL was built and installed either way. Without MATLAB, the GitHub build among them, it held no box, and Designer's plugin manager logged at every start: "No 'plugin object descriptor' found from [...openvibe-plugins-matlab.dll] even if it looked like a plugin module".

- Without MATLAB the DLL is no longer built (`EXCLUDE_FROM_ALL`) nor installed. The warning is gone, and nothing else changes: the box was not there before either.
- The MATLAB files the box runs (`share/openvibe/plugins/matlab`, with `OV_stimulations.m`) and the MATLAB tutorial scenarios are still installed. `OV_stimulations.m` is generated by a target that now always builds (`ALL`), not only as a step of the plugin. An installed OpenViBE therefore needs only the DLL to get the box.
- On Windows, CMake looked for MATLAB on `PATH` and in `C:\Program Files\MATLAB\r2015b`. It now looks in every `C:\Program Files\MATLAB\R*`, newest first, and then on `PATH`.

**Getting the box on a PC with MATLAB**
- Build on a PC with 64-bit MATLAB installed (the headers and `libeng.lib`, `libmx.lib`, `libmex.lib` come from MATLAB). The configure step prints "Found Matlab in [...]".
- Copy `bin\openvibe-plugins-matlab.dll` from that build into the `bin` folder of the installed OpenViBE (`C:\Program Files\openvibe-4.0\bin`). The build must be of the same version as the install, since the DLL uses OpenViBE's own DLLs; the MATLAB release does not have to match, since the DLL loads `libeng.dll` and `libmx.dll` only when the box starts.
- In the box, set "Matlab executable (path)" to MATLAB's `bin\win64` folder, e.g. `C:/Program Files/MATLAB/R2026a/bin/win64`. The box adds it to `PATH` and opens the MATLAB engine. MATLAB has to be registered as a COM server, which the MATLAB installer does (`matlab -regserver` as administrator otherwise).

**Consequences**
- Tested on a PC with MATLAB R2026a: found, the DLL built, installed and loaded by Designer ("Added 1 plugin object descriptor"). With MATLAB hidden from CMake: "FAILED to find Matlab (optional)", no DLL built or installed, `OV_stimulations.m` installed.
- The box itself did not run on that PC: MATLAB's engine did not open (`engOpen` returned nothing at once), also in a 10-line program built with MATLAB's own `mex -client engine`, while MATLAB itself and its COM server (`Matlab.Application`) started. This is the PC's MATLAB set-up, not OpenViBE; not solved. Not tested on another PC.

---

### 2.52 "for Windows (64-bit)" instead of "for Win32"

**Files**
- `CMake/OvConfigureRC.cmake`

Every program and DLL that gets a version resource was described as "*name* for Win32", e.g. "OpenViBE Designer for Win32". Windows shows this description in Task Manager and in the file's Properties > Details, where it reads as a 32-bit program. The build is 64-bit (x64, checked in the files themselves); Win32 is the name of the Windows programming interface, which 64-bit programs use too. The description now names the bitness the build has: "*name* for Windows (64-bit)" (32-bit only in a 32-bit build).

**Consequences**
- Tested: after a build, the 21 version resources read "for Windows (64-bit)", and Windows shows "OpenViBE Designer for Windows (64-bit)" and "OpenViBE Acquisition Server for Windows (64-bit)". Nothing else in the programs changes.

---

### 2.53 Ports can be added to boxes that accept any stream type

**Files**
- `designer/applications/platform/designer/src/BoxObject.cpp`

A box whose inputs or outputs the user may add, but which names no stream types for them, accepts every stream type: that is the kernel's rule (`hasInterfacorTypeSupport`), and Designer applies it to the scenario's own inputs and outputs. For boxes, Designer listed only the types a box names. For such a box the list was empty, so "+" in the box's settings asked for a port of type "" (the first of an empty list), which is no stream type; the kernel refused and Designer logged "Could not add input port to box ...". The type list on a port's card was empty as well. About 20 boxes are of this kind, among them MATLAB Scripting, Identity, Signal Merger, Generic Stream Writer, Classifier Trainer, Voting Classifier, Feature Aggregator, Stimulation Listener, Lua Stimulator, Matrix Validity Checker, External Processing and Null.

- Such a box now lists every stream type, as the scenario does, with Signal first: a new port is a Signal port, and its card offers the other types. Boxes that name their types are unchanged.
- A box whose listener sets the type of a new port (Identity, for example, matches it to its other ports) still does so; it now gets a port to work on.

**Consequences**
- Tested in Designer (with 2.56): "+" under Input in the MATLAB Scripting box's settings adds "New Input", a Signal input, and the box shows its port. Before, it logged "Could not add input port".

---

### 2.54 The apology varies, and knows the errors before it

**Files**
- `sdk/openvibe/include/openvibe/ErrorApology.hpp`, `sdk/openvibe/src/ErrorApology.cpp` (new)
- `sdk/openvibe/include/openvibe/ovAssert.h`
- `sdk/unit-test/ov-base/ErrorApologyTest.hpp` (new), `sdk/unit-test/ov-base/main.cpp`

Every error raised through the `OV_ERROR` macros began with the same apology (2.23): "We do most humbly apologise:". It is now picked at random each time, from phrasings that depend on the errors before it in the same program:

| The error | Apology, one of |
|---|---|
| The first of its type, not right after another | "I am so very sorry:", "My sincere apologies:", "I am terribly sorry:", "Please forgive me:", "I regret to report:", "Oh dear, I am sorry:" |
| Within a second of the previous error, which is often its cause (a box fails, then the player) | "And, sorry again:", "I am afraid it does not end there:", "Following from that, I apologise:", "One more thing, I am sorry to say:" |
| Of a type that occurred before, not right after another | "Sorry again, this happened before:", "I am afraid this is the 3rd time:", "This again, I do apologise:", "Once more, and I am sorry for it (the 3rd time):" |

- Two errors in a row never get the same apology.
- "Type" is the error's type, as named for the developers at the end of the message (`BadSetting`, `BadFileRead`, ...). The counts and the time of the last error are kept in `openvibe.dll`, so they are shared by every plugin of a program and start anew with each program.
- Fatal errors keep "We are mortified, and must stop at once:".

**Consequences**
- An error no longer starts with a fixed text, so a search of a log for errors should look for `[ ERROR ]` (the log level), or for "(for the developers:", which every such message still has.
- A plugin built for an earlier version of the fork must be built again: the macros now call `errorApology()` in `openvibe.dll`.
- Tested: unit tests of the phrasings, the ordinals ("3rd", "11th", "22nd") and the choice of context.

---

### 2.55 The MATLAB box finds MATLAB from any reasonable path

**Files**
- `extras/plugins/processing/matlab/src/box-algorithms/ovpCBoxAlgorithmMatlabScripting.cpp`

The setting "Matlab executable (path)" told the box where MATLAB is. Upstream took the folder of the file it names, if that file existed, or else the value itself as a folder, and refused anything else with "the Matlab folder [...] does not seem to be a folder", quoting the value with a backslash added: also for a path to `matlab.exe` that did not exist, or that had quotes around it, as Explorer's "Copy as path" gives. A path to `bin\matlab.exe` passed, but the engine is in `bin\win64`, and it failed to load. The tutorials' default, a 32-bit MATLAB R2013b, exists on no current PC.

- The setting may name `matlab.exe` in `bin` or `bin\win64`, either folder, or the release folder (`C:\Program Files\MATLAB\R2026a`), with or without quotes. The box looks for MATLAB's engine (`libeng.dll`) in that folder, its `win64` folder, and its `bin\win64` folder.
- Where it finds none, it says why, and uses the newest MATLAB in `C:\Program Files\MATLAB`, with a warning, e.g. "This box's setting "Matlab executable (path)" [C:/Program Files (x86)/MATLAB/R2013b/bin/win32/matlab.exe] is a file that does not exist, so we use the newest MATLAB installed, in [C:\Program Files\MATLAB\R2026a\bin\win64]". The reasons: the value is empty; it is a file, or a folder, that does not exist; it is a file whose folder holds no engine (that folder is named); it is a folder that holds no engine. With no MATLAB installed there either, the same reason ends an error.
- The box loads `libmx.dll` and `libeng.dll` itself, from that folder, so that Windows looks for the DLLs they need there first. Upstream let Windows load them on first use, through `PATH`; in Designer that failed with exception `0xC06D007E` (a DLL not found). A DLL that does not load gives Windows' reason.
- When the engine fails, the log names Windows' exception code, and, when the engine loaded but MATLAB did not start for it, says that MATLAB starts for the engine as a COM server, and how to register it (`matlab.exe -regserver` as administrator; 2.58 names the cause found).

**Consequences**
- Tested in Designer with the third MATLAB tutorial and seven values: `bin\matlab.exe` in quotes, `bin\win64\MATLAB.exe`, the release folder, the old R2013b default, a path that does not exist, `C:\Windows\notepad.exe` and `C:\Windows`. Each gave the folder or the warning described above. The engine now loads; MATLAB then did not start for it on that PC, as in MATLAB's own engine example there (2.51), so a MATLAB script had still not run. It ran after 2.58.

---

### 2.56 Log entries can be selected and copied

**Files**
- `extras/applications/qml/OVControls/Logger.qml`
- `extras/applications/qml/OVControls/OVLogController.h`, `OVLogController.cpp`

The log panel of Designer (and of the acquisition server's QML window) showed entries that could not be selected, so an error could not be copied into a message or a report.

- A click selects an entry; Ctrl+click adds or removes one; Shift+click selects the range from the last clicked entry (with Ctrl, adds it). Selected entries have a light blue background.
- Ctrl+C copies the selected entries, Ctrl+A selects all. The right-click menu has "Copy entry" (or "Copy 3 entries"), "Copy all entries" and "Select all"; a right-click on an entry that is not selected selects it first.
- The text is one line per entry, in the log's order, as the panel shows it: `[ Error ] I am so very sorry: ...`. A message of several lines keeps them.
- Ctrl+C and Ctrl+A act on the log only while it has the keyboard focus, which a click in it gives and a click on the canvas takes back; otherwise Ctrl+C copies boxes, as before.
- Entries that leave the list, by Clear or by the filter, leave the selection.
- `OVLogController.copyToClipboard()` puts the text on the system clipboard (QML has no clipboard of its own).

**Consequences**
- Tested in Designer, by clicks and keys sent from a script and the clipboard read back: a click and a Shift+click two entries further copied those three entries; Ctrl+click took one out of a selection; right-click selected an unselected entry and opened the menu; "Copy entry", "Select all" (46 entries) and "Copy all entries" (46 lines) did what they say. After a click on the canvas, Ctrl+C left the clipboard alone.
- Not tried in the acquisition server's QML window, which uses the same panel.

---

### 2.57 A MATLAB example that squares the EEG channels

**Files**
- `extras/plugins/processing/matlab/box-tutorials/matlab/tuto4-square-eeg.xml`
- `extras/plugins/processing/matlab/box-tutorials/matlab/tuto4_square_eeg_Initialize.m`, `tuto4_square_eeg_Process.m`, `tuto4_square_eeg_Uninitialize.m`

A scenario to test the MATLAB box (2.51, 2.55) with a script whose effect is plain to see. It is installed with the other MATLAB tutorials, in `share/openvibe/scenarios/box-tutorials/matlab`.

- A Sinus oscillator makes four channels (512 Hz, 32 samples a chunk), which Channel Rename names Fz, Cz, EOG and BIP1. One Signal display shows them; the Matlab Scripting box passes them to a second.
- The script squares every sample of the EEG channels, Fz and Cz, and passes EOG and BIP1 on unchanged. It logs once which channels it squares and which it leaves.
- A signal stream carries channel names only, no types, so the script takes a channel's type from its name, by the rule the Signal display uses to scale "By type" (2.26, `SignalScaling.h`, `channelKind()`): trigger, status and counter channels, EOG, and other body signals and amplifier inputs (ECG, EMG, BIP, AUX, ...) are not EEG; every other name is an electrode. MATLAB cannot call that C++ function, so the script has its own copy of the rule, and both say to change them together.
- "Matlab executable (path)" is left empty, so the box uses the newest MATLAB installed, with the warning of 2.55.

**Consequences**
- The script was run in MATLAB R2026a (`matlab -batch`), outside OpenViBE, with the box's own `OV_*` functions and ten channel names: Fz, Cz, A12 and O2 were squared; EOG, BIP1, HEOG, Trigger, AUX2 and Saw were passed on unchanged, and the output kept the input's header.
- Run end to end in Designer, from its Play button, with MATLAB R2026a, after 2.58: the log says "squaring Fz, Cz; passing on unchanged: EOG, BIP1"; the "After" display shows Fz and Cz squared, never below zero, beside the signed sines of "Before", and EOG and BIP1 the same in both. Stop ran the Uninitialize function, and the MATLAB the run had started quit. A second Play in the same Designer started a new MATLAB and ran as well.
- Started from the command line (`designer --play`), neither this scenario nor the Signal display tutorial opens its display windows; MATLAB runs all the same.

---

### 2.58 The MATLAB box works in Designer

**Files**
- `extras/plugins/processing/matlab/src/MatlabEngineThread.h` (new)
- `extras/plugins/processing/matlab/src/ovpCMatlabHelper.h`
- `extras/plugins/processing/matlab/src/box-algorithms/ovpCBoxAlgorithmMatlabScripting.h`, `ovpCBoxAlgorithmMatlabScripting.cpp`

With MATLAB found and started (2.51, 2.55), the box still stopped the scenario at the first call after Play: "An error occurred while updating the box clock", then errors from the Uninitialize function and from closing the engine. MATLAB's Initialize function had run.

- **Cause.** On Windows the MATLAB engine talks to MATLAB over COM, and a COM connection works only from the thread that opened it. Designer initializes and uninitializes the player on its window thread, but plays it on a thread of its own (`PlayerBridge`). The engine, opened in `initialize()`, was then called from `processClock()` on the play thread; the call failed, and the session was lost with it. OpenViBE 3 played on its window thread, so upstream's box never met this.
- **Fix.** The box runs every engine call on one thread, `CEngineThread` in `MatlabEngineThread.h`: a thread with its own COM apartment that pumps its messages, started by the first MATLAB box to initialize and ended by the last to uninitialize. The calling thread waits for the answer, so the box's code runs in the same order as before. Macros route the engine functions (`engEvalString`, `engGetVariable`, `engPutVariable`, `engOutputBuffer`, `engOpen`, `engClose`) there, so the 51 calls in the box and its helper are unchanged. Opening and closing the engine run on that thread as a whole, with their guards against Windows exceptions; the guarded `engOpen` is a function of its own, as a function with `__try` may hold no C++ objects.
- **MATLAB not registered.** `libeng.dll` asks Windows for MATLAB by the COM name `MATLAB.AutoServer`. On the PC it was tested on, only the release's own name (`MATLAB.AutoServer.26.1`) was registered, and the engine could not start MATLAB, as in MathWorks' own engine example there. An older release (R2024b) had been there, and its leftover entries suggest that uninstalling it took the name away. When the name is missing, the log now says so and gives the command that registers MATLAB under it, to run in a PowerShell opened as administrator: `& "C:\Program Files\MATLAB\R2026a\bin\matlab.exe" -regserver`. That command restored the name.

**Consequences**
- Tested with the square-EEG tutorial (2.57): it plays, stops, and plays again in the same Designer; see there. The other MATLAB tutorials were not run again.
- MATLAB wrote warnings about `OV_define.m` into the log at every start; 2.59 removes them.
- A MATLAB that was started for a Designer that was then killed keeps running, and the next run uses it. It is not one OpenViBE can close.

---

### 2.59 No more flintmax warnings from MATLAB, and the type IDs are exact

**Files**
- `extras/plugins/processing/matlab/share/OV_define.m`

At every start the MATLAB box runs `OV_define.m`, which gives MATLAB scripts the OpenViBE stream and setting type IDs as `uint64` globals. It read each 64-bit hexadecimal ID with `uint64(hex2dec('...'))`. `hex2dec` returns a double, which holds integers exactly only up to 2^53 (`flintmax`). MATLAB warned about this once per ID, a screenful in Designer's log at every Play, and the warning was right: the IDs came out rounded, `OV_TypeId_EBMLStream` as `434F65872EFD2C00` instead of `434F65872EFD2B7E`. A script comparing against them would never have matched.

- **Fix.** Each ID is read with `sscanf('...', '%lx')`, which returns the `uint64` exactly and warns about nothing. It works in old MATLAB releases as well, unlike the `0x...u64` literals of R2019b and later.

**Consequences**
- Tested in MATLAB R2026a: the file runs without a warning, and `OV_TypeId_Signal`, `OV_TypeId_Script` (above 2^63) and the stimulation range come out exact. No OpenViBE script used the IDs, so nothing else changes.
- The installed copy is in `share/openvibe/plugins/matlab`; a build or install copies it there.

---

### 2.60 The Left ruler's help says what its range means

**Files**
- `extras/plugins/processing/simple-visualization/src/DisplayPanel.cpp`

The amplitude ruler labels each lane "±R", and the automatic scale (2.26) centres every channel on the middle of what it spanned on the last page. A channel of 0 to 9, such as the squared channels of the MATLAB example (2.57), is labelled ±5 but drawn from −0.5 to 9.5, the same label as a channel of −3 to 3 drawn from −5 to 5. Hovering over "Left ruler" in the display's settings now says so: ±R is R above and R below the lane's middle line, in the signal's own unit, and that middle line is not always 0. The label itself is unchanged.

**Consequences**
- The Signal display and the LSL display share the settings panel, so both show the new help.
- Seen in Designer (signal-display tutorial), once the tooltip wrapped (2.61); before that the window's edge cut it off.

---

### 2.61 Hovering over a lane's amplitude bracket shows its min, max and range

**Files**
- `extras/plugins/processing/simple-visualization/qml/AmplitudeScaleBar.qml`
- `extras/plugins/processing/simple-visualization/src/CSignalTraceItem.cpp`, `CSignalTraceItem.h`, `SignalScaling.h`
- `extras/plugins/processing/simple-visualization/test/SignalScalingTest.cpp`
- `extras/plugins/processing/simple-visualization/doc/Doc_BoxAlgorithm_SignalDisplay.dox-part`
- `extras/applications/qml/OVDesignSystem/qml/ToolTip.qml`

The ±R of the amplitude ruler (2.60) says how much a lane shows, not where, nor what the channel actually does. With the mouse over a lane's bracket, a tooltip now gives both, for the samples on screen, and keeps them current twice a second:

> On screen: min −23660, max −23550, range 108.5
> The lane shows −23630 to −23530

- **Values.** `CSignalTraceItem::rowStats(row)` reads the row's channel from the trace buffer, the same samples the automatic scale reads, and gives min, max and count (from `channelStats()` in `SignalScaling.h`, which leaves out NaN and infinite samples, as the trace does), and the values at the two ends of the bracket, centre ± half-range. Values are shown with at most four significant digits and a real minus sign. A mean was shown at first, and dropped: it said little that min and max do not.
- **Not in Multiview**, where one bracket stands for every lane.
- **The tooltip wraps.** The design system's `ToolTip` was one line high and never wrapped: the help of "Left ruler" (2.60) ran off the window. It now wraps at 480 pixels, or the window's width if less, and grows to the lines it needs; a one-line tooltip looks as before. Its text was centred with `anchors.centerIn`, which overrode the left and right anchors and so kept it from wrapping; it is centred vertically only now.

**Consequences**
- Seen in Designer with the signal-display tutorial: the stats follow the lane under the mouse, and a lane whose samples on screen run past its bracket (the scale fits the previous page) shows that plainly, as min or max outside "the lane shows".
- Two new unit tests of `channelStats()`: a ring buffer's two runs, and NaN and infinity left out (30 tests pass).
- The tooltip change applies to every design-system tooltip, in Designer, the Acquisition Server and the plugin inspector. Only the two above were looked at.

---

### 2.62 The lane tooltip names the dominant frequency, when there is one

**Files**
- `extras/plugins/processing/simple-visualization/src/SpectrumPeak.h` (new), `test/SpectrumPeakTest.cpp` (new)
- `extras/plugins/processing/simple-visualization/src/CSignalTraceItem.cpp`, `CSignalTraceItem.h`, `qml/AmplitudeScaleBar.qml`
- `extras/plugins/processing/simple-visualization/CMakeLists.txt`, `test/CMakeLists.txt`, `doc/Doc_BoxAlgorithm_SignalDisplay.dox-part`

The tooltip of a lane's amplitude bracket (2.61) gains a third line when the channel's last 4 seconds have a dominant frequency, and only then: "Dominant frequency, last 4 s: 11 Hz". The display's ring buffer holds the time window, so the last 4 seconds are there whenever Time Scale is 4 s or more; with less, it uses what there is.

What "dominant" means, as asked: the highest spectral peak from 6 to 100 Hz, if its power is at least 3 times that of the other frequencies. Read literally, "the average power of the other frequencies" found a peak in pure noise, so the rule had to be made precise, against noise with no rhythm in it (`SpectrumPeakTest`):

- **The spectrum** is Welch's: half-second segments, half overlapping (fifteen in 4 s), each detrended and Hann-windowed, their power spectra averaged; bins of 2 Hz. With one-second segments, seven in 4 s, the bins scatter so much that white noise passed the 3× test one time in ten.
- **The peak** is the highest bin from 6 to 100 Hz higher than both its neighbours; at a sampling rate below 250 Hz the range ends lower, at 0.8 of Nyquist, below an amplifier's anti-aliasing filter. The background around a peak may take in bins outside the range. The highest, as asked: picking the one that stands out most above its background named a 150 Hz mains harmonic on band-passed EEG, where it stood out from the noise filtered with it while having next to no power.
- **"The other frequencies"** are those around the peak, leaving out its own two bins on either side: the higher of a line through their log power against log frequency over an octave either side (the 1/f slope EEG rhythms sit on) and the median of the four nearest bins on either side (which a band-pass's cliff next to the peak does not pull down). The average over all frequencies made the lowest bins of any falling spectrum a "dominant peak", 20 noise runs out of 20. One line through the whole range found peaks where the spectrum bends, 1 run in 40; one parabola, inside the edges of a 4–40 Hz band-pass, 39 runs in 400.
- **The frequency** is refined between bins by a parabola through the peak's height above the background and its neighbours'. Noise in those bins leaves it some 0.4 Hz uncertain, so the tooltip gives whole hertz.

With that, noise alone named a frequency 0 times in 1000 (white), 2 in 1000 (brown, the falling spectrum of EEG) and 1 in 400 (band-passed 4–40 Hz like the signal-display tutorial), and an alpha rhythm with about ten times the noise's power in its bin was found in 20 runs of 20, raw and band-passed, within 0.5 Hz. One with two and a half times, plain in the trace, is rightly not dominant. The FFT is Eigen's, as the signal-processing plugins use; it runs only for the lane under the mouse, twice a second.

**Consequences**
- Seen in Designer with the signal-display tutorial, a motor-imagery recording. On the band-passed display, lanes named 7, 11, 12 and 22 Hz, others nothing; on the raw display, a few lanes named 11 or 22 Hz, one the 150 Hz mains harmonic, which the 100 Hz limit, asked for after that, now leaves out. What the recording's rhythms really are was not checked; nor was the display run again after the limit.
- Twelve new unit tests (42 pass).
- A dominant mains line is named as such: on raw EEG the answer can be 50 Hz.

### 2.63 Impedances measured when a recording starts, in the BrainVision header

**Files**
- `sdk/openvibe/include/openvibe/ImpedanceRequest.hpp` (new), `sdk/openvibe/include/openvibe/acquisition-messaging/ovCtrlCmdMsg.h`, `ovChannelInfo.h`
- `extras/applications/platform/acquisition-server/core/Acquisition.h` and `.cpp`, `AcquisitionManager.h` and `.cpp`, `Api.h` and `.cpp`, `net-api/CNetAcqControlAPI.h` and `.cpp`
- `extras/applications/platform/acquisition-server/builtin/drivers/tmsi/CDriverTMSi.h` and `.cpp`, `extras/contrib/plugins/server-drivers/eemagine-eego/src/CDriverEEGO.cpp`
- `extras/applications/platform/acquisition-server/test/AcquisitionImpedanceTest.cpp` (new), `OvMsgSerializerTest.cpp`
- `extras/plugins/processing/acquisition/src/box-algorithms/BoxAcquisitionClient.h` and `.cpp`, `doc/Doc_BoxAlgorithm_AcquisitionClient.dox-part`
- `extras/contrib/plugins/processing/file-io/src/box-algorithms/BrainVisionUG.h`, `ovpCBoxAlgorithmBrainampFileWriterUG.h` and `.cpp`, `test/BrainVisionUGTest.cpp`
- `montages.md` (7.3)

**What it does**
- **The writer asks.** The BrainVision Format file writer (UG) says, when it initialises, that its run wants the impedances: a token in the run's configuration, as `RecordingStatus.hpp` does (`ImpedanceRequest.hpp`). A box can not see which other boxes are in its scenario, and this needs no setting and no link.
- **The client asks the server first.** The Acquisition client no longer asks for the stream as soon as the server's list of acquisitions comes in, but at its first `process()`, when every box has initialised. If the run wants the impedances, it first sends `MEASURE_IMPEDANCES` (`ovCtrlCmdMsg.h`) and asks for the stream when the answer comes, or after 20 s from a server that does not answer.
- **The server measures.** `Acquisition::measureImpedances()`: if the acquisition runs, it stops; the driver goes into impedance mode for 3 s; the last values with something in them are kept, by channel name, in kOhm; the acquisition starts again. The settings it then sends to every client (2.30) carry them, matched to the channels by name, after the positions (`ovChannelInfo.h`: the word `impedances`, the time, one value per channel). Without an acquisition running, the values wait for the next Play. The values of an earlier measurement are dropped first, so a failed one sends none. It runs on the server's control thread, under the lock that Play and Stop in the server's window also take.
- **The drivers measure.** eego through its impedance stream, as before, but without the two values it adds for the reference and the ground, which upstream listed under the names of the first two bipolar channels. TMSi, which had no impedances in the server, as Charmeleon reads them (`TmsiDriver.cs`): the signal buffer at no rate, Start, `SetMeasuringMode(MEASURE_MODE_IMPEDANCE_EX, IC_OHM_200)`, and the first sample of each block, one value per channel in kOhm; for EXG, BIP and AUX channels only. Charmeleon is not involved, and must not be open: it would hold the amplifier.
- **The client passes them on the positions' link.** On its *Channel localisation* output, in a fourth column after x, y and z, labelled `Impedance [kOhm] at <time>` (`impedanceColumnLabel()`), with a row for each channel that has a position or an impedance; a channel without a position has x, y and z at 0, which reads as none. Without impedances the matrix is the three columns it was, so recording scenarios need no new link.
- **The writer lists them as BrainVision Recorder does**, under `[Comment]`: `Impedance [kOhm] at 10:42:17 :`, then `Fp1:          4`, one line per electrode in whole kOhm, then a line saying when the server measured them (`BrainVisionUG::impedanceBlock()`). This is the form BrainVision Analyzer shows and MNE reads into `raw.impedances`. MNE stops at the first name with other characters than letters, digits, spaces and `_+-`, so such names go last.
- **The stimulation stream starts at 0.** The client's first stimulation chunk now runs from the scenario's start to the signal's, as the kernel wants a stimulation stream continuous from 0; with the signal starting later, after the measurement, it warned "sends inconsistent chunk dates on output [1]".
- **Play after the server's own impedance check starts the acquisition.** Upstream's `Acquisition::start()` left impedance mode and started nothing. It now closes the driver and starts from the beginning.

**Consequences**
- Every recording from a scenario with the BrainVision writer (UG) has the impedances of the moment it started, from eego or TMSi, without opening Charmeleon.
- **The server's signal stops for about 4 seconds** when such a scenario starts: LSL Output, so LabRecorder, and any other scenario see a gap there. The LSL streams stay open through it (2.65). The recording itself starts after the measurement and has none. Asked for in this form; measuring at the server's Play, or using Charmeleon's last values, were the alternatives that do not interrupt.
- The scenario's signal begins some 4 s later than before after Play in Designer.
- On the localisation link a box that expects three columns does not take the four-column form; the 2D topographic map is one, and is fed from an electrode file in practice. LSL and XDF do not carry the impedances.
- With TMSi, the server's own impedance list now works too, when Charmeleon is not installed.
- Tested live with the test build, the server on its Dummy driver and Designer without a window: the writer's request reached the client, the server logged it and answered that the driver can not measure, and the client asked for the stream with a warning in Designer's log. Not seen live: an amplifier being measured, and samples reaching the file in that run.
- Tested: 9 new unit tests (the measurement stopping and restarting a running acquisition and sending the values with the settings, waiting for the next Play, a driver that can not measure, Play after the server's own check; the block on the wire, without a montage, from a client before; the header lines, names MNE can not read, none), and the whole suite (the failures are those that fail without this change too: a Japanese path, a socket test, classifier comparisons, and a test that needs the one before it). **Not tried on an amplifier**: the TMSi impedance mode is Charmeleon's, untried in the server; nor the whole chain to a file.

### 2.64 LSL times as LSL asks: the LSL driver and the LSL Export box

A long-standing complaint about OpenViBE and LSL (sccn/lsl_archived [#100](https://github.com/sccn/lsl_archived/issues/100), 2016, and [#333](https://github.com/sccn/lsl_archived/issues/333), 2018; the LSL documentation still warns that the acquisition server "incorrectly overrides the timestamps") is that OpenViBE stamped what it sent to LSL with a clock of its own, so that its streams could not be aligned with others. LSL asks for every sample's time on LSL's own clock, `lsl::local_clock()`, less the sample's age when it is known. The server's LSL Output does so since 2.21. The timing review (`timing-review-2026-11-22.md`, findings 3 and 6) found the two other places where OpenViBE meets LSL still wrong; this section fixes them.

**Files**
- `extras/applications/platform/acquisition-server/builtin/drivers/labstreaminglayer/CDriverLabStreamingLayer.h` and `.cpp`
- `extras/applications/platform/acquisition-server/builtin/LslClock.h` (new), `builtin/writers/lsl-output/LSLOutputWriter.cpp`
- `extras/applications/platform/acquisition-server/abstractions/writer/AbstractWriter.cpp`
- `extras/applications/platform/acquisition-server/test/lsl-input/LSLDriverTest.cpp` and `CMakeLists.txt` (new), `test/CMakeLists.txt`, `test/TcpTagTimingTest.cpp`
- `extras/plugins/processing/network-io/src/box-algorithms/CBoxLSLExport.cpp` and `.hpp`, `LSLTime.h` (new)
- `extras/plugins/processing/network-io/test/LSLTimeTest.cpp` (new), `test/CMakeLists.txt`

**What it does**
- **The LSL driver keeps its sender's times.** It pulled samples and markers without their times: the last sample of each read got the moment of the read, the others times spread evenly back to the read before (the first read's back to Play), and markers the moment they were read. The network's and LSL's delays and the reads' timing went into every time, differently for the samples and the markers, which come by different paths. It now pulls each sample's and marker's LSL time, has liblsl move them to this PC's LSL clock (`post_clocksync`; the signal's also kept increasing, `post_monotonize`; 2.65 dropped the smoothing this first had), and moves them to the server's clock with the offset the LSL Output uses, now shared (`LslClock.h`). A marker so lands on the sample its sender dated it at.
- **LSL markers reach BrainVision files.** A marker value from 1 to 255 is now a trigger, `OVTK_StimulationId_Label(value)`, as an amplifier's are, which the BrainVision writer (UG) records as `S  1` … `S255`. Upstream passed every value on as a stimulation ID (marker 7 became stimulation 7), which the writer leaves out. Other values stay stimulation IDs.
- **A sender that starts again is picked up again:** the driver's inlets now recover.
- **A marker ahead of its samples is no longer lost.** A marker dated after the last sample of the block it came with stayed in that block, which went back to the pool: `AbstractWriter::writeSamples()` now keeps it pending for the chunk that holds its sample, as it already did for TCP tags. With the sender's times, an LSL marker often comes before its samples.
- **The LSL Export box dates by age.** It took the wall-clock time of its first chunk for the scenario's time 0, and added the scenario dates to it: a box whose first chunk came at 2.5 s sent everything 2.5 s late. It now sends each sample and stimulation at `lsl::local_clock()` less its age in the scenario's time (`lslTimeOfScenarioDate()`): a stimulation made in this round is "now". This holds however late the player runs. The `LSL_UseOVTimestamps` token still sends the scenario's own seconds, the behaviour of #333, for whoever asks for it.

**Consequences**
- LSL sources recorded through the acquisition server, into BrainVision or anything else, have their sender's times; their markers sit on the samples their sender meant, and markers 1 to 255 are in the `.vmrk`.
- Markers sent from Designer with LSL Export line up with other LSL streams in LabRecorder, to within a round of the player.
- A stimulation from 1 to 255 sent over LSL as an OpenViBE stimulation ID, rather than as a trigger, now arrives as a trigger.
- Tested: a real LSL sender on this PC whose samples and markers are 5 s old: each arrives at its sender's time, within 1 ms, markers on their samples, the trigger as a label (`LSLDriverTest`; upstream dated them 5 s late); the writer keeps a marker that comes ahead of its samples; the LSL Export's times (4 tests). Not run in Designer, nor against another PC.

### 2.65 The LSL Output's streams stay open when the acquisition starts again; no online smoothing in the LSL driver

**Files**
- `extras/applications/platform/acquisition-server/builtin/writers/lsl-output/LSLOutputWriter.h` and `.cpp`
- `extras/applications/platform/acquisition-server/test/lsl-output/LSLOutputWriterTest.cpp`

**The fault:** every start of an acquisition closed the LSL Output's two outlets and opened them again: each Play in the server, and since 2.63 each impedance measurement when a recording scenario starts. A LabRecorder recording them sees the streams close and come back. Brain Products warns that closing a stream before the recording ends "can result in broken XDF files" ([Tips and tricks for using LSL](https://www.brainproducts.com/support-resources/tips-and-tricks-for-lsl/)); at best the client reconnects (2.21 kept the stream's ID for that) and the file has a gap.

**What it does:** an acquisition that describes the same streams as the last (the stream names, the rate, the channel count, the chunk size, and the channels' names, montage and positions) keeps the outlets open; only the clock offset is measured again. The impedances, which change at every measurement, do not count. Other streams close the old outlets and open new ones, as before. The outlets are closed when the writer goes, and no longer when the acquisition is set up anew.

**Consequences**
- During the impedance measurement at a scenario's start, and after Stop and Play with the same settings, LabRecorder sees a pause in the samples, not streams that close. The pause is still in the data: check an XDF for it before analysis, as Brain Products also advises.
- After Stop the outlets stay open, without samples, until the server's settings change or it closes; upstream did the same until the next Play.
- Tested: a client that does not reconnect keeps getting samples after the acquisition starts again with the same streams and new impedances; another channel name opens a new outlet with the new description; the earlier LSL Output tests (9 pass).

**Also: the LSL driver no longer smooths times online.** 2.64 had liblsl smooth the incoming signal's times (`post_dejitter`). The LSL paper (Kothe et al., bioRxiv 10.1101/2024.02.13.580071) warns that such smoothing can "significantly (even catastrophically) distort the data" when samples are lost or the rate changes: after a dropout, e.g. over Bluetooth, it goes on fitting one line through sample numbers and times, and misdates the samples that follow. The driver now keeps the sender's times, moved to this PC's clock and kept increasing; loaders smooth a recording when they read it, and split it at gaps. Files: `CDriverLabStreamingLayer.cpp`. `LSLDriverTest` passes unchanged, as its sender's times are regular.

---

## 3. All modified files

`A` = added, `M` = modified, `D` = removed. The section column refers to the sections above.

| Status | File | Section |
|---|---|---|
| A | `.github/install-qt.py` | 2.50 |
| A | `.github/workflows/windows-installer.yml` | 2.34, 2.50 |
| M | `CMake/FindThirdPartyLSL.cmake` | 2.21 |
| M | `CMake/OvConfigureRC.cmake` | 2.52 |
| M | `CMakeLists.txt` | 2.37, 2.50 |
| M | `conda/env_linux.yaml` | 2.21 |
| M | `conda/env_osx.yaml` | 2.21 |
| M | `conda/env_osx_arm64.yaml` | 2.21 |
| M | `conda/env_windows.yaml` | 2.21 |
| M | `conda/recipe/meta.yaml` | 2.21 |
| A | `documentation/TMSiSDK.chm` | 2.1 |
| M | `designer/applications/platform/designer/CMakeLists.txt` | 2.33, 2.37 |
| M | `designer/applications/platform/designer/Main.qml` | 2.33, 2.36, 2.47 |
| M | `designer/applications/platform/designer/MenuModel.qml` | 2.36 |
| M | `designer/applications/platform/designer/OVVisualization/Comment.qml` | 2.38 |
| M | `designer/applications/platform/designer/OVVisualization/CommentEditDialog.qml` | 2.38 |
| M | `designer/applications/platform/designer/OVVisualization/PipelineCanvas.qml` | 2.36, 2.38 |
| A | `designer/applications/platform/designer/RuntimeOverlay.qml` | 2.33, 2.44 |
| M | `designer/applications/platform/designer/ShortcutManager.qml` | 2.36 |
| M | `designer/applications/platform/designer/share/designer.conf-base` | 2.33 |
| M | `designer/applications/platform/designer/src/DesignerScenario.cpp` | 2.12, 2.42, 2.46, 2.48, 2.49 |
| M | `designer/applications/platform/designer/src/BoxObject.cpp` | 2.49, 2.53 |
| M | `designer/applications/platform/designer/src/DesignerScenario.h` | 2.36, 2.48 |
| M | `designer/applications/platform/designer/src/OVDesigner.cpp` | 2.32, 2.33, 2.44, 2.46, 2.47, 2.48 |
| M | `designer/applications/platform/designer/src/OVDesigner.h` | 2.33, 2.44, 2.47 |
| M | `designer/applications/platform/designer/src/PlayerBridge.cpp` | 2.18, 2.23, 2.46 |
| M | `designer/applications/platform/designer/src/PlayerBridge.h` | 2.44, 2.46 |
| M | `designer/applications/platform/designer/src/main.cpp` | 2.17 |
| M | `designer/plugins/visualization/simple-visualization/src/algorithms/CAlgorithmSphericalSplineInterpolation.cpp` | 2.49 |
| M | `designer/plugins/visualization/simple-visualization/src/algorithms/CAlgorithmSphericalSplineInterpolation.hpp` | 2.49 |
| M | `designer/plugins/visualization/simple-visualization/src/algorithms/spline.cpp` | 2.49 |
| M | `designer/plugins/visualization/simple-visualization/src/box-algorithms/topographicMap2DDisplay/CTopographicMap2DView.cpp` | 2.49 |
| M | `designer/plugins/visualization/simple-visualization/src/box-algorithms/topographicMap2DDisplay/CTopographicMap2DView.hpp` | 2.49 |
| M | `designer/plugins/visualization/simple-visualization/src/box-algorithms/topographicMap2DDisplay/CTopographicMapDatabase.cpp` | 2.49 |
| M | `designer/plugins/visualization/simple-visualization/src/box-algorithms/topographicMap2DDisplay/CTopographicMapDatabase.hpp` | 2.49 |
| M | `extras/applications/developer-tools/plugin-inspector/src/CAlgorithmGlobalDefinesGen.cpp` | 2.23 |
| M | `extras/applications/developer-tools/plugin-inspector/src/CBoxQmlRenderer.cpp` | 2.35 |
| M | `extras/applications/developer-tools/skeleton-generator/src/CDriverSkeletonGenerator.cpp` | 2.23 |
| M | `extras/applications/developer-tools/skeleton-generator/src/CSkeletonGenerator.cpp` | 2.23 |
| M | `extras/applications/developer-tools/skeleton-generator/src/main.cpp` | 2.23 |
| M | `extras/applications/platform/acquisition-server/CMakeLists.txt` | 2.1, 2.4, 2.6, 2.17, 2.37, 2.42 |
| M | `extras/applications/platform/acquisition-server/abstractions/CMakeLists.txt` | 2.3, 2.24, 2.25, 2.41 |
| M | `extras/applications/platform/acquisition-server/abstractions/driver/AbstractDriver.cpp` | 2.16, 2.41 |
| M | `extras/applications/platform/acquisition-server/abstractions/driver/AbstractDriver.h` | 2.4, 2.6, 2.7, 2.8, 2.16, 2.41, 2.42 |
| A | `extras/applications/platform/acquisition-server/abstractions/driver/BlockStamps.h` | 2.24 |
| A | `extras/applications/platform/acquisition-server/abstractions/driver/CharmeleonDriver.cpp` | 2.41 |
| A | `extras/applications/platform/acquisition-server/abstractions/driver/CharmeleonDriver.h` | 2.41 |
| A | `extras/applications/platform/acquisition-server/abstractions/driver/CharmeleonMontage.cpp` | 2.3, 2.41, 2.42 |
| A | `extras/applications/platform/acquisition-server/abstractions/driver/CharmeleonMontage.h` | 2.3, 2.41, 2.42 |
| A | `extras/applications/platform/acquisition-server/abstractions/driver/SteadyTrigger.h` | 2.25 |
| M | `extras/applications/platform/acquisition-server/abstractions/writer/AbstractWriter.cpp` | 2.7, 2.18, 2.42, 2.64 |
| M | `extras/applications/platform/acquisition-server/abstractions/writer/AbstractWriter.h` | 2.7, 2.21, 2.42 |
| A | `extras/applications/platform/acquisition-server/builtin/LslClock.h` | 2.64 |
| M | `extras/applications/platform/acquisition-server/builtin/drivers/labstreaminglayer/CDriverLabStreamingLayer.cpp` | 2.64, 2.65 |
| M | `extras/applications/platform/acquisition-server/builtin/drivers/labstreaminglayer/CDriverLabStreamingLayer.h` | 2.64 |
| A | `extras/applications/platform/acquisition-server/builtin/drivers/tmsi/CDriverTMSi.cpp` | 2.1, 2.3, 2.4, 2.5, 2.6, 2.16, 2.24, 2.25, 2.27, 2.41, 2.42, 2.63 |
| A | `extras/applications/platform/acquisition-server/builtin/drivers/tmsi/CDriverTMSi.h` | 2.1, 2.3, 2.4, 2.6, 2.7, 2.24, 2.25, 2.27, 2.41, 2.63 |
| M | `extras/applications/platform/acquisition-server/builtin/writers/lsl-output/LSLOutputWriter.cpp` | 2.21, 2.64, 2.65 |
| M | `extras/applications/platform/acquisition-server/builtin/writers/lsl-output/LSLOutputWriter.h` | 2.21, 2.65 |
| M | `extras/applications/platform/acquisition-server/core/Acquisition.cpp` | 2.4, 2.6, 2.7, 2.21, 2.30, 2.41, 2.42, 2.63 |
| M | `extras/applications/platform/acquisition-server/core/Acquisition.h` | 2.4, 2.6, 2.7, 2.8, 2.16, 2.41, 2.63 |
| M | `extras/applications/platform/acquisition-server/core/AcquisitionManager.cpp` | 2.63 |
| M | `extras/applications/platform/acquisition-server/core/AcquisitionManager.h` | 2.63 |
| M | `extras/applications/platform/acquisition-server/core/Api.cpp` | 2.63 |
| M | `extras/applications/platform/acquisition-server/core/Api.h` | 2.63 |
| M | `extras/applications/platform/acquisition-server/core/CircularBuffer.cpp` | 2.4 |
| M | `extras/applications/platform/acquisition-server/core/CircularBuffer.h` | 2.4 |
| M | `extras/applications/platform/acquisition-server/core/net-api/CNetAcqControlAPI.cpp` | 2.4, 2.63 |
| M | `extras/applications/platform/acquisition-server/core/net-api/CNetAcqControlAPI.h` | 2.4, 2.63 |
| M | `extras/applications/platform/acquisition-server/main.cpp` | 2.17, 2.31 |
| A | `extras/applications/platform/acquisition-server/test/AcquisitionConfigTest.cpp` | 2.30 |
| A | `extras/applications/platform/acquisition-server/test/AcquisitionImpedanceTest.cpp` | 2.63 |
| A | `extras/applications/platform/acquisition-server/test/CharmeleonMontageTest.cpp` | 2.3 |
| A | `extras/applications/platform/acquisition-server/test/CircularBufferTest.cpp` | 2.4 |
| A | `extras/applications/platform/acquisition-server/test/OvMsgSerializerTest.cpp` | 2.7, 2.41, 2.63 |
| M | `extras/applications/platform/acquisition-server/ui/AcquisitionController.cpp` | 2.4, 2.6, 2.8, 2.16, 2.41, 2.42, 2.47, 2.49 |
| M | `extras/applications/platform/acquisition-server/ui/AcquisitionController.h` | 2.4, 2.6, 2.8, 2.16, 2.42, 2.47 |
| A | `extras/applications/platform/acquisition-server/ui/Charmeleon.cpp` | 2.6, 2.41, 2.42 |
| A | `extras/applications/platform/acquisition-server/ui/Charmeleon.h` | 2.6, 2.41 |
| M | `extras/applications/platform/acquisition-server/ui/qml/ChannelRename.qml` | 2.8 |
| A | `extras/applications/platform/acquisition-server/ui/qml/BufferControl.qml` | 2.4, 2.42 |
| D | `extras/applications/platform/acquisition-server/ui/qml/DriftBar.qml` | 2.4 |
| D | `extras/applications/platform/acquisition-server/ui/qml/DriftControl.qml` | 2.4 |
| A | `extras/applications/platform/acquisition-server/ui/qml/DriverNote.qml` | 2.16, 2.42 |
| A | `extras/applications/platform/acquisition-server/ui/qml/LevelBar.qml` | 2.4, 2.42 |
| M | `extras/applications/platform/acquisition-server/ui/qml/Main.qml` | 2.4, 2.8, 2.16, 2.47 |
| M | `extras/applications/qml/OVControls/Logger.qml` | 2.23, 2.56 |
| M | `extras/applications/qml/OVControls/OVLogController.cpp` | 2.56 |
| M | `extras/applications/qml/OVControls/OVLogController.h` | 2.56 |
| M | `extras/applications/qml/OVDesignSystem/Theme.qml` | 2.42 |
| M | `extras/applications/qml/OVParameters/CMakeLists.txt` | 2.22 |
| M | `extras/applications/qml/OVParameters/MultiSelectParameter.qml` | 2.26 |
| M | `extras/applications/qml/OVParameters/PathParameter.qml` | 2.13 |
| M | `extras/applications/qml/OVParameters/src/parameterObject.cpp` | 2.26 |
| M | `extras/applications/qml/OVParameters/src/parameterObject.h` | 2.26 |
| M | `extras/applications/qml/OVParameters/src/parameterPathObject.cpp` | 2.13, 2.22 |
| M | `extras/applications/qml/OVParameters/src/parameterPathObject.h` | 2.13 |
| A | `extras/applications/qml/OVParameters/test/CMakeLists.txt` | 2.22 |
| A | `extras/applications/qml/OVParameters/test/ParameterPathObjectTest.cpp` | 2.22 |
| A | `extras/contrib/plugins/processing/file-io/scripts/check_brainvision.py` | 2.27, 2.45, 2.47 |
| A | `extras/contrib/plugins/processing/file-io/src/box-algorithms/BrainVisionUG.h` | 2.27, 2.63 |
| A | `extras/contrib/plugins/processing/file-io/src/box-algorithms/ovpCBoxAlgorithmBrainampFileWriterUG.cpp` | 2.9, 2.27, 2.41, 2.44, 2.45, 2.47, 2.63 |
| A | `extras/contrib/plugins/processing/file-io/src/box-algorithms/ovpCBoxAlgorithmBrainampFileWriterUG.h` | 2.9, 2.27, 2.41, 2.44, 2.45, 2.63 |
| M | `extras/contrib/plugins/processing/file-io/src/ovp_defines.h` | 2.9 |
| M | `extras/contrib/plugins/processing/file-io/src/ovp_main.cpp` | 2.9 |
| A | `extras/contrib/plugins/processing/file-io/test/BrainVisionUGTest.cpp` | 2.27, 2.63 |
| M | `extras/contrib/plugins/processing/file-io/test/CMakeLists.txt` | 2.27, 2.45, 2.47 |
| A | `extras/contrib/plugins/processing/file-io/test/scenarios-tests/BrainVision-UG-Write-input.csv` | 2.27 |
| A | `extras/contrib/plugins/processing/file-io/test/scenarios-tests/BrainVision-UG-Write-test.xml` | 2.27, 2.41, 2.45 |
| M | `extras/contrib/plugins/server-drivers/CMakeLists.txt` | 2.2 |
| M | `extras/contrib/plugins/server-drivers/eemagine-eego/src/CDriverEEGO.cpp` | 2.2, 2.3, 2.4, 2.5, 2.6, 2.16, 2.24, 2.25, 2.27, 2.41, 2.42, 2.63 |
| M | `extras/contrib/plugins/server-drivers/eemagine-eego/src/CDriverEEGO.h` | 2.2, 2.3, 2.4, 2.6, 2.7, 2.24, 2.25, 2.27, 2.41 |
| M | `extras/cmake-modules/FindThirdPartyMatlab.cmake` | 2.51 |
| M | `extras/plugins/processing/acquisition/CMakeLists.txt` | 2.20 |
| M | `extras/plugins/processing/acquisition/doc/Doc_BoxAlgorithm_AcquisitionClient.dox-part` | 2.63 |
| M | `extras/plugins/processing/acquisition/src/box-algorithms/BoxAcquisitionClient.cpp` | 2.5, 2.7, 2.20, 2.30, 2.41, 2.63 |
| M | `extras/plugins/processing/acquisition/src/box-algorithms/BoxAcquisitionClient.h` | 2.7, 2.20, 2.30, 2.41, 2.63 |
| A | `extras/plugins/processing/acquisition/src/box-algorithms/SampleBlocks.h` | 2.30 |
| A | `extras/plugins/processing/acquisition/src/box-algorithms/StimulationDelay.h` | 2.20 |
| A | `extras/plugins/processing/acquisition/test/CMakeLists.txt` | 2.20, 2.30 |
| A | `extras/plugins/processing/acquisition/test/SampleBlocksTest.cpp` | 2.30 |
| A | `extras/plugins/processing/acquisition/test/StimulationDelayTest.cpp` | 2.20 |
| A | `extras/applications/platform/acquisition-server/builtin/drivers/tagging/TagDate.h` | 2.18 |
| M | `extras/applications/platform/acquisition-server/builtin/drivers/tagging/TaggingDriver.cpp` | 2.18 |
| A | `extras/applications/platform/acquisition-server/test/BlockStampsTest.cpp` | 2.24 |
| A | `extras/applications/platform/acquisition-server/test/SteadyTriggerTest.cpp` | 2.25 |
| A | `extras/applications/platform/acquisition-server/test/TcpTagTimingTest.cpp` | 2.18, 2.64 |
| M | `extras/applications/platform/acquisition-server/test/CMakeLists.txt` | 2.21, 2.42, 2.64 |
| A | `extras/applications/platform/acquisition-server/test/lsl-input/CMakeLists.txt` | 2.64 |
| A | `extras/applications/platform/acquisition-server/test/lsl-input/LSLDriverTest.cpp` | 2.64 |
| A | `extras/applications/platform/acquisition-server/test/lsl-output/CMakeLists.txt` | 2.21 |
| A | `extras/applications/platform/acquisition-server/test/lsl-output/LSLOutputWriterTest.cpp` | 2.21, 2.65 |
| M | `extras/applications/qml/OVDesignSystem/qml/ToolTip.qml` | 2.61 |
| A | `extras/applications/demos/p300/bci-examples/p300-speller-python/` (5 scripts, 5 scenarios and a README) | 2.18 |
| A | `extras/applications/demos/p300/bci-examples/p300-speller-xDAWN-python/` (6 scenarios, and a README) | 2.18 |
| M | `extras/contrib/plugins/processing/python3/share/StimulationsCodes.py` | 2.18 |
| A | `extras/contrib/plugins/processing/python3/share/ovstimulationbox.py` | 2.18 |
| M | `extras/plugins/processing/artifact/src/boxes/CBoxAlgorithmASRProcessor.cpp` | 2.23 |
| M | `extras/plugins/processing/artifact/src/boxes/CBoxAlgorithmASRTrainer.cpp` | 2.23 |
| M | `extras/plugins/processing/artifact/src/boxes/CBoxAlgorithmAmplitudeArtifactDetector.cpp` | 2.23 |
| M | `extras/plugins/processing/evaluation/src/box-algorithms/CROCCurveView.cpp` | 2.49 |
| M | `extras/plugins/processing/features-selection/src/boxes/CBoxAlgorithmFeaturesSelection.cpp` | 2.23 |
| M | `extras/plugins/processing/features-selection/src/boxes/CBoxAlgorithmFeaturesSelector.cpp` | 2.23 |
| A | `extras/plugins/processing/matlab/box-tutorials/matlab/tuto4-square-eeg.xml` | 2.57 |
| A | `extras/plugins/processing/matlab/box-tutorials/matlab/tuto4_square_eeg_Initialize.m` | 2.57 |
| A | `extras/plugins/processing/matlab/box-tutorials/matlab/tuto4_square_eeg_Process.m` | 2.57 |
| A | `extras/plugins/processing/matlab/box-tutorials/matlab/tuto4_square_eeg_Uninitialize.m` | 2.57 |
| M | `extras/plugins/processing/matlab/CMakeLists.txt` | 2.51 |
| M | `extras/plugins/processing/matlab/share/OV_define.m` | 2.59 |
| A | `extras/plugins/processing/matlab/src/MatlabEngineThread.h` | 2.58 |
| M | `extras/plugins/processing/matlab/src/box-algorithms/ovpCBoxAlgorithmMatlabScripting.cpp` | 2.23, 2.55, 2.58 |
| M | `extras/plugins/processing/matlab/src/box-algorithms/ovpCBoxAlgorithmMatlabScripting.h` | 2.58 |
| M | `extras/plugins/processing/matlab/src/ovpCMatlabHelper.h` | 2.58 |
| M | `extras/plugins/processing/network-io/src/box-algorithms/CBoxLSLCommunication.cpp` | 2.23 |
| M | `extras/plugins/processing/network-io/src/box-algorithms/CBoxLSLExport.cpp` | 2.64 |
| M | `extras/plugins/processing/network-io/src/box-algorithms/CBoxLSLExport.hpp` | 2.64 |
| A | `extras/plugins/processing/network-io/src/box-algorithms/LSLTime.h` | 2.64 |
| M | `extras/plugins/processing/network-io/test/CMakeLists.txt` | 2.64 |
| A | `extras/plugins/processing/network-io/test/LSLTimeTest.cpp` | 2.64 |
| M | `extras/plugins/processing/riemannian/src/boxes/CBoxAlgorithmCovarianceMatrixToFeatureVector.cpp` | 2.23 |
| M | `extras/plugins/processing/riemannian/src/boxes/CBoxAlgorithmCovarianceMeanCalculator.cpp` | 2.23 |
| M | `extras/plugins/processing/riemannian/src/boxes/CBoxAlgorithmFeatureVectorToCovarianceMatrix.cpp` | 2.23 |
| M | `extras/plugins/processing/riemannian/src/boxes/CBoxAlgorithmMatrixAffineTransformation.cpp` | 2.23 |
| M | `extras/plugins/processing/riemannian/src/boxes/CBoxAlgorithmMatrixClassifierProcessor.cpp` | 2.23 |
| M | `extras/plugins/processing/riemannian/src/boxes/CBoxAlgorithmMatrixClassifierTrainer.cpp` | 2.23 |
| M | `extras/plugins/processing/signal-processing/src/box-algorithms/basic/CBoxAlgorithmMatrix2dToVector.cpp` | 2.23 |
| M | `extras/plugins/processing/signal-processing/src/box-algorithms/basic/CBoxAlgorithmMatrix3dTo2d.cpp` | 2.23 |
| M | `extras/plugins/processing/signal-processing/src/box-algorithms/connectivity/CBoxAlgorithmConnectivityMeasure.cpp` | 2.23 |
| M | `extras/plugins/processing/stimulation/share/lua-stimulator-stim-codes.lua` | 2.18 |
| M | `extras/plugins/processing/simple-visualization/CMakeLists.txt` | 2.18, 2.26, 2.28, 2.40, 2.41, 2.62 |
| A | `extras/plugins/processing/simple-visualization/box-tutorials/lsl-display-xdf.xml` | 2.29, 2.39 |
| A | `extras/plugins/processing/simple-visualization/box-tutorials/lsl-display.xml` | 2.28, 2.29, 2.39 |
| M | `extras/plugins/processing/simple-visualization/doc/Doc_BoxAlgorithm_P300SpellerVisualization.dox-part` | 2.18 |
| M | `extras/plugins/processing/simple-visualization/doc/Doc_BoxAlgorithm_SignalDisplay.dox-part` | 2.26, 2.39, 2.40, 2.43, 2.61, 2.62 |
| M | `extras/plugins/processing/simple-visualization/qml/AmplitudeScaleBar.qml` | 2.26, 2.61, 2.62 |
| M | `extras/plugins/processing/simple-visualization/qml/ChannelGutter.qml` | 2.43 |
| A | `extras/plugins/processing/simple-visualization/qml/P300SpellerVisualization.qml` | 2.18 |
| A | `extras/plugins/processing/simple-visualization/qml/LSLDisplay.qml` | 2.28, 2.41, 2.43 |
| A | `extras/plugins/processing/simple-visualization/qml/SettingsPanel.qml` | 2.28 |
| M | `extras/plugins/processing/simple-visualization/qml/SignalDisplay.qml` | 2.26, 2.28, 2.41, 2.43 |
| M | `extras/plugins/processing/simple-visualization/qml/StimulationOverlay.qml` | 2.11 |
| A | `extras/plugins/processing/simple-visualization/qml/TracePanel.qml` | 2.41 |
| M | `extras/plugins/processing/simple-visualization/qml/TimeRuler.qml` | 2.26 |
| M | `extras/plugins/processing/simple-visualization/src/box-algorithms/CBoxAlgorithmP300SpellerVisualization.cpp` | 2.18 |
| M | `extras/plugins/processing/simple-visualization/src/box-algorithms/CBoxAlgorithmP300SpellerVisualization.hpp` | 2.18 |
| M | `extras/plugins/processing/simple-visualization/src/box-algorithms/CGrazMultiVisualization.cpp` | 2.23 |
| A | `extras/plugins/processing/simple-visualization/src/box-algorithms/CP300SpellerView.cpp` | 2.18 |
| A | `extras/plugins/processing/simple-visualization/src/box-algorithms/CP300SpellerView.h` | 2.18 |
| A | `extras/plugins/processing/simple-visualization/src/box-algorithms/CLSLDisplay.cpp` | 2.28, 2.29 |
| A | `extras/plugins/processing/simple-visualization/src/box-algorithms/CLSLDisplay.h` | 2.28, 2.29 |
| A | `extras/plugins/processing/simple-visualization/src/box-algorithms/CLSLXDFWriter.cpp` | 2.29 |
| A | `extras/plugins/processing/simple-visualization/src/box-algorithms/CLSLXDFWriter.h` | 2.29 |
| M | `extras/plugins/processing/simple-visualization/src/box-algorithms/CSignalDisplay.cpp` | 2.11, 2.26, 2.28, 2.39 |
| M | `extras/plugins/processing/simple-visualization/src/box-algorithms/CSignalDisplay.h` | 2.26, 2.28, 2.39 |
| M | 35 demo and tutorial scenarios (`*.xml`) with a Signal display | 2.11 |
| M | 31 demo and tutorial scenarios (`*.xml`) with an Acquisition client box | 2.19, 2.41 |
| M | 59 demo, tutorial and test scenarios (`*.xml`) with a Signal display | 2.26, 2.39 |
| M | `extras/plugins/server-extensions/tcp-tagging-service/test/test_tcptaggingservice.cpp` | 2.19 |
| A | `extras/plugins/processing/simple-visualization/src/box-algorithms/P300SpellerState.cpp` | 2.18 |
| A | `extras/plugins/processing/simple-visualization/src/box-algorithms/P300SpellerState.h` | 2.18 |
| A | `extras/plugins/processing/simple-visualization/src/CLSLDisplayController.cpp` | 2.28, 2.29, 2.39, 2.40, 2.41 |
| A | `extras/plugins/processing/simple-visualization/src/CLSLDisplayController.h` | 2.28, 2.29, 2.39, 2.40, 2.41, 2.43 |
| A | `extras/plugins/processing/simple-visualization/src/CLSLReader.cpp` | 2.28, 2.42 |
| A | `extras/plugins/processing/simple-visualization/src/CLSLReader.h` | 2.28 |
| A | `extras/plugins/processing/simple-visualization/src/CLSLRecorder.cpp` | 2.29 |
| A | `extras/plugins/processing/simple-visualization/src/CLSLRecorder.h` | 2.29 |
| M | `extras/plugins/processing/simple-visualization/src/CSignalDisplayController.cpp` | 2.26, 2.28, 2.39, 2.40, 2.41, 2.43 |
| M | `extras/plugins/processing/simple-visualization/src/CSignalDisplayController.h` | 2.26, 2.39, 2.40, 2.41, 2.43 |
| M | `extras/plugins/processing/simple-visualization/src/CSignalTraceItem.cpp` | 2.26, 2.28, 2.39, 2.40, 2.41, 2.61, 2.62 |
| M | `extras/plugins/processing/simple-visualization/src/CSignalTraceItem.h` | 2.26, 2.28, 2.39, 2.41, 2.61, 2.62 |
| M | `extras/plugins/processing/simple-visualization/src/CTraceBuffer.cpp` | 2.26, 2.28 |
| M | `extras/plugins/processing/simple-visualization/src/CTraceBuffer.h` | 2.26, 2.28 |
| A | `extras/plugins/processing/simple-visualization/src/DisplayPanel.cpp` | 2.41, 2.60 |
| A | `extras/plugins/processing/simple-visualization/src/DisplayPanel.h` | 2.41 |
| M | `extras/plugins/processing/simple-visualization/src/SDisplaySettings.h` | 2.26, 2.39, 2.40, 2.41 |
| A | `extras/plugins/processing/simple-visualization/src/SignalScaling.h` | 2.26, 2.41, 2.61 |
| A | `extras/plugins/processing/simple-visualization/src/SpectrumPeak.h` | 2.62 |
| A | `extras/plugins/processing/simple-visualization/src/TraceShading.h` | 2.40 |
| A | `extras/plugins/processing/simple-visualization/src/TracePalette.h` | 2.28 |
| A | `extras/plugins/processing/simple-visualization/src/XDFWriter.cpp` | 2.29 |
| A | `extras/plugins/processing/simple-visualization/src/XDFWriter.h` | 2.29 |
| M | `extras/plugins/processing/simple-visualization/src/defines.hpp` | 2.26, 2.28 |
| M | `extras/plugins/processing/simple-visualization/src/main.cpp` | 2.18, 2.26, 2.28 |
| M | `extras/plugins/processing/simple-visualization/test/CMakeLists.txt` | 2.18, 2.26, 2.28, 2.40, 2.62 |
| A | `extras/plugins/processing/simple-visualization/test/LSLReaderTest.cpp` | 2.28 |
| A | `extras/plugins/processing/simple-visualization/test/LSLRecorderTest.cpp` | 2.29 |
| A | `extras/plugins/processing/simple-visualization/test/P300SpellerStateTest.cpp` | 2.18 |
| A | `extras/plugins/processing/simple-visualization/test/SignalScalingTest.cpp` | 2.26, 2.41, 2.61 |
| A | `extras/plugins/processing/simple-visualization/test/SpectrumPeakTest.cpp` | 2.62 |
| A | `extras/plugins/processing/simple-visualization/test/TraceBufferTest.cpp` | 2.26, 2.28 |
| A | `extras/plugins/processing/simple-visualization/test/TraceShadingTest.cpp` | 2.40 |
| A | `extras/plugins/processing/simple-visualization/test/scenarios-tests/P300-speller-lua-test.xml` | 2.18 |
| A | `extras/plugins/processing/simple-visualization/test/scenarios-tests/P300-speller-python-check.py` | 2.18 |
| A | `extras/plugins/processing/simple-visualization/test/scenarios-tests/P300-speller-python-test.xml` | 2.18 |
| A | `extras/plugins/processing/simple-visualization/test/scenarios-tests/P300-speller-test.xml` | 2.18 |
| M | `sdk/kernel/src/kernel/configuration/ovkCConfigurationManager.cpp` | 2.23 |
| M | `sdk/kernel/CMakeLists.txt` | 2.42 |
| M | `pixi.lock` | 2.15, 2.21 |
| M | `pixi.toml` | 2.15, 2.21, 2.50 |
| M | `README.md` | 2.50 |
| M | `sdk/kernel/src/kernel/ovkCKernelContext.cpp` | 2.10, 2.42 |
| M | `sdk/kernel/src/kernel/player/ovkCBoxSettingModifierVisitor.cpp` | 2.23 |
| M | `sdk/kernel/src/kernel/player/ovkCPlayer.cpp` | 2.23 |
| M | `sdk/kernel/src/kernel/player/ovkCScheduler.cpp` | 2.23 |
| M | `sdk/kernel/src/kernel/player/ovkCSimulatedBox.cpp` | 2.23 |
| M | `sdk/kernel/src/kernel/scenario/ovkCBoxUpdater.cpp` | 2.48 |
| M | `sdk/kernel/src/kernel/scenario/ovkCBoxUpdater.h` | 2.48 |
| M | `sdk/kernel/src/kernel/scenario/ovkCScenario.cpp` | 2.48 |
| M | `sdk/kernel/src/kernel/scenario/ovkCScenarioManager.cpp` | 2.23 |
| M | `sdk/modules/network/Client/CClient.cpp` | 2.19 |
| M | `sdk/modules/network/AbstractionLayer/NAL.h` | 2.41 |
| M | `sdk/modules/network/Server/CServer.cpp` | 2.4, 2.31, 2.41, 2.42 |
| M | `sdk/modules/network/Server/CServer.h` | 2.31 |
| M | `sdk/modules/network/Server/ConnectedClient.cpp` | 2.19 |
| A | `sdk/openvibe/include/openvibe/ConsoleLog.hpp` | 2.17 |
| A | `sdk/openvibe/include/openvibe/ErrorApology.hpp` | 2.54 |
| A | `sdk/openvibe/include/openvibe/ImpedanceRequest.hpp` | 2.63 |
| A | `sdk/openvibe/include/openvibe/RecordingStatus.hpp` | 2.44 |
| A | `sdk/openvibe/include/openvibe/acquisition-messaging/ovChannelInfo.h` | 2.7, 2.41, 2.63 |
| M | `sdk/openvibe/include/openvibe/acquisition-messaging/ovCtrlCmdMsg.h` | 2.63 |
| M | `sdk/openvibe/include/openvibe/acquisition-messaging/ovMsg.h` | 2.7 |
| M | `sdk/openvibe/include/openvibe/acquisition-messaging/ovMsgSerializer.h` | 2.7, 2.42 |
| M | `sdk/openvibe/include/openvibe/ovAssert.h` | 2.23, 2.54 |
| A | `sdk/openvibe/src/ErrorApology.cpp` | 2.54 |
| M | `sdk/plugins/processing/classification/src/algorithms/CAlgorithmClassifierLDA.cpp` | 2.23 |
| M | `sdk/plugins/processing/classification/src/algorithms/CAlgorithmClassifierOneVsAll.cpp` | 2.23 |
| M | `sdk/plugins/processing/classification/src/algorithms/CAlgorithmClassifierOneVsOne.cpp` | 2.23 |
| M | `sdk/plugins/processing/classification/src/algorithms/CAlgorithmConditionedCovariance.cpp` | 2.23 |
| M | `sdk/plugins/processing/classification/src/algorithms/CAlgorithmPairwiseDecisionHT.cpp` | 2.23 |
| M | `sdk/plugins/processing/classification/src/box-algorithms/CBoxAlgorithmClassifierProcessor.cpp` | 2.23 |
| M | `sdk/plugins/processing/classification/src/box-algorithms/CBoxAlgorithmClassifierTrainer.cpp` | 2.23 |
| M | `sdk/plugins/processing/classification/src/box-algorithms/CBoxAlgorithmCommonClassifierListener.inl` | 2.23 |
| M | `sdk/plugins/processing/file-io/src/algorithms/ovpCAlgorithmOVMatrixFileReader.cpp` | 2.23 |
| M | `sdk/plugins/processing/file-io/src/algorithms/ovpCAlgorithmOVMatrixFileWriter.cpp` | 2.23 |
| M | `sdk/plugins/processing/file-io/src/box-algorithms/csv/CBoxAlgorithmOVCSVFileReader.cpp` | 2.23, 2.41 |
| M | `sdk/plugins/processing/file-io/src/box-algorithms/csv/CBoxAlgorithmOVCSVFileWriter.cpp` | 2.23, 2.41 |
| M | `sdk/plugins/processing/file-io/src/box-algorithms/csv/ovpCBoxAlgorithmCSVFileReader.cpp` | 2.23, 2.41 |
| M | `sdk/plugins/processing/file-io/src/box-algorithms/csv/ovpCBoxAlgorithmCSVFileWriter.cpp` | 2.23, 2.41 |
| M | `sdk/plugins/processing/file-io/src/box-algorithms/openvibe/ovpCBoxAlgorithmGenericStreamReader.cpp` | 2.23, 2.41 |
| M | `sdk/plugins/processing/file-io/src/box-algorithms/openvibe/ovpCBoxAlgorithmGenericStreamWriter.cpp` | 2.23, 2.41 |
| M | `sdk/plugins/processing/signal-processing/src/algorithms/basic/CAlgorithmOnlineCovariance.cpp` | 2.23 |
| M | `sdk/plugins/processing/signal-processing/src/box-algorithms/CBoxAlgorithmWindowing.cpp` | 2.23 |
| M | `sdk/plugins/processing/signal-processing/src/box-algorithms/CBoxXDAWNTrainer.cpp` | 2.23 |
| M | `sdk/plugins/processing/signal-processing/src/box-algorithms/basic/CBoxAlgorithmChannelRename.cpp` | 2.23 |
| M | `sdk/plugins/processing/signal-processing/src/box-algorithms/basic/CBoxAlgorithmChannelSelector.cpp` | 2.23 |
| M | `sdk/plugins/processing/signal-processing/src/box-algorithms/basic/CBoxAlgorithmCrop.cpp` | 2.23 |
| M | `sdk/plugins/processing/signal-processing/src/box-algorithms/basic/CBoxAlgorithmEpochAverage.cpp` | 2.23 |
| M | `sdk/plugins/processing/signal-processing/src/box-algorithms/basic/CBoxAlgorithmReferenceChannel.cpp` | 2.23 |
| M | `sdk/plugins/processing/signal-processing/src/box-algorithms/basic/CBoxAlgorithmSignalDecimation.cpp` | 2.23 |
| M | `sdk/plugins/processing/signal-processing/src/box-algorithms/basic/CBoxAlgorithmZeroCrossingDetector.cpp` | 2.23 |
| M | `sdk/plugins/processing/signal-processing/src/box-algorithms/filters/CBoxAlgorithmRegularizedCSPTrainer.cpp` | 2.23 |
| M | `sdk/plugins/processing/signal-processing/src/box-algorithms/filters/CBoxAlgorithmSpatialFilter.cpp` | 2.23 |
| M | `sdk/plugins/processing/signal-processing/src/box-algorithms/filters/CBoxAlgorithmTemporalFilter.cpp` | 2.23 |
| M | `sdk/plugins/processing/signal-processing/src/box-algorithms/resampling/CBoxAlgorithmSignalResampling.cpp` | 2.23 |
| M | `sdk/plugins/processing/signal-processing/src/box-algorithms/spectral-analysis/CBoxAlgorithmFrequencyBandSelector.cpp` | 2.23 |
| M | `sdk/plugins/processing/stimulation/src/box-algorithms/CBoxAlgorithmClockStimulator.cpp` | 2.23 |
| M | `sdk/plugins/processing/stimulation/src/box-algorithms/CBoxAlgorithmPlayerController.cpp` | 2.23 |
| M | `sdk/plugins/processing/stimulation/src/box-algorithms/CBoxAlgorithmTimeout.cpp` | 2.23 |
| M | `sdk/plugins/processing/tools/src/box-algorithms/CBoxAlgorithmExternalProcessing.cpp` | 2.23 |
| M | `sdk/plugins/processing/tools/src/box-algorithms/CBoxAlgorithmMatrixValidityChecker.cpp` | 2.23 |
| M | `sdk/toolkit/src/algorithms/scenario-io/ovtkCAlgorithmScenarioImporter.cpp` | 2.23, 2.48 |
| M | `sdk/toolkit/include/toolkit/box-algorithms/ovtkTBoxAlgorithm.h` | 2.41 |
| A | `sdk/unit-test/ov-base/ErrorApologyTest.hpp` | 2.54 |
| M | `sdk/unit-test/ov-base/main.cpp` | 2.54 |
| A | `share/ov-acquisition-server-icon.ico` | 2.37 |
| A | `share/tools/make_acquisition_server_icon.py` | 2.37 |
| A | `UG.md` | this document |
| A | `montages.md` | 2.3: the montage how-to, 2.63 |
| A | `review-2026-11-18.md` | 2.41: the review of the fork's code, two passes |
| A | `timing-review-2026-11-22.md` | 2.63: a review of how samples and markers get their times; 2.64: findings 3 and 6 fixed |
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
  - `Acquisition`, `CircularBuffer`, `AcquisitionController`
  - the network control API and `CServer`
  - Designer's and the server's `main.cpp`
  - `DesignerScenario::clickPort`
  - the kernel context
  - `pixi.toml` and `pixi.lock`

  New files (TMSi driver, Charmeleon, UG writer, buffer display) merge cleanly.
- **Charmeleon.** The montage round trip and the choice of amplifier need Charmeleon 2.2 (see 2.6). Charmeleon is a separate repository and installer.
- **Tests.** The acquisition server unit tests (`test-acquisition-server-core`, 53 tests) pass. They include new tests for the circular buffer, the config message and the montage rules. So do the P300 speller tests (`test-p300-speller`, 12 tests), for the letter grid, what the display shows and when a frame showed it. So do the Signal display tests (`test-signal-display`, 14 tests), for channel kinds, round and shared scales, and the ring of samples.
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
