# -*- coding: utf-8 -*-
# UG fork: new file, see speller.md
#
# For the test scenario, where the classifier is replaced by the display's own flags: the selected letters must be the targets.
# Input 0: the targets. Input 1: the selected rows. Input 2: the selected columns.

from ovstimulationbox import OVStimulationBox, LABEL_START, LABEL_END


class MyOVBox(OVStimulationBox):
	def setup(self):
		self.targets = []
		self.rows = []
		self.columns = []

	def step(self):
		for stimulation in self.take_stimulations(0):
			if LABEL_START <= stimulation.identifier <= LABEL_END:
				self.targets.append(stimulation.identifier)
		self.rows += [stimulation.identifier for stimulation in self.take_stimulations(1)]
		self.columns += [stimulation.identifier for stimulation in self.take_stimulations(2)]

	def uninitialize(self):
		selected = [code for pair in zip(self.rows, self.columns) for code in pair]
		if self.targets and selected == self.targets:
			print('P300 speller scripts: %d letters, as expected' % len(self.rows))
		else:
			print('P300 speller scripts: expected %s, got %s' % (self.targets, selected))


box = MyOVBox()
