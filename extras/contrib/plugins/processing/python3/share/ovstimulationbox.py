# -*- coding: utf-8 -*-
# UG fork: new file, see speller.md
#
# A Python box that handles stimulations the way the Lua Stimulator box does, so that a script only says what happens:
#  - the stimulations of each input wait in date order, until the script takes them;
#  - a script can send a stimulation with a date in the future, and it goes out when its time has come;
#  - the output chunks follow each other without a gap, or with filter_mode, take the time range of the input chunks.

import bisect
import __main__

OVBox = __main__.OVBox
OVStimulation = __main__.OVStimulation
OVStimulationHeader = __main__.OVStimulationHeader
OVStimulationSet = __main__.OVStimulationSet
OpenViBE_stimulation = __main__.OpenViBE_stimulation


def stimulation_code(name):
	""" The code of a stimulation, by its name as in a box setting (OVTK_StimulationId_Label_01) or as a number. """
	if name in OpenViBE_stimulation:
		return OpenViBE_stimulation[name]
	return int(name, 0)


LABEL_START = OpenViBE_stimulation['OVTK_StimulationId_LabelStart']
LABEL_END = OpenViBE_stimulation['OVTK_StimulationId_LabelEnd']


class OVStimulationBox(OVBox):
	filter_mode = False		# True: for a box that only reacts to its inputs, and passes on their time ranges

	def setup(self):
		""" Reads the settings. Called once. """
		pass

	def step(self):
		""" Takes and sends stimulations. Called on each clock tick, and each time an input has received something. """
		pass

	def take_stimulations(self, index):
		""" All stimulations waiting on an input, oldest first, as OVStimulation objects. """
		taken = [waiting[2] for waiting in self.__incoming[index]]
		self.__incoming[index] = []
		return taken

	def send_stimulation(self, output, identifier, date, duration=0.):
		""" Sends a stimulation on an output, once its date has passed. """
		if date < self.__lastTime:
			print('Warning: stimulation %d on output %d, dated %f, is older than the last chunk, at %f. It is dropped.' % (identifier, output, date, self.__lastTime))
			return
		self.__sequence += 1
		bisect.insort(self.__outgoing[output], (date, self.__sequence, identifier, duration))

	def initialize(self):
		self.__incoming = [[] for _ in self.input]
		self.__outgoing = [[] for _ in self.output]
		self.__lastTime = 0.
		self.__sequence = 0
		for output in self.output:
			output.append(OVStimulationHeader(0., 0.))
		self.setup()

	def process(self):
		# The time range of this round's output
		start = None if self.filter_mode else self.__lastTime
		end = 0. if self.filter_mode else self.getCurrentTime()

		for index, buffer in enumerate(self.input):
			for _ in range(len(buffer)):
				chunk = buffer.pop()
				if isinstance(chunk, OVStimulationSet):
					for stimulation in chunk:
						self.__sequence += 1
						bisect.insort(self.__incoming[index], (stimulation.date, self.__sequence, stimulation), key=lambda waiting: waiting[:2])
					if self.filter_mode:
						start = chunk.startTime if start is None else min(start, chunk.startTime)
						end = max(end, chunk.endTime)

		self.step()

		# Nothing to send yet at time 0, or from an input of a box that runs on its clock, in the same tick
		if end > 0. and (self.filter_mode or end > self.__lastTime):
			self.__sendUpTo(start, end)

	def __sendUpTo(self, start, end):
		for index, output in enumerate(self.output):
			chunk = OVStimulationSet(start, end)
			due = self.__outgoing[index]
			while due and due[0][0] < end:
				date, _, identifier, duration = due.pop(0)
				chunk.append(OVStimulation(identifier, date, duration))
			output.append(chunk)
		self.__lastTime = end
