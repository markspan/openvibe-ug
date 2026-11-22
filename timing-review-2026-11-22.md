# Timing review, 22 November 2026

A read of every place in OpenViBE UG where a sample or a marker gets its time, for the lab's use: eego, TMSi and LSL sources, recorded to BrainVision (`.vhdr/.eeg/.vmrk`, through Designer's *BrainVision Format file writer (UG)*) or to XDF (the server's *LSL Output* into LabRecorder, or Designer's *LSL XDF writer (UG)*). Each finding says what a fix would take; findings 3 and 6 have been fixed since (UG.md 2.64). Earlier work this builds on: UG.md 2.5, 2.20, 2.21, 2.24, 2.25, 2.27, 2.30, and the lab measurements of 7 and 9 November (TMSi at 2048 Hz: hardware marker 1.87 ms after the LSL marker, SD 0.64 ms with raw marker times, 0.26 ms when each marker takes its EEG sample's dejittered time).

Read: the acquisition server's core (`Acquisition`, `CircularBuffer`, `DriftCalculator`, `AbstractDriver`, `BlockStamps.h`, `SteadyTrigger.h`, `AbstractWriter`, `NetWriter`), the TMSi, eego, LSL and TCP-tagging drivers, the LSL Output writer, the Acquisition client box (`SampleBlocks.h`, `StimulationDelay.h`), the BrainVision writer (UG), the LSL recorder and XDF writer, the LSL Export box, the player's clock and `System::Time`.

## How a time is made, in short

- **Samples (eego, TMSi):** each block from the SDK is stamped when `GetSamples`/`getData` returns; its last sample gets that moment, every earlier one the nominal period less per place (`BlockStamps.h`). The clock is `steady_clock` (QueryPerformanceCounter), in ns.
- **Hardware triggers:** dated at the stamp of the sample where the trigger line changed (`SteadyTrigger.h`), so in the data they sit on their own sample.
- **BrainVision:** has no times, only sample numbers. The writer places a marker by its Designer date, which the client derives from the sample it belongs to, so a hardware trigger lands on its own sample exactly (`BrainVisionUG::markerPosition()`, tested). Only hardware triggers are written.
- **XDF:** every sample and marker carries the server's stamp, moved to LSL's clock by an offset measured at start (`LSLOutputWriter.cpp:110`).

So within one BrainVision file, hardware triggers are exact to the sample. The problems are in the times themselves (XDF, and anything aligned by clock), in samples that go missing, and in markers that do not come from the amplifier.

## Findings, most important first

### 1. Sample stamps carry the jitter of the read loop (XDF; eego and TMSi)

`stampFromBlock()` (called at `CDriverTMSi.cpp:718`, `CDriverEEGO.cpp:618`) gives a block's last sample the moment the SDK call returned. The newest sample in a block was really acquired earlier: by the amplifier's and USB's latency, plus however long it waited for the next poll. The poll repeats after a 1 ms back-off (`Acquisition.cpp:605`), which on Windows lasts 1 ms only while something has set the timer to 1 ms (liblsl does when the LSL Output writer is loaded) and ~15.6 ms otherwise (finding 7). So every block's stamps are late by a varying amount, and the stamp of a sample jumps between blocks.

- In XDF, a loader's dejitter fits a line through the EEG stamps and removes this from the signal. It does not do so for the marker stream, which is irregular, so the hardware markers keep the raw jitter: the measured SD of 0.64 ms against 0.26 ms.
- Anything that dates by clock against these stamps (TCP tags, LSL markers via the server) inherits it.

**Fix:** stamp from a clock fit instead of the fetch time: keep a running fit of sample number against fetch time that follows the earliest fetches (the lower envelope: a fetch can be late, never early), and give every sample and trigger its time from that line. Unit-testable with simulated late fetches; no hardware needed to write it, the lab test to confirm it. This is the work put on hold on 7 November.

### 2. No allowance for the amplifier's own latency (XDF)

Even with finding 1 fixed, a stamp is the time the PC saw the sample, not when the electrode voltage was sampled: the hardware marker came 1.87 ms after the LSL marker (TMSi, 2048 Hz). eego has not been measured.

**Fix:** a per-driver "latency" setting (ms) subtracted from every stamp, filled in from the lab's sync measurement. One number per amplifier type and rate.

### 3. LSL as a source: the LSL timestamps are thrown away (LSL driver)

**Fixed in UG.md 2.64:** samples and markers keep their sender's times, markers 1–255 are triggers, the inlets recover, and a marker ahead of its samples is kept.

`CDriverLabStreamingLayer::read()` pulls samples without their timestamps (`CDriverLabStreamingLayer.cpp:291`, `nullptr`), so `AbstractDriver::getData()` stamps only the last sample of each pull with the moment of the pull and the server interpolates the rest between pulls; the first pull is interpolated back to the moment Play was pressed (`Acquisition.cpp:388`). Markers are pulled without timestamps too (`:304`) and dated when the server reads them (`:308`). So:

- samples and markers are dated by arrival: network and LSL buffering, the sender's chunking and the poll go into their times;
- a marker and its sample arrive by different paths, so they can be several ms apart, up to a sender's chunk;
- the inlets are opened with `recover = false` (`:250`, `:260`): a sender that restarts stops the data without a word.

And a functional point that matters for BrainVision: LSL marker values are passed on as the stimulation ID itself (marker 5 becomes stimulation 5), not as a hardware label (`OVTK_StimulationId_Label(5)`), and the BrainVision writer (UG) writes only labels. **LSL markers recorded through the server never reach a `.vmrk`.**

**Fix:** pull with timestamps, add `time_correction()`, and move them into the server's clock with the inverse of the LSL Output's offset; send marker values 1–255 as labels; open the inlets with recovery. All in one driver.

### 4. Lost samples leave no gap (BrainVision; eego and TMSi)

When the eego's sample counter skips (`CDriverEEGO.cpp:545`) or the TMSi SDK's buffer overflows (`CDriverTMSi.cpp:601`), the server logs it and counts it, but the recording just continues: the `.eeg` file closes the gap. Each hardware trigger stays on its own sample, but from there on the file's time axis is short by the samples lost, so aligning it with any other recording (an eye tracker, an XDF, video) is off by that much. A TMSi overflow loses a whole buffer (10 s).

**Fix:** have the drivers hand the number of missing samples to the server, which fills them (NaN in float files, or zeros) and sends a marker so the writer can write a BrainVision `New Segment` or a comment there. eego knows the count exactly from its counter; TMSi knows it from the overflow count.

### 5. The `.vmrk` start time is not the first sample's (BrainVision)

`Mk1=New Segment,,1,1,0,<date>` is written from the wall clock at the writer's first `process()`, to the second, with the microseconds padded with zeros (`ovpCBoxAlgorithmBrainampFileWriterUG.cpp:260`, `:272`). That is not when the first sample was taken, and only to a second. Analyzer, MNE and EEGLAB read it as the recording's start, so it is the only clock link between a BrainVision file and other recordings.

**Fix:** carry the first sample's server stamp to the writer (the client can put it in the stream, e.g. as a dimension label or with the impedances' column), turn it into wall-clock time with microseconds, and write that. Combine with finding 2's latency.

### 6. Markers sent from Designer through *LSL Export* get the wrong time

**Fixed in UG.md 2.64:** each time is LSL's clock less its age in the scenario. The *LSL Export (Gipsa)* box is unchanged.

`CBoxLSLExport.cpp:108` and `:196` take the wall-clock time at the box's first chunk as the scenario's time 0, and date every marker as that plus its scenario date. If the first chunk comes at scenario time 2.5 s, every marker is 2.5 s late; and scenario time runs in steps behind the wall clock (finding 7), so markers also jitter by up to a step. LSL Export is how a Designer-generated stimulus reaches LabRecorder.

**Fix:** subtract the player's current time when taking the start (`zgetTime() - getCurrentTime()`); better, for stimuli made in Designer, push them with `lsl::local_clock()` at the moment they are made. The *LSL Export (Gipsa)* box in contrib pushes its chunks without times (`ovpCBoxLSLExportGipsa.cpp:154`), so LSL dates them at the moment of sending, a Designer round or more after the fact; it puts the stimulations in a channel of the signal, which keeps them with their sample.

### 7. Windows timer resolution: 1 ms sleeps last ~16 ms

Neither the acquisition server nor Designer calls `timeBeginPeriod(1)`; only liblsl does, once loaded (memory note of 19 November; the legacy server did call it). Effects on timing: the server's poll interval and so the jitter of finding 1 (much larger in a session without LSL Output), and Designer's player rounds, which quantise every Designer-generated stimulus (finding 6, and finding 9's software stimulations).

**Fix:** `timeBeginPeriod(1)` at the start of both programs, `timeEndPeriod(1)` at exit. Deferred on 19 November; it is the cheapest item here.

### 8. `zgetTimeRaw()` scales microseconds slightly wrong (all OpenViBE times)

`ovCTime.cpp:79` converts the microseconds of a second to 32-bit fixed point with an integer division, `0xFFFFFFFF / 999999 = 4294` instead of 4294.97, so within each second a time runs up to 0.225 ms behind, a sawtooth reset at every whole second. A TCP tag's age is the difference of two such times, so a tag can be off by up to ±0.225 ms; player and box times share the error.

**Fix:** `(fraction << 32) / 1000000`. One line; a unit test can show the old error.

### 9. Smaller points

- **eego Stop can crash the reader:** `CDriverEEGO::stop()` (`:486`) frees the stream while the reader thread may still be inside `getData()`, as `Acquisition::stop()` calls `stop()` before it joins that thread. The TMSi and LSL drivers already free theirs in `unInit()` for this reason. Not timing, but it can end a recording badly at Stop.
- **Late TCP tags land on a wrong sample silently:** a tag that reaches the server more than 100 ms after its sample (`StimulationDelay.h:33`) is placed on the first sample still open instead. Worth a log line. Not in BrainVision (software markers are not written), but in other writers.
- **Stimulations beyond 128 pending** (`AbstractWriter.cpp:11`) are dropped without a word. Only in a burst of tags.
- **Software stimulations in Designer** are dated in scenario time, and the client's signal starts at the scenario time when the server's configuration arrived. A stimulus made in Designer and a sample of the same moment therefore differ by the acquisition's latency at that moment. TCP tagging avoids this, which is why the P300 speller uses it. Not in BrainVision.
- **Nominal rate in BrainVision:** the file's time axis is the amplifier's sample count at the nominal rate. An amplifier whose crystal is 50 ppm off drifts 0.18 s an hour against the PC and every other device. Markers in the same amplifier are unaffected; alignment with other recordings should use shared markers, not the start time and rate.
- `Acquisition::m_bRun` is a plain `bool` read by four threads; harmless in practice, but formally a data race (`std::atomic<bool>`).

## What is fine

- Hardware triggers sit on their own sample in every path: one-sample exact in BrainVision, and the same stamp as their sample in XDF (UG.md 2.5, 2.24, 2.25). A trigger shorter than two samples is lost by design.
- No sample is dropped between the server and Designer, whatever the block sizes (2.30), and the server blocks rather than drops when its buffers are full; samples are then lost only in the amplifier's own buffer, which is reported.
- TCP tags are dated by the sender's clock, not by arrival (2.20), and land on their own sample when they come within 100 ms.
- The LSL Output keeps every sample's own stamp (2.21); the LSL XDF writer records every sample with its stamp and the clock offsets every 5 s, as LabRecorder does.

## Suggested order

1. Finding 7 (`timeBeginPeriod`) and 8 (`zgetTimeRaw`): a few lines each.
2. Finding 3 (LSL driver timestamps and labels), if LSL sources go through the server.
3. Finding 1 with 2 (clock fit and latency setting), then the lab sync test again.
4. Finding 4 and 5 (gaps and start time in BrainVision files).
5. Finding 6 (LSL Export), if Designer sends markers to LabRecorder.
6. The eego Stop race.
