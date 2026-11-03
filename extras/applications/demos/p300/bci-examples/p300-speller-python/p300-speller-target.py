# -*- coding: utf-8 -*-
# UG fork: new file, see speller.md
#
# Generates the target stimulations for the P300 visualization box, from the grid of letters and numbers of the speller.
# Unlike the Lua script, it takes capitals too, and says so when it can not give a target.

import random

from ovstimulationbox import OVStimulationBox, stimulation_code


class MyOVBox(OVStimulationBox):
	# As in the speller's interface file
	GRID = ('abcdef', 'ghijkl', 'mnopqr', 'stuvwx', 'yz1234', '567890')

	def setup(self):
		self.word = self.setting['Word to Spell (leave this empty if random)'].lower()
		self.rowBase = stimulation_code(self.setting['Row Stimulation Base'])
		self.columnBase = stimulation_code(self.setting['Column Stimulation Base'])
		self.delay = float(self.setting['Delay Before Sending (in sec)'])
		self.restStart = stimulation_code('OVTK_StimulationId_RestStart')
		self.experimentStop = stimulation_code('OVTK_StimulationId_ExperimentStop')
		self.train = stimulation_code('OVTK_StimulationId_Train')
		self.index = 0
		if self.word == '':
			self.word = ''.join(random.choice(random.choice(self.GRID)) for _ in range(1000))

	def location(self, letter):
		for row, letters in enumerate(self.GRID):
			column = letters.find(letter)
			if column >= 0 and letter != '':
				return row, column
		return None

	def step(self):
		now = self.getCurrentTime()
		for stimulation in self.take_stimulations(0):
			if stimulation.identifier == self.restStart:
				letter = self.word[self.index:self.index + 1]
				self.index += 1
				place = self.location(letter)
				if place is None:
					print("Warning: no target for trial %d: '%s' is not in the grid, or the word has ended" % (self.index, letter))
				else:
					self.send_stimulation(0, self.rowBase + place[0], now + self.delay)
					self.send_stimulation(0, self.columnBase + place[1], now + self.delay)
			elif stimulation.identifier == self.experimentStop:
				self.send_stimulation(0, self.train, now + self.delay + 1)


box = MyOVBox()
