# -*- coding: utf-8 -*-
# UG fork: new file, see speller.md
#
# Picks out the targets from a recording's stimulation stream: the labels between a rest's start and stop

from ovstimulationbox import OVStimulationBox, stimulation_code, LABEL_START, LABEL_END


class MyOVBox(OVStimulationBox):
	filter_mode = True

	def setup(self):
		self.restStart = stimulation_code('OVTK_StimulationId_RestStart')
		self.restStop = stimulation_code('OVTK_StimulationId_RestStop')
		self.inRest = False

	def step(self):
		for stimulation in self.take_stimulations(0):
			if stimulation.identifier == self.restStart:
				self.inRest = True
			elif stimulation.identifier == self.restStop:
				self.inRest = False

			if self.inRest and LABEL_START <= stimulation.identifier <= LABEL_END:
				self.send_stimulation(0, stimulation.identifier, stimulation.date)


box = MyOVBox()
