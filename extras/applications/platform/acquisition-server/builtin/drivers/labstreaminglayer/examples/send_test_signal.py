#!/usr/bin/env python3
"""
Pushes a 4-channel sinusoidal signal to LSL for testing the LabStreamingLayer
acquisition-server driver.

Requires pylsl: pip install pylsl
"""
import math
import time

from pylsl import StreamInfo, StreamOutlet

CHANNEL_COUNT = 4
SAMPLING_RATE = 250.0
CHANNEL_NAMES = [f"Sin {i + 1}" for i in range(CHANNEL_COUNT)]

info = StreamInfo("LSL-test-stream", "EEG", CHANNEL_COUNT, SAMPLING_RATE, "float32", "lsl-test-stream-id")

channels = info.desc().append_child("channels")
for name in CHANNEL_NAMES:
    channels.append_child("channel") \
        .append_child_value("label", name) \
        .append_child_value("unit", "microvolts") \
        .append_child_value("type", "EEG")

outlet = StreamOutlet(info)

print(f"Streaming '{info.name()}': {CHANNEL_COUNT} channels at {SAMPLING_RATE} Hz. Press Ctrl+C to stop.")

period = 1.0 / SAMPLING_RATE
sampleIdx = 0
try:
    while True:
        t = sampleIdx / SAMPLING_RATE
        sample = [math.sin(2 * math.pi * (ch + 1) * t) for ch in range(CHANNEL_COUNT)]
        outlet.push_sample(sample)
        sampleIdx += 1
        time.sleep(period)
except KeyboardInterrupt:
    print("Stopped.")
