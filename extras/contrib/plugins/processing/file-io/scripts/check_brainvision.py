#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
# UG fork: new file, see UG.md 2.27.
"""Checks a recording of the BrainVision Format file writer (UG) for integrity.

Usage: python check_brainvision.py recording.vhdr [--expect-markers N] [--expect-samples N] [--max-gap-samples N]

What it checks:
  - the header: format, channel count and names, sampling interval, and the [Coordinates] section if there is one;
  - the data file: its size is a whole number of samples, and no value is NaN or infinite;
  - the markers: each lies within the data, and each stimulus "S<code>" sits on the sample where the recorded Trigger
    channel switches on that code; each switch on of the Trigger channel has its marker;
  - the SampleCounter channel, when recorded: it counts up by one step at every sample, so no sample went missing;
  - the units: EEG in microvolt, not volt.

It prints what it found, and ends with exit code 1 when something is wrong, 0 otherwise. Warnings do not fail.
"""

import argparse
import os
import re
import sys

import numpy as np

FORMATS = {"IEEE_FLOAT_32": np.float32, "INT_16": np.int16, "UINT_16": np.uint16}
STATUS_CHANNELS = ("Trigger", "SampleCounter")


class Report:
    def __init__(self):
        self.errors = []
        self.warnings = []

    def error(self, text):
        self.errors.append(text)
        print("ERROR   " + text)

    def warning(self, text):
        self.warnings.append(text)
        print("WARNING " + text)

    @staticmethod
    def info(text):
        print("        " + text)


class Sections(dict):
    """{section: {key: value}}, in file order."""

    def has_section(self, name):
        return name in self


def read_ini(path):
    """A BrainVision header or marker file: its first line, and its sections of key=value lines.

    Not configparser: the header's [Comment] section is free text, which BrainVision allows and an INI parser refuses.
    """
    with open(path, encoding="utf-8", errors="replace") as file:
        lines = file.read().splitlines()
    sections, current = Sections(), None
    for line in lines[1:]:
        if line.startswith(";") or not line.strip():
            continue
        if line.startswith("[") and line.rstrip().endswith("]"):
            current = sections.setdefault(line.strip()[1:-1], {})
        elif current is not None and "=" in line:
            key, value = line.split("=", 1)
            current[key.strip()] = value
    return lines[0] if lines else "", sections


def trigger_onsets(trigger, hold=2):
    """Where the Trigger channel switches to a new value and holds it for `hold` samples: {sample: value}.

    As the drivers decide (SteadyTrigger): a value caught for one sample while the lines switch is no switch, and the
    switch is dated at the first sample where the channel changed.
    """
    onsets = {}
    steady = int(trigger[0]) if len(trigger) else 0
    candidate, start, held = None, 0, 0
    for i, raw in enumerate(trigger):
        value = int(raw)
        if value == steady:
            candidate = None
            continue
        if candidate is None:
            candidate, start, held = value, i, 1
        elif value == candidate:
            held += 1
        else:
            candidate, held = value, 1
        if held >= hold:
            onsets[start] = (steady, candidate)
            steady, candidate = candidate, None
    return onsets


def check(vhdr_path, expect_markers=None, expect_samples=None, max_gap_samples=0):
    report = Report()
    folder = os.path.dirname(os.path.abspath(vhdr_path))

    # ---- header
    first_line, header = read_ini(vhdr_path)
    if not first_line.startswith("Brain Vision Data Exchange Header File"):
        report.error(f"{vhdr_path} does not start as a BrainVision header: {first_line!r}")
        return report
    common = header["Common Infos"]
    binary_format = header["Binary Infos"].get("BinaryFormat", "")
    if common.get("DataFormat") != "BINARY" or common.get("DataOrientation") != "MULTIPLEXED":
        report.error(f"Data are {common.get('DataFormat')} {common.get('DataOrientation')}; the writer writes BINARY MULTIPLEXED")
    if binary_format not in FORMATS:
        report.error(f"Unknown binary format {binary_format!r}")
        return report
    n_channels = int(common["NumberOfChannels"])
    interval_us = float(common["SamplingInterval"])
    rate = 1e6 / interval_us
    channel_infos = header.get("Channel Infos", {})
    names = []
    for i in range(1, n_channels + 1):
        entry = channel_infos.get(f"Ch{i}")
        if entry is None:
            report.error(f"Channel {i} of {n_channels} has no entry in [Channel Infos]")
            names.append(f"Ch{i}")
            continue
        fields = entry.split(",")
        names.append(fields[0].replace("\\1", ","))
        unit = fields[3] if len(fields) > 3 else ""
        if unit not in ("", "µV", "uV"):
            report.warning(f"Channel {names[-1]} is in {unit!r}, not microvolt")
    if len(channel_infos) != n_channels:
        report.error(f"[Channel Infos] has {len(channel_infos)} entries for {n_channels} channels")
    duplicates = sorted({name for name in names if names.count(name) > 1})
    if duplicates:
        report.error(f"Channel names used twice: {', '.join(duplicates)}")
    if header.has_section("Coordinates"):
        coordinates = header["Coordinates"]
        if len(coordinates) != n_channels:
            report.error(f"[Coordinates] has {len(coordinates)} entries for {n_channels} channels")
        placed = sum(1 for value in coordinates.values() if not value.startswith("0,"))
        report.info(f"Electrode positions for {placed} of the {n_channels} channels")
    report.info(f"{n_channels} channels at {rate:g} Hz, {binary_format}: {', '.join(names)}")

    # ---- data
    data_path = os.path.join(folder, common["DataFile"])
    dtype = np.dtype(FORMATS[binary_format]).newbyteorder("<")
    size = os.path.getsize(data_path)
    frame = n_channels * dtype.itemsize
    if size % frame != 0:
        report.error(f"{common['DataFile']} holds {size} bytes, not a whole number of samples of {frame} bytes: it was cut short")
    n_samples = size // frame
    data = np.fromfile(data_path, dtype=dtype, count=n_samples * n_channels).reshape(n_samples, n_channels).astype(np.float64)
    report.info(f"{n_samples} samples, {n_samples / rate:.3f} s")
    if expect_samples is not None and n_samples != expect_samples:
        report.error(f"{n_samples} samples, where {expect_samples} were expected")
    bad = ~np.isfinite(data)
    for c in np.nonzero(bad.any(axis=0))[0]:
        report.error(f"Channel {names[c]} has {int(bad[:, c].sum())} values that are not a number or infinite")

    eeg = [c for c, name in enumerate(names) if name not in STATUS_CHANNELS]
    for c in eeg:
        column = data[:, c]
        spread = float(np.percentile(column, 95) - np.percentile(column, 5)) if n_samples else 0.0
        if n_samples > 1 and spread == 0.0:
            report.warning(f"Channel {names[c]} is flat")
        elif 0 < spread < 1e-3:
            report.error(f"Channel {names[c]} varies by {spread:.3g} only: it looks like volt, where microvolt is declared")

    # ---- sample counter: every sample there
    if "SampleCounter" in names:
        counter = data[:, names.index("SampleCounter")]
        if n_samples > 2:
            steps = np.diff(counter)
            step = float(np.median(steps))
            gaps = np.nonzero(np.abs(steps - step) > 0.5 * abs(step))[0] if step else []
            if len(gaps):
                exact = float(np.max(np.abs(counter))) < 2 ** 24 or binary_format != "IEEE_FLOAT_32"
                lost = int(round(sum((steps[g] - step) / step for g in gaps))) if step else 0
                message = (f"The SampleCounter jumps {len(gaps)} times, first after sample {int(gaps[0]) + 1}: "
                           f"about {lost} samples lost")
                if exact and lost > max_gap_samples:
                    report.error(message)
                elif not exact:
                    report.warning(message + " (but the counter is beyond what a 32-bit float holds exactly)")
                else:
                    report.warning(message)
            else:
                report.info("SampleCounter: no sample missing")

    # ---- markers
    marker_path = os.path.join(folder, common["MarkerFile"])
    _, markers = read_ini(marker_path)
    stimuli = []
    for key, value in markers.get("Marker Infos", {}).items():
        fields = value.split(",")
        kind, description, position = fields[0], fields[1], int(fields[2])
        if not 1 <= position <= max(n_samples, 1):
            report.error(f"{key} ({kind} {description}) lies at sample {position}, outside the {n_samples} samples")
        if kind == "Stimulus":
            match = re.fullmatch(r"S\s*(\d+)", description)
            if not match:
                report.error(f"{key}: stimulus {description!r} is not named as BrainVision Recorder names them")
                continue
            stimuli.append((position, int(match.group(1)), key))
    report.info(f"{len(stimuli)} stimulus markers")
    if expect_markers is not None and len(stimuli) != expect_markers:
        report.error(f"{len(stimuli)} stimulus markers, where {expect_markers} were expected")
    positions = [p for p, _, _ in stimuli]
    if positions != sorted(positions):
        report.warning("The stimulus markers are not in the order of their samples")

    # ---- markers against the Trigger channel
    if "Trigger" in names and n_samples:
        trigger = data[:, names.index("Trigger")]
        onsets = trigger_onsets(trigger)
        matched = set()
        for position, code, key in stimuli:
            sample = position - 1
            onset = onsets.get(sample)
            if onset is None:
                near = [s for s in onsets if abs(s - sample) <= 3]
                where = f"; the Trigger channel switches at sample {near[0] + 1}" if near else ""
                report.error(f"{key} S{code} at sample {position}: the Trigger channel does not switch there{where}")
                continue
            previous, value = onset
            # TMSi sends the value, eego the lines that came on
            if code != (value & 0xFF) and code != ((value & ~previous) & 0xFF):
                report.error(f"{key} S{code} at sample {position}: the Trigger channel switches to {value} there")
                continue
            matched.add(sample)
        unmarked = [s for s, (previous, value) in onsets.items() if s not in matched and (value & ~previous) & 0xFF]
        if unmarked:
            report.error(f"The Trigger channel switches on {len(unmarked)} times without a marker, first at sample {unmarked[0] + 1}")
        elif stimuli:
            report.info("Every marker sits on its Trigger channel switch, and every switch has its marker")

    print(f"{len(report.errors)} errors, {len(report.warnings)} warnings")
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("vhdr")
    parser.add_argument("--expect-markers", type=int)
    parser.add_argument("--expect-samples", type=int)
    parser.add_argument("--max-gap-samples", type=int, default=0, help="lost samples to accept before it is an error")
    args = parser.parse_args()
    report = check(args.vhdr, args.expect_markers, args.expect_samples, args.max_gap_samples)
    sys.exit(1 if report.errors else 0)


if __name__ == "__main__":
    main()
