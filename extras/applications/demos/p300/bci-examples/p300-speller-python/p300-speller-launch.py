# -*- coding: utf-8 -*-
# UG fork: new file, see speller.md
#
# Sends one stimulation, at the given time: what starts the flashing

from ovstimulationbox import OVStimulationBox, stimulation_code


class MyOVBox(OVStimulationBox):
	def setup(self):
		self.stimulation = stimulation_code(self.setting['Stimulation to send'])
		self.time = float(self.setting['Time to send'])
		self.sent = False

	def step(self):
		if not self.sent:
			self.send_stimulation(0, self.stimulation, self.time)
			self.sent = True


box = MyOVBox()
