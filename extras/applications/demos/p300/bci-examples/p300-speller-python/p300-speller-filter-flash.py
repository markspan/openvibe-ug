# -*- coding: utf-8 -*-
# UG fork: new file, see speller.md
#
# Picks out the flashes from a recording's stimulation stream: the labels between a segment's start and stop

from ovstimulationbox import OVStimulationBox, stimulation_code, LABEL_START, LABEL_END


class MyOVBox(OVStimulationBox):
	filter_mode = True

	def setup(self):
		self.segmentStart = stimulation_code('OVTK_StimulationId_SegmentStart')
		self.segmentStop = stimulation_code('OVTK_StimulationId_SegmentStop')
		self.inSegment = False

	def step(self):
		for stimulation in self.take_stimulations(0):
			if stimulation.identifier == self.segmentStart:
				self.inSegment = True
			elif stimulation.identifier == self.segmentStop:
				self.inSegment = False

			if self.inSegment and LABEL_START <= stimulation.identifier <= LABEL_END:
				self.send_stimulation(0, stimulation.identifier, stimulation.date)


box = MyOVBox()
