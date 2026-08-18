#######################################################################
# Software License Agreement (AGPL-3 License)
#
# OpenViBE SDK Test Software
# Based on OpenViBE V1.1.0, Copyright (C) Inria, 2006-2015
# Copyright (C) Inria, 2015-2017,V1.0
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License version 3,
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.
# If not, see <http://www.gnu.org/licenses/>.
#######################################################################

#######################################################################
# Script description
# Goal: check if the number of stimuations present after classification
# is identical to a reference result.
#
# Step 1: Define time windows between 2 stimulation id in the file before the classification,
# the first stimulation ID will be the reference stimulation ID.
# Step 2: Count how many time you get the reference stimulation ID in the time windows.
# Step 3: Compute the number of success and provide a ratio.
# Step 4: Compare the ratio to a reference value compute from previous test.
#######################################################################

import csv
import sys
from itertools import islice

# assign arguments to variables
#arg 1 csv file recorded from ov file before the classification
#arg 2 csv file recorded after the classification
#arg 3 reference classification result
if len(sys.argv) < 4 :
    print('incorrect args')
    sys.exit(101)
# open files associated with the varaibles
with open(sys.argv[1], 'r') as fileReferenceData :
    readerReferenceData = csv.reader(fileReferenceData, delimiter=',')
    inputData = [rowReference for rowReference in readerReferenceData if rowReference[4] != ""]
    if len(inputData) == 0:
        print('The lenght of the file reference data: %s is equal to 0 or lower'%sys.argv[1])
        sys.exit(109)
with open(sys.argv[2], 'r') as fileTestData :
    readerTestData = csv.reader(fileTestData, delimiter=',')
    next(readerTestData)
    outputData = [rowReference for rowReference in readerTestData if rowReference[3] != ""]
    if len(outputData) == 0:
        print('The lenght of the test data: %s is equal to 0 or lower'%sys.argv[2])
        sys.exit(109)
with open(sys.argv[3], 'r') as fileClassificationReference :
    classificationReferenceData = [fileIndex for fileIndex in fileClassificationReference]

# Select into input data the signal with good stimulation marker
referenceData = [rowReference for rowReference in inputData if rowReference[3] in ['33026', '33027', '33025']]
referenceData.append(inputData[-1])

#  Check in a time window define by two timestamp the number of stimuation
for firstLineReference, secondLineReference, classificationLine in zip(referenceData, islice(referenceData, 1, None), classificationReferenceData) :
    # Create the first timestamp referenc
    stimulationReference = firstLineReference[3]
    firstTimeReference = float(firstLineReference[0])
    # Create the second timestamp reference
    secondTimeReference = float(secondLineReference[0])
    stimulationCount = 0
    numberOfStimulation = 0

    # Create the time window between the first and second time stamp reference
    #Count the reference stimulation ID
    ref_list = [index.count(stimulationReference) for index in outputData if firstTimeReference < float(index[0]) < secondTimeReference]
    print(ref_list)
    #Compute the percentage of success
    classificationEvaluation = (sum(ref_list)/len(ref_list))*100

    # Compare to the reference value
    if abs(classificationEvaluation - float(classificationLine)) > sys.float_info.epsilon:
        print('Classification reference value:%s'%classificationLine)
        print('Classification evaluation value:%s'%classificationEvaluation)
        sys.exit(108)

sys.exit()
