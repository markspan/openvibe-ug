# Montages in the UG-lab EEG workflow

A how-to for montages in the UG fork of OpenViBE: what they are and why each amplifier needs a different kind, how to make one in Charmeleon, and how the acquisition server and the BrainVision writer use it.

**You need:**
- the UG fork of OpenViBE (this repository), built from the commit that added this file or later;
- Charmeleon 2.2 or later. Charmeleon 2.1 opens montages but cannot hand edits back to the server, see [Editing a montage from the acquisition server](#editing-a-montage-from-the-acquisition-server).

---

## 1. What a montage is

A **montage** is a small JSON file that says, for every electrode, **which amplifier channel it is wired to**, what it is called, and where it sits on the head. In this workflow the same file is used in three places:

| Where | What the montage does there |
|---|---|
| **Charmeleon** (impedance check) | Shows each electrode's impedance at its place on the head map, under its name. |
| **Acquisition server** (TMSi and eego drivers) | Names the channels, and decides which EEG channels are recorded: an EEG channel the montage leaves out is not recorded. |
| **BrainVision writer (UG)** in Designer | Writes where each electrode sat into the recording's header (`[Coordinates]`), so that EEGLAB, MNE and BrainVision Analyzer load the layout with the data. |

So there is **one source of truth**. You name the electrodes once, in Charmeleon, and the impedance check, the recording and the analysis all agree on what "Cz" is.

---

## 2. Why montages differ per amplifier

The two amplifiers in the lab differ in one way that matters here: **who decides which electrode goes on which channel.**

### 2.1 TMSi Refa: tailor-made montages

On a TMSi Refa the electrodes plug into a **head box** with numbered sockets: the EXG inputs 1, 2, 3, ... **No rule ties a socket to an electrode position.** Whoever cables the cap decides that Fp1 goes into socket 7 and Cz into socket 16, and a different researcher, cap or study can cable it differently.

The amplifier therefore cannot know which electrode it is measuring. The montage is what tells it, so on TMSi **a montage is tailor-made**: one per way of cabling. You make it once and reuse it for as long as the cap is cabled the same way.

What that gives you:
- **Any cap, any layout.** 10/20, equidistant, a 32-electrode subset, a single strip: anything the head box has inputs for.
- **Any number of electrodes.** Leave sockets empty, or switch electrodes off in the montage. Only the EEG channels the montage names are recorded.
- **The other inputs are free too.** The Refa's bipolar (BIP) and auxiliary (AUX) inputs can carry EOG, ECG, EMG, a respiration belt or a GSR sensor; the montage names them as well (section 4.3).

The Refa's EEG inputs are measured against the **average of the connected inputs** (Charmeleon's `docs/tmsi.md` explains the principle). The montage's job is only to name them and decide which are recorded. It does not change that reference.

### 2.2 eego: semi-fixed montages

On an eego amplifier the **cap plugs into the amplifier through a fixed connector**. Each electrode of a waveguard cap is wired to one particular referential channel, so **the cap dictates the montage**: on a given cap, Fp1 is always the same EEG channel, whoever puts the cap on.

So on eego a montage is **semi-fixed**:
- **The EEG part mirrors the cap.** Make one montage per cap model, with each electrode on the channel the cap wires it to, and leave the EEG channel numbers alone after that: changing one does not move the electrode, it only gives a channel the wrong name. What you do change on eego is which electrodes are **in use**: switch off an electrode you do not want recorded.
- **The bipolar channels are yours.** eego's bipolar inputs (BIP1, BIP2, ...) are not part of the cap. What you plug into them (EOG, ECG, EMG, ...) is up to you, and the montage names them (section 4.3). The same goes for auxiliary (AUX) inputs, on amplifiers that have them.

### 2.3 In short

| | TMSi Refa | eego |
|---|---|---|
| Who decides which electrode is on which EEG channel | Whoever cables the cap | The cap |
| The montage's EEG part | Tailor-made, one per way of cabling | Fixed by the cap: take the cap's montage |
| What you change in the montage | Channel numbers, names, which electrodes are in use | Which electrodes are in use |
| Bipolar (BIP) and auxiliary (AUX) inputs | Free: name them in the montage | Free: name them in the montage |
| Impedances in Charmeleon | Every input, BIP and AUX included (to be confirmed on the amplifier) | EEG channels only; the eego SDK measures none on bipolar channels |

---

## 3. How the channels are numbered

A montage numbers channels **per kind, in the amplifier's own order**:

| Kind | Written as | Means |
|---|---|---|
| EEG | `HardwareChannel: n` | The n-th **EEG** channel: TMSi's n-th EXG input, eego's n-th referential channel. |
| Bipolar | `"Channel": "BIP1"` | The first bipolar channel, `BIP2` the second, ... |
| Auxiliary | `"Channel": "AUX1"` | The first auxiliary channel, ... |

**The same name means the same input on both amplifiers.** `BIP1` is the first bipolar input on a Refa and on an eego. These are also the greyed hints the acquisition server shows in its channel list.

> **Montages from before Charmeleon 2.2.** Some older montages put the four markers Left, Right, Top and Bottom on numbers above the EEG range (e.g. 72 to 75 on a 64-channel Refa), counting all of the device's channels. Such a number means nothing now: the server reports "Not recorded, as the amplifier has no such channel". Open the montage in Charmeleon, set those channels to `BIPn` or `AUXn` (section 4.3) and save it.

---

## 4. Making a montage in Charmeleon

### 4.1 Start from the right layout

| You use | Start from |
|---|---|
| A cap and cabling with a montage in Charmeleon's library (e.g. `ca-208.json`) | **Setup > Open Montage**, the library's file. **Save Montage As** under a name of your own, as the library is read-only. |
| A standard 10/20 cap | **Setup > Create Montage > 10/20 Layout** |
| An equidistant (ANT waveguard Duke) cap | **Setup > Create Montage > Equidistant Layout** |
| A montage you made before | **Setup > Open Montage** |

Charmeleon opens the amplifier it finds, or the one the acquisition server tells it to (section 5.4). Without an amplifier it starts in **demo mode**: everything works except the impedances.

### 4.2 Wire the EEG electrodes (TMSi)

On eego, skip this step: the cap decides it (section 2.2).

1. **View > View Channels.** Each circle shows its electrode's name, and the label beneath it shows the channel.
2. For each electrode, click the **channel label** beneath its circle, type the number of the head-box socket it is plugged into, and press Enter. Escape cancels.
3. **Click an electrode** to switch it off (grey) when it is not connected or not wanted. An electrode that is off is not recorded.

### 4.3 Add EOG, ECG and other channels

Channels that are not on the head (EOG, ECG, EMG, GSR, a photodiode, ...) are the **other channels**. They are drawn in a cluster to the right of the head.

- **Add one:** right-click anywhere and choose **Add Other Channel Here...**, or use **Setup > Add Other Channel...**. Give it a name (e.g. `HEOG`) and pick its channel: `BIP1`, `AUX2`, ... or a number for an EEG input. EOG electrodes plugged into EEG inputs are common on a Refa.
- **Rename it, or change its channel:** right-click it and choose **Rename...** or **Channel...**. This works for head electrodes too.
- **Move it:** drag it.
- **Remove it:** right-click it and choose **Remove**.
- In **View Channels**, its channel label can be clicked and typed into like any other: `BIP1`, `AUX2` or a number.

### 4.4 Check the montage

Charmeleon checks the montage as you edit it. A problem is **ringed in red** on the head map and summed up in the **status bar** at the bottom; click the status bar to see every problem.

| Problem | Why it matters |
|---|---|
| Two active electrodes on one channel | The recording can only have one of them; the server keeps the first in the file. |
| One name for two active electrodes | The recording would have two channels of that name. |
| A channel the amplifier does not have (e.g. EEG 73 on a 64-channel amplifier, or BIP9) | That electrode is not recorded. |
| Made for another amplifier | The montage was saved with an amplifier that has a different number of channels. |

### 4.5 Save it

- **Setup > Save Montage** (Ctrl+S) saves to the montage's own file.
- **Save Montage As...** (Ctrl+Shift+S) saves under another name.

The title bar shows the file, and an asterisk while there are unsaved changes. Charmeleon asks before closing with unsaved changes, except when the acquisition server started it (section 5.4).

Saving refuses a montage with two electrodes on one channel, and asks about the other problems first. Charmeleon writes the file in one go, so a reader never sees half a file. It records the amplifier it was saved with (not in demo mode), and keeps the electrodes in the order they were in.

---

## 5. Using a montage in the acquisition server

This applies to the **TMSi** and **eego** drivers.

### 5.1 Choose it

Set the driver's **Montage** setting to:
- **a path**, e.g. `D:\Montages\Refa_LabA.json`, with the folder button or typed in; or
- **just a name from Charmeleon's library**, e.g. `ca-208.json`. The server finds it in Charmeleon's installation (its `montages` folder, then its default layouts). A name like this works on every PC that has Charmeleon installed, unlike a path into one user's Desktop.

Choosing the same file again **reloads** it, e.g. after editing it in Charmeleon.

### 5.2 What you see

Under the driver settings the server says what the montage did, for example:

> ca-208.json: 31 EEG electrodes and 2 other channels.

| The note's colour | Means |
|---|---|
| Blue | All is well. |
| Orange | Something to look at: electrodes the amplifier has no channel for ("Not recorded: Left (EEG 73)"), a montage made for another amplifier, problems in the file, or a montage file that has gone (see 5.5). |
| Red | The montage can not be used ("Montage ... not found"), or Play failed and why. |

In the **Channel Names** list, the entries the montage decides are **greyed and read-only**. Hover over one to see why. That is every EEG channel, named or left out, plus the BIP and AUX channels the montage names. Change these in the montage, in Charmeleon. The other entries stay yours: type a name to record a BIP or AUX channel the montage does not name.

### 5.3 Without an amplifier

With no amplifier connected, the server can not know which of its channels are EEG. It shows the montage's names anyway, provisionally, and the note says so:
- **TMSi:** it assumes the layout of a Refa. The list's channels are EEG, except the last eight, which are BIP1–4 and AUX1–4, with their hints. A montage's BIP and AUX channels land there, and names you type for them are kept.
- **eego:** every entry counts as EEG, in channel order.

**Play** opens the amplifier and puts every name on the right channel, so the provisional list never ends up in a recording.

### 5.4 Editing a montage from the acquisition server

The **impedance button** starts Charmeleon:
- with the driver's montage;
- on **the driver's amplifier**: TMSi on the driver's protocol and device, or eego. Charmeleon then never opens another amplifier connected to the same PC. If it can not open that amplifier, it says so and starts in demo mode.

Edit the montage there as in section 4. Then either:
- **close Charmeleon**: it saves the changes to the montage's file without asking, and the driver reads the montage again at once; or
- **press Play**: the server closes Charmeleon, which saves on the way out, and the recording uses the montage as you left it.

If you used **Save Montage As**, the driver switches to the new file. A Charmeleon older than 2.2 does not report back; the driver then keeps its montage, and the server's log says so.

### 5.5 When the montage file is moved or deleted

Each time the server loads a montage it keeps a copy, in `%LOCALAPPDATA%\OpenViBE\acquisition-server\montages`. If the file is gone at the next session, the driver uses that copy, and the note turns orange to tell you. Restore the file, or choose another montage, to make the note go away.

---

## 6. Charmeleon's montage library

Charmeleon installs ready-made montages, read-only, in its `montages` folder (e.g. `C:\Program Files\Charmeleon\montages`):

| File | For |
|---|---|
| `1020Layout.json` | The 10/20 positions |
| `EquidistantLayout.json` | The equidistant (Duke) positions |
| `ca-208.json`, `ca-212.json`, `cy-401.json` | Examples: waveguard caps as cabled to a TMSi Refa in the lab |

Use them by name in the acquisition server (section 5.1), or open one in Charmeleon and save your own version under a new name. Check a library montage in Charmeleon before you rely on it: its status bar lists any problems.

---

## 7. Electrode positions in the recording

In Designer, set the **BrainVision Format file writer (UG)** box's **Montage (electrode positions)** setting to the same montage. The header (`.vhdr`) then gets:
- a `[Coordinates]` section, one line per channel: `Ch<n>=1,<theta>,<phi>`, on a unit sphere, in BrainVision's convention (theta negative on the left half of the head);
- a line under `[Comment]` naming the montage.

Channels are matched to electrodes **by name**. A channel the montage does not place on the head (EOG, ECG, ...) gets `0,0,0`, "no position".

Where the positions come from:
- **Exact,** when the montage has `Theta` and `Phi`. Charmeleon writes these for the equidistant layout, from the cap's 3-D coordinates.
- **From the head map** otherwise: Charmeleon's rings are 22.5 degrees apart, so ring 4 (Fpz, T7, Oz) lies on the equator, as in the 10-20 system. This is exact for the standard 10/20 positions and close for the positions between them.

Leave the setting empty to record no positions, as before.

---

## 8. The montage file format

A montage is a JSON object with one entry per electrode, plus (since Charmeleon 2.2) one describing the amplifier:

```json
{
  "Cz":    { "IsActive": true,  "LabelText": "Cz",   "HardwareChannel": 16, "Angle": 0,   "Radius": 0,  "X": null, "Y": null },
  "Fp1":   { "IsActive": true,  "LabelText": "Fp1",  "HardwareChannel": 7,  "Angle": 108, "Radius": 4,  "X": null, "Y": null },
  "Right": { "IsActive": true,  "LabelText": "HEOG", "HardwareChannel": 0,  "Angle": null, "Radius": null, "X": 0.85, "Y": 0.72, "Channel": "BIP1" },
  "Amplifier": { "Type": "TMSi Refa", "EEGChannels": 64, "BipolarChannels": 4, "AuxiliaryChannels": 4 }
}
```

| Field | Meaning |
|---|---|
| key (`"Cz"`, `"Right"`) | The electrode's position on the head map, or the other channel's key. |
| `LabelText` | The electrode's name: the channel's name in the recording. |
| `IsActive` | `false` leaves the electrode out: not recorded, no impedance shown. |
| `HardwareChannel` | The n-th EEG channel; 0 for none. |
| `Channel` | A BIP or AUX channel (`"BIP1"`, `"AUX2"`). It takes precedence over `HardwareChannel`, which Charmeleon then sets to 0 so that older readers leave the electrode out rather than put it on an EEG channel. *Since 2.2.* |
| `Angle`, `Radius` | Position on the head map: angle in degrees (0 = right, 90 = front), radius in rings (0 = vertex, 4 = equator, 5 = the rim). |
| `X`, `Y` | Where an other channel is drawn: fractions of the window's width and height. |
| `Theta`, `Phi` | The exact position on a spherical head, in degrees: theta from the vertex, phi as `Angle`. Optional. *Since 2.2.* |
| `Amplifier` | The amplifier the montage was saved with. The server warns when the connected one differs. *Since 2.2.* |

An electrode is **in use** when it is active and on a channel (`HardwareChannel` above 0, or a `Channel`).

---

## 9. Troubleshooting

| You see | Do this |
|---|---|
| The channel names stay empty, or show the old names | Is the Montage setting the file you think it is? The note under the settings says what the server read. Choose the file again to reload it. |
| "Not recorded, as the amplifier has no such channel: Left (EEG 73)" | An older montage, see section 3. Set the channel to `BIPn` or `AUXn` in Charmeleon. |
| "X is on EEG 10 as well as Y, so it is left out" | Two electrodes on one channel. Correct it in Charmeleon; its status bar lists every clash. |
| "Made for TMSi Refa with 64 EEG ... channels; this one has 32" | The montage belongs to another amplifier or cap. Choose the right one. |
| "the file is gone, so this is the copy kept from its last use" | Restore the file or choose another montage (section 5.5). |
| "Play failed: ..." in red | The note says why: e.g. no device connected, the device refuses access because another program (Charmeleon) holds it, or no channel has a name. |
| Charmeleon opens in demo mode from the impedance button | It could not open the driver's amplifier; its message says why. Is the amplifier on, and not in use by another program? |
| Edits in Charmeleon do not reach the server | Is it Charmeleon 2.2 or later? Check **About**. An older one does not report back; save there, then choose the montage file again in the server. |

---

## 10. For developers

- The montage rules live in **`CharmeleonMontage`**, in the acquisition server's driver library (`extras/applications/platform/acquisition-server/abstractions/driver/`). Both drivers call them, so a montage means the same to both. They are tested in `test/CharmeleonMontageTest.cpp`.
- A driver reports to the server through `AbstractDriver::note()`, shown under the settings, and `AbstractDriver::channelLocks()`, the read-only entries. It talks to Charmeleon through `charmeleonArguments()`, `charmeleonMontage()` and `takeOverCharmeleonMontage()`.
- Charmeleon's command line: `Charmeleon.exe [montage.json] [--save-on-close] [--result file] [--amplifier eego|tmsi] [--device name] [--protocol USB|WiFi|Network|Bluetooth]`. With `--result`, it writes `{"File": ..., "Saved": ..., "Unsaved": ...}` there on closing.
- Every place where this fork differs from upstream OpenViBE is marked `UG fork:` in the code. UG.md lists the differences.
