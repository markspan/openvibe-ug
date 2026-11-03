# -*- coding: utf-8 -*-
# UG fork: new file, see speller.md
#
# Works out each letter from the classifier's predictions. Every flash that the classifier calls a target gives its row
# or column a vote. After the trial's last prediction, the row and the column with the most votes are sent.
#
# Input 0: the predictions, one for each flash. Input 1: the timeline, for which row or column each flash was.
# Output 0: the selected row. Output 1: the selected column.

from collections import deque

from ovstimulationbox import OVStimulationBox, stimulation_code, LABEL_END


class MyOVBox(OVStimulationBox):
	VOTE_SLOTS = 20
	INACTIVE, IN_SEGMENT, CAN_VOTE = 0, 1, 2

	def setup(self):
		self.rowBase = stimulation_code(self.setting['Row stimulation base'])
		self.columnBase = stimulation_code(self.setting['Column stimulation base'])
		self.segmentStart = stimulation_code(self.setting['Segment start'])
		self.segmentStop = stimulation_code(self.setting['Segment stop'])
		self.target = stimulation_code('OVTK_StimulationId_Target')
		self.nonTarget = stimulation_code('OVTK_StimulationId_NonTarget')
		self.status = self.INACTIVE
		self.flashes = deque()	# The flashes that wait for their prediction, which come some time later
		self.rowVotes = None
		self.columnVotes = None

	def readTimeline(self):
		for stimulation in self.take_stimulations(1):
			identifier = stimulation.identifier
			if identifier == self.segmentStart:
				self.rowVotes = [0] * self.VOTE_SLOTS
				self.columnVotes = [0] * self.VOTE_SLOTS
				self.flashes.clear()
				self.status = self.IN_SEGMENT

			# Rows come before columns in the labels
			if self.status == self.IN_SEGMENT and self.rowBase <= identifier <= LABEL_END:
				if identifier < self.columnBase:
					self.flashes.append((self.rowVotes, identifier - self.rowBase))
				else:
					self.flashes.append((self.columnVotes, identifier - self.columnBase))

			if identifier == self.segmentStop:
				self.status = self.CAN_VOTE

	def readPredictions(self):
		for stimulation in self.take_stimulations(0):
			if stimulation.identifier in (self.target, self.nonTarget):
				if not self.flashes:
					print('Warning: a prediction without a flash to go with it. The votes of this trial are not to be trusted.')
					continue
				votes, index = self.flashes.popleft()
				if stimulation.identifier == self.target:
					votes[index] += 1

	def vote(self):
		# When votes are equal, the first one wins
		row = self.rowVotes.index(max(self.rowVotes))
		column = self.columnVotes.index(max(self.columnVotes))
		if self.rowVotes[row] == 0 and self.columnVotes[column] == 0:
			print("Warning: Classifier predicted 'no p300' for all flashes of the trial")

		now = self.getCurrentTime()
		self.send_stimulation(0, row + self.rowBase, now)
		self.send_stimulation(1, column + self.columnBase, now)
		self.status = self.INACTIVE

	def step(self):
		self.readTimeline()
		self.readPredictions()
		if self.status == self.CAN_VOTE and not self.flashes:
			if self.rowVotes is None:
				self.status = self.INACTIVE		# A stop without a start: there is nothing to vote on
			else:
				self.vote()


box = MyOVBox()
