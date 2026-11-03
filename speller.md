# The P300 speller in the UG-lab EEG workflow

A how-to for the P300 speller in the UG fork of OpenViBE: what the task is, how to set it up and run it, what the port of its display to Qt changed, and the setup with Python scripts next to the original Lua one.

**You need:**
- the UG fork of OpenViBE (this repository), built from the commit that added this file or later;
- an amplifier with a working driver and montage, see [montages.md](montages.md);
- one PC for Designer and the acquisition server together, see [Set up the PC and the server](#22-set-up-the-pc-and-the-server);
- a separate monitor for the participant, as the display opens in a window of its own.

**State of the port.** The display and its markers ran in Designer without an amplifier. A full session has not been run yet: recording, training and online spelling with an amplifier are untested. See [What has been tested](#35-what-has-been-tested).

---

## 1. The task

### 1.1 The idea

The participant looks at a 6 × 6 grid of letters and digits, while rows and columns flash one at a time in random order. They pay attention to one letter and silently count how often it flashes. Only 2 of the 12 rows and columns contain that letter, so its flashes are rare, attended events. Those evoke a **P300**, a positive deflection roughly 300 ms after the event, mostly over central and parietal sites.

A classifier learns to tell flashes with a P300 from those without. The row and the column that collect the most P300 votes point to the letter. This is the classic matrix speller of Farwell & Donchin (1988).

```
A B C D E F     row labels:     Label_01 ... Label_06  (top to bottom)
G H I J K L     column labels:  Label_07 ... Label_0C  (left to right)
M N O P Q R
S T U V W X
Y Z 1 2 3 4
5 6 7 8 9 0
```

### 1.2 One session

There are three levels, each with its own setting in the **P300 Speller Stimulator** box. With the demo's values:

| Level | What happens | Demo value |
|---|---|---|
| **Flash** | One row or column lights up, then a pause | 200 ms on, 100 ms off |
| **Repetition** | All 12 rows and columns once, in a new random order, alternating column and row, then a pause | 12 flashes, 3.6 s, then 1 s rest |
| **Trial** (one letter) | A target letter, then a number of repetitions | 12 repetitions, 55.2 s |
| **Session** | A number of trials, with a rest between them | 10 trials, 3 s rest |

A session goes like this:

1. Press Play. Nothing happens for **20 s**, time to settle. The **Launch** box then starts the stimulator.
2. A trial's **rest** starts. The grid is dim.
3. **2 s** into the rest, the target letter turns blue, and its letter is added to the *Target* line under the grid. In online use, the letter found in the previous trial is green until now.
4. **1 s** later the flashing starts. The target's blue goes at the first flash, so the participant has to remember the letter.
5. After the 12 repetitions, the next rest starts. Online, the detected letter now turns green and appears on the *Result* line.
6. After the last trial, the experiment ends **3 s** later.

**What that adds up to, with the demo's values:**

| | |
|---|---|
| Duration of a session | about 10 minutes (23 s + 10 × 55.2 s + 9 × 3 s, and the 3 s at the end) |
| Flashes per letter | 144 |
| Flashes in a session | 1,440: 240 of the target's row or column, 1,200 others |
| Target flashes per letter | 24 |

### 1.3 What to tell the participant

General practice for this kind of task, not something the software checks:

- Look at the blue letter and keep looking at it until the flashing has ended. Count how often it flashes.
- Try not to blink or move during the flashing. The 3 s between letters are the time for that.
- Sit still, jaw relaxed. Say so before Play, as there is nothing to read on the screen.

### 1.4 The markers in the data

The display box sends these to the server, which puts them in the recording's stimulation stream. Training and online use depend on them.

| Marker | When | Used for |
|---|---|---|
| `ExperimentStart`, `ExperimentStop` | Begin and end | `ExperimentStop` ends the scenario and starts the training |
| `RestStart`, `RestStop` | Rest between trials | |
| `TrialStart`, `TrialStop` | Begin and end of a letter | Clearing and counting the votes |
| `SegmentStart`, `SegmentStop` | Begin and end of a repetition | |
| `Label_01` … `Label_0C` | Which row or column flashes. **Twice between `RestStart` and `RestStop`: the target's row and column** | Knowing which flash a vote is for |
| `Target`, `NonTarget` | With each flash: whether it holds the target | Training classes |
| `VisualStimulationStart` | Each flash on | Online epochs |
| `VisualStimulationStop` | Each flash off | Returning the grid to dim |

---

## 2. Setting up and running

### 2.1 Which set of scenarios

The scenarios are in `build/share/openvibe/scenarios/bci-examples/`, after a build. There are two sets, each in a Lua and a [Python](#4-the-python-setup) version:

| | `p300-speller/` | `p300-speller-xDAWN/` |
|---|---|---|
| **Channels** | `Cz`, `O1` and `O2`, selected by name | All, reduced to 3 by a trained spatial filter |
| **Montage** | Must name those three channels, or it has nothing to work on | Any |
| **Training** | Classifier only | Spatial filter, then classifier |
| **Choose it for** | Few electrodes, a first try | A full cap |

Otherwise the two work alike:

| Step | `p300-speller/` | `p300-speller-xDAWN/` | Needs the amplifier? |
|---|---|---|---|
| Check the signal | `p300-speller-0-signal-monitoring` | `p300-xdawn-0-signal-monitoring` | Yes |
| Record training data | `p300-speller-1-acquisition` | `p300-xdawn-1-acquisition` | Yes |
| Train the spatial filter | | `p300-xdawn-2-train-xDAWN` | No |
| Train the classifier | `p300-speller-2-train-classifier` | `p300-xdawn-3-train-classifier` | No |
| Spell | `p300-speller-3-online` | `p300-xdawn-4-online` | Yes |
| Replay a recording | `p300-speller-4-replay` | `p300-xdawn-5-replay` | No |

The Python versions are in `p300-speller-python/` and `p300-speller-xDAWN-python/`, and work the same way. Their scenario names start with `p300-python-` and `p300-xdawn-python-`. This guide names the Lua versions.

**Signal processing, in all of them.** The signal goes through a 1–20 Hz band pass (order 4), then decimation by 4. Each flash gets an epoch of 0.6 s from its start. The epoch's samples are the features for a linear discriminant classifier (LDA), with 5-fold cross-validation during training. The classifier judges each flash on its own, without averaging over repetitions.

**Sampling rate.** It must be a multiple of 4, or the decimation stops with an error. At 512 Hz the classifier gets about 77 samples per channel for each flash, so about 230 features with three channels. At 1,000 Hz it is about 450. There are 240 target flashes to train on, so a higher rate does not help here.

### 2.2 Set up the PC and the server

1. **Run Designer and the acquisition server on the same PC.** The display box sends its markers to `localhost`, port 15361, and this can't be changed in the box.
2. **Leave the server's TCP Tagging port at 15361.** It's the default, and the server shows it among its settings. If the box can't connect, the log says `Unable to connect to AS TCP Tagging, stimuli wont be forwarded`. The display then still runs, **but the recording has no markers and can't be used for training.**
3. **Choose the driver and the montage in the server.** Check that it names the channels you need, see [montages.md](montages.md).
4. **Keep hardware triggers out of it.** Disconnect any trigger line from the amplifier during the speller. The amplifier's triggers arrive as `Label(1)` to `Label(255)`, the same codes as the speller's rows and columns. During a trial, a trigger would be counted as a flash and shift the votes.
5. **Set the server's connection port to 1024.** The Acquisition client box in all the speller scenarios, Lua and Python, connects on 1024, and the server's default is 8088. Otherwise, the log says `Could not connect to server localhost:1024`. The server remembers the setting. You can also change the **Acquisition server port** setting of the box, in each scenario.
6. Press **Connect** and then **Play** in the server, before Play in Designer.

### 2.3 Set up the display

1. Open the scenario in Designer.
2. On the **Configuration** tab, in the **Window Manager**, drag the **P300 Speller Visualization** box into the composer. **Do this every time you open the scenario.** The layout isn't saved with it, and without it no window opens. This holds for every visualization box in this Designer.
3. Press Play. The grid opens in a window called *Window Manager*. Move it to the participant's monitor and maximize it within the first 20 s.

Right-click the grid to show or hide the *Target* and *Result* lines, for instance to hide the result from the participant. They are shown by default.

**The look.** The demo's colors and font sizes are in `p300-speller-visualization.cfg`, next to the scenarios, and apply to all of them. Colors are percentages of red, green and blue. Sizes are in points, and a small window scales all of them down alike.

| | Background | Letter | Size |
|---|---|---|---|
| Flash | `5,5,5` | `100,100,100` (white) | 60 |
| No flash | `0,0,0` | `25,25,25` (dim) | 40 |
| Target | `20,20,70` (blue) | `10,10,30` | 40 |
| Selected | `10,40,10` (green) | `60,100,60` | 40 |

### 2.4 Run a session

**Step 0. Check the signal.** Run the `0-signal-monitoring` scenario, with its Signal display in the Window Manager. The signals should show eye blinks and jaw clenches, and alpha waves when the eyes close.

**Step 1. Record training data.** Run the `1-acquisition` scenario. It stops by itself after about 10 minutes, and the recording is in `signals/`, next to the scenario, as `p300-train-[date-time].ov`.

**Step 2. Train.** In the `2-train-classifier` scenario, or in the xDAWN set `2-train-xDAWN` and then `3-train-classifier`:

1. Point the **Generic stream reader** box to your recording. It reads `signals/CHANGE_ME.ov` until you do.
2. Press Fast Forward. The scenario stops by itself when the training is done.
3. Read the classifier's cross-validated accuracy in the log. **Below 70%, record again.** This is the demo's advice. The chance level is not 50% for this data, as only one flash in six is a target, so also look at how it does on each class.

The result is `p300-classifier.cfg`, next to the scenario, and for xDAWN also `p300-spatial-filter.cfg`. **A new training overwrites them**, so copy them if you want to keep them per participant.

**Step 3. Spell.** Run the `3-online` scenario (xDAWN: `4-online`). It pauses at the end, and doesn't stop as the acquisition does. It records too, as `p300-online-[date-time].ov`, or for xDAWN `p300-xdawn-online-[date-time].ov`.

The **P300 accumulator** box works out each letter. Every flash that the classifier calls a target gives its row or column one vote. After the trial's last prediction, the row and the column with the most votes make up the letter. When no flash at all got a vote it logs `Classifier predicted 'no p300' for all flashes of the trial`, and the first row and column win, which is the letter `A`.

The *Result* line shows each letter by how it compares to the target:

| Color | Meaning |
|---|---|
| Green | Correct |
| Orange | Right row or right column |
| Red | Neither |
| No color | There was no target to compare with |

**Step 4 (optional). Replay.** The `4-replay` scenario (xDAWN: `5-replay`) shows an online recording again, with the classifier, without an amplifier. Point its **Generic stream reader** to the file.

### 2.5 Changing the task

**A fixed word.** Set **Word to Spell** in the **Target Letter Generation** box. Three things to watch:
- In the Lua scripts, use lower case: `openvibe`, not `OPENVIBE`. The script looks up letters in a lower-case grid of its own, and a letter it can't find gives a wrong target without an error. The Python script takes both, and warns about a letter it can't find.
- Set **Number of trials** in the **P300 Speller Stimulator** to the number of letters. If they differ, the recording can't be used for training.
- Empty means random letters.

**Free spelling.** Disconnect the **Target Letter Generation** box in the online scenario. The participant then picks their own letters, and the result has no color. The recording can't then be used for training.

**Timing.** The flash and pause durations, repetitions and trials are settings of the **P300 Speller Stimulator** box. Keep them the same in the acquisition and online scenarios, and remember the screen's refresh: at 60 Hz, one frame is 16.7 ms, and a flash lasts a whole number of frames.

**A different grid.** Copy `p300-speller.ui` from `build/share/openvibe/plugins/simple-visualization/`, and point the display's **Interface filename** setting to your copy. The display reads the table named `p300-speller-table` from it: `n_rows`, `n_columns`, and each cell's label and position.

If you change the size, also change the rows and columns in the stimulator. If you change the letters, also change the grid at the top of `p300-speller-target.lua` or `p300-speller-target.py`, which each keep their own copy.

### 2.6 Troubleshooting

| What you see | Likely cause |
|---|---|
| No window opens on Play | The box isn't in the Window Manager, see [2.3](#23-set-up-the-display) |
| `Could not connect to server localhost:1024` in the log | The server isn't playing, or its connection port isn't 1024, see [2.2](#22-set-up-the-pc-and-the-server) |
| `Unable to connect to AS TCP Tagging` in the log | Server not running, or its TCP Tagging port isn't 15361 |
| Training finds no targets, or fails | No markers in the recording: the connection above, or a recording made without the server |
| Decimation error at Play | The sampling rate isn't a multiple of 4 |
| `No frame drawn for the speller` in the log | The window is hidden or covered completely: markers are then sent without waiting for the screen |
| Every letter comes out as `A`, with `Classifier predicted 'no p300'` | Poor training data, or hardware triggers in the stream |
| Votes look random although training went well | Hardware triggers in the stream, see [2.2](#22-set-up-the-pc-and-the-server) |

---

## 3. The port to Qt

### 3.1 What was wrong

The display box was GTK code, which upstream's Qt version of Designer left out of the build. Six of the eleven speller scenarios use it, so neither recording nor spelling was possible. The Identifier Card and Magic Card displays have the same problem and are **still not ported**.

### 3.2 What was kept, and what changed

**Kept:**
- The box's ID, inputs, output and settings, so that the scenarios open as they are.
- The stimulation logic: which marker is sent when, and how flashes, targets and selections change the grid.
- The letter grid from the **Interface filename** setting, so that a lab's own `.ui` file still works.

**Changed:**

| | GTK | Qt |
|---|---|---|
| Grid and text | GTK window | `P300SpellerVisualization.qml` |
| Showing target and result | Toolbar buttons | Right-click menu, to keep the stimulus screen clean |
| Text under the grid | Theme's background, dark green, orange and red | Dark bar, with colors that read on it |
| Small window | Fonts keep their size | All scale down alike, so that a flash stays larger than no flash |
| Moment the markers are sent | Next idle moment of GTK, assumed to follow the drawing | After the frame that shows the change, with that frame's time, see below |
| Selected column check | Compared against the number of rows | Against the number of columns. It only matters for a grid that isn't square |

**The code:**

| File, in `extras/plugins/processing/simple-visualization/` | What it holds |
|---|---|
| `src/box-algorithms/CBoxAlgorithmP300SpellerVisualization.cpp`, `.hpp` | The box: decoding the stimulations, sending the markers |
| `src/box-algorithms/P300SpellerState.cpp`, `.h` | Without Qt or the player: reading the grid file, what is on display, and when a frame showed it |
| `src/box-algorithms/CP300SpellerView.cpp`, `.h` | The QML item, and the connection to its window's frames |
| `qml/P300SpellerVisualization.qml` | The grid and the text |
| `test/P300SpellerStateTest.cpp` | Unit tests (`test-p300-speller`) |
| `test/scenarios-tests/P300-speller-test.xml` | The stimulator and the display without an amplifier, spelling `ug` (`run_P300-speller`) |

### 3.3 Timing

Timing is what makes or breaks a P300 experiment, so here is what happens between a flash being due and its marker being in the data:

```
Stimulator    flash is due, at the player's 128 Hz clock
    |         the player only runs in rounds, with a 1 ms sleep between them
Display box   writes the change to the QML item, and holds the markers
    |
Qt            first frame after that write is swapped to the screen (time noted)
    |
Display box   sends the markers at its next clock tick, at most 1/128 s later
    |         TCP, on this PC
Server        reads waiting markers between reads from the amplifier, and dates them with the frame's time
    |         a marker waits here for the chunk that holds its sample
Client box    puts each marker on the sample stamped at or before that date
```

**The player's sleep.** Designer's player ran in rounds with a 50 ms sleep between them, so a flash could only start at the start of a round, about every 62 ms. That is now **1 ms**, in `PlayerBridge::looping()`. This changes the timing for all scenarios, not just the speller.

Measured on the development PC with a 60 Hz screen, with a real window, over one short run of 48 flashes. The demo asks for 200 ms on and 300 ms from flash to flash:

| | 50 ms sleep (upstream) | 1 ms sleep (now) |
|---|---|---|
| Flash on | 160–266 ms | 187–216 ms |
| Flash to flash | 269–352 ms | 283–315 ms |
| Spread of the time from frame to marker reaching the server | up to 57 ms | up to 11 ms |
| CPU of Designer, in % of one core, without a window | 1.8–3.3% | 2.1–4.0% |

With the 1 ms sleep, the flash timing is within about one frame of what was asked for.

**What a marker's time means.** The display box notes when the frame was swapped, and sends that time with the marker. The server dates the marker with it (`tagDate()` in `TagDate.h`), by subtracting the tag's age from the moment it reads it. So the time it took the marker to get to the server doesn't count, nor does the wait for the server's next read. Upstream ignored the tag's time, and dated a marker when it read it.

Two things then decide which sample a marker lands on, as the Acquisition client box puts a marker on a sample in the chunk it comes with:

| The marker reaches the server | What happens |
|---|---|
| **Before its sample**, which is on its way from the amplifier | It waits for the chunk that holds its sample, in `AbstractWriter`, and lands on the sample stamped at or before its date. Upstream sent it with the next chunk, whichever that was. |
| **After its sample's chunk has gone** | It lands on the first sample of the next chunk: late, by at most the marker's age. It can't go back, as boxes such as the Lua and Python filters drop a stimulation dated before the chunk they last sent. |

A marker reached the server about 11 ms after its frame at the median (1.4�12.6 ms), in one run through the real server. Samples from an amplifier that come in blocks will often take longer than that, which makes the second case rare, but this has not been measured with TMSi or eego.

**Other programs.** This holds for every program that sends TCP tags. One that stamps its tags when sending them is now dated at the moment it sends, and not on arrival. A tag whose time can't be from this PC, from another PC's clock for instance, is dated on arrival, as before.

**The screen.** A buffer swap isn't light from the screen, as the monitor adds a delay of its own. Measure the delay from flash to marker once per setup with a photodiode on the grid, and compare it with the markers in a recording. This is the check that counts.

### 3.4 How the box is built

- **Threads.** The box runs on the player's thread, and the QML item belongs to the GUI thread. The box therefore posts each change to the item, as `CDisplayCueImage` does, and never touches QML properties itself.
- **Knowing a frame showed it.** A frame that is already being rendered when the change is written doesn't show it. So a change counts as displayed at the first buffer swap after a synchronization that took place after the write (`CP300FrameTracker`).
- **Markers.** Those of one round wait for that round's change. If a round changes nothing and nothing is waiting, its markers go at once, as there'd be no frame to wait for. Otherwise they wait behind what is being drawn, so that they stay in order.
- **No frame.** A hidden window draws nothing. Without a window to draw in, a change counts as displayed right away. If frames stop coming, markers are released after 1 s with a warning, as a late marker beats a lost one.

### 3.5 What has been tested

**Tested on the development PC, without an amplifier:**
- `test-p300-speller`, 12 unit tests: the grid file, the display's state, and the frame tracking.
- `test-acquisition-server-core`, with 7 new tests (`TcpTagTimingTest.cpp`): a tag's date from its age, tags with a time that means nothing here, a marker waiting for its sample, and triggers staying with their own sample.
- The speller, in a real window, with the real server's tag service reading the markers. The server read them all, about 11 ms old.
- `run_P300-speller`: the scenario runs without errors, with no window. `run_Display-cue-image`, `run_ERP-plot`, `run_Multimodal-Graz-visualization` and `run_Simple-visualization` still pass with the 1 ms sleep.
- In a real window, once, using test code that placed the box in the Window Manager and has since been removed:
  - images of the grid, showing a flashing row, and `ug` on both text lines, in green;
  - 214 markers at a program standing in for the TCP Tagging port, in order, with 8 `Target` and 40 `NonTarget`.

**Not tested:**
- **A marker's way from the server to a recording.** The test code could start the real server's oscillator and its tag service, but Designer got no samples from it that way, so no marker has been seen on a sample from the real server. The step is covered by the unit tests only. This is the first thing to check in the lab: a recording of the `1-acquisition` scenario should have its `Target` and `NonTarget` markers, 240 and 1,200.
- Any step with an amplifier, or training on real data.
- Dragging the box into the Window Manager by hand, and the right-click menu.
- Showing the grid on a second monitor.
- A normal recording with the 1 ms sleep, which is worth doing, as it applies to all scenarios.
- The light from the screen, against a photodiode.

---

## 4. The Python setup

Next to the Lua scenarios there is a complete set that uses the **Python 3 scripting** box. The Lua scenarios and scripts are unchanged, apart from the repair in 4.1.

### 4.1 Choosing between them

Both sets run the same task, and in the tests below their scripts send the same stimulations. Choose by which language you want to change the scripts in. Python's standard library and packages are the main reason to pick it, for instance to add your own analysis to a box.

**A repair to the Lua scenarios.** Three of their scripts, `p300-speller-accumulator.lua`, `p300-speller-filter-flash.lua` and `p300-speller-filter-target.lua`, compare against `OVTK_StimulationId_LabelEnd`, which upstream's `lua-stimulator-stim-codes.lua` didn't define. Lua stopped with `attempt to compare number with nil`, and Designer stopped the scenario: online at the first flash, as seen in Designer. The replay scenarios use the same comparison. The codes file now has the constant, as does Python's `StimulationsCodes.py`. The scripts themselves are unchanged.

### 4.2 What is where

| Where | What |
|---|---|
| `extras/applications/demos/p300/bci-examples/p300-speller-python/` | The classic set's scenarios, and the five scripts |
| `extras/applications/demos/p300/bci-examples/p300-speller-xDAWN-python/` | The xDAWN set's scenarios. They use the scripts above |
| `extras/contrib/plugins/processing/python3/share/ovstimulationbox.py` | `OVStimulationBox`, the base class of the scripts |
| `extras/plugins/processing/simple-visualization/test/scenarios-tests/P300-speller-python-test.xml`, `P300-speller-lua-test.xml`, `P300-speller-python-check.py` | The tests (`run_P300-speller-python`, `run_P300-speller-lua`) |

Both Python sets take their look and epoch settings from `p300-speller/p300-speller-visualization.cfg` and `p300-speller/p300-epoch-average.cfg`, so there is one copy to edit. Each set has its own `signals/` folder, classifier and spatial filter. A classifier trained in a Lua scenario can't be found by a Python one until you copy it over.

**Edit the scripts in `extras/`, not in `build/`.** A build copies them over the ones in `build/share/`.

**Nothing to install.** The build brings its own Python (3.10, in `build/bin/python_runtime/`), and the scripts only use its standard library.

### 4.3 The scripts

Each Lua script has a Python script of the same name, with the same settings:

| Script | Box | What it does |
|---|---|---|
| `p300-speller-launch.py` | Launch | Starts the flashing at the given time |
| `p300-speller-target.py` | Target Letter Generation | Chooses each trial's target |
| `p300-speller-accumulator.py` | P300 accumulator | Works out each letter from the classifier's predictions |
| `p300-speller-filter-flash.py` | Pick flash | In a replay, picks the flashes from the recording's stream |
| `p300-speller-filter-target.py` | Pick target | In a replay, picks the targets from the recording's stream |

The Lua Stimulator box does a lot for its scripts, which the Python box doesn't do: holding stimulations with a future date, keeping them in date order, and filling in chunk times. `OVStimulationBox` adds that, for any script. A script says what happens in `setup()` and `step()`:

```python
from ovstimulationbox import OVStimulationBox, stimulation_code

class MyOVBox(OVStimulationBox):
	def setup(self):
		self.start = stimulation_code(self.setting['Start stimulation'])

	def step(self):
		for stimulation in self.take_stimulations(0):
			if stimulation.identifier == self.start:
				self.send_stimulation(0, self.start + 1, self.getCurrentTime() + 2)

box = MyOVBox()
```

| Lua | Python |
|---|---|
| `box:get_setting(2)` | `self.setting['Name of the setting']` |
| `box:get_stimulation(1, 1)`, `box:remove_stimulation(1, 1)` | `self.take_stimulations(0)`: all waiting, oldest first |
| `box:send_stimulation(1, id, date, 0)` | `self.send_stimulation(0, id, date)`. Inputs and outputs count from 0 |
| `box:set_filter_mode(1)` | `filter_mode = True`, in the class |
| `box:log("Warning", text)` | `print('Warning: ...')`, which reaches the log as information |
| `while box:keep_processing() do ... box:sleep() end` | `step()` is called on each tick |

**All Python boxes in a scenario share one namespace.** Keep a script's values in its class, not at the top level of the file, or scripts overwrite each other's.

### 4.4 Where the Python scripts differ

On purpose, in cases where the Lua script goes wrong:

| Case | Lua | Python |
|---|---|---|
| Capitals in the word | Wrong target, no message | Taken as lower case |
| A letter not in the grid, or the word has ended | Wrong target, no message | No target, and a warning |
| A prediction without a flash | Script stops with an error | Warning, and it carries on. Votes in that trial are not to be trusted |

### 4.5 What has been tested

**Tested on the development PC, without an amplifier:**
- **Each script against its Lua original.** The same made-up stimulations went into each, outside Designer. All 21 comparisons sent the same stimulations with the same dates: the launch, and for 5 sets of input the target, both filters and the accumulator.
- **The launch, target and accumulator scripts in Designer**, in both languages (`run_P300-speller-python`, `run_P300-speller-lua`). The display's own flags stand in for the classifier, so its votes are perfect. A test passes only if the selected letters equal the targets, which it reports at the end. The Python one sent the expected 214 markers to a stand-in for the TCP Tagging port.
- **The repair.** Before it, the Lua test scenario stopped at the first flash. After it, the scenario runs to the end, with the letters right.
- **Loading.** The six Python scenarios that use scripts start without a script error, as far as they get without a server or a trained classifier. Designer asks for no box updates. The other four, for monitoring and training, were not started.

**Not tested:**
- Everything in [3.5](#35-what-has-been-tested) under *Not tested* applies here too.
- The two filter scripts inside Designer, in either language, as they need a real recording. They've only had the comparison above.
- Flash timing with the Python scripts in a real window. The scripts run on the player's thread, between the flashes. The CPU use is below.
- Pressing Play a second time without closing Designer.

**CPU**, in % of one core, for Designer without a window, over 12 s of the test scenarios, three runs each: Lua 0.5�2.0%, Python 0.7�2.3%. That's no measurable difference, on so few runs.

---

## References

- L. A. Farwell and E. Donchin (1988). Talking off the top of your head: toward a mental prosthesis utilizing event-related brain potentials. *Electroencephalography and Clinical Neurophysiology*, 70(6), 510–523.
- B. Rivet, A. Souloumiac, V. Attina and G. Gibert (2009). xDAWN algorithm to enhance evoked potentials: application to brain-computer interface. *IEEE Transactions on Biomedical Engineering*, 56(8), 2035–2043.
- The box documentation in Designer: select a box and press F1.
