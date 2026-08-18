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

import csv
import sys

# test script parameter arg1: reference file csv 
# test script parameter arg2: test file csv

# assign arguments to variable
if len(sys.argv) != 3 :
    print('incorrect args')
    sys.exit(101)
referenceData=sys.argv[1]
testData=sys.argv[2]

def compareCells (file1,file2):
    """Compare values of cell from two list of lists"""
    # select the ligne for both csv file
    for line_index, (input_line, output_line) in enumerate(zip(file1, file2)) :
        #check line sizes 
        if len(input_line) != len(output_line):
            print('input and output cells size are different on line %s' % (line_index + 2))
            print('input cells: %s' % len(input_line))
            print('output cells: %s' % len(output_line))
            sys.exit(107)

        for col_index, (inputCell, outputCell) in enumerate(zip(input_line, output_line)):
            if inputCell != "" and abs(float(outputCell)-float(inputCell)) > sys.float_info.epsilon:
                print('error on line %s and column %s' % (line_index + 2, col_index+1))
                print( 'the input cell %s is different from output cell %s' % (inputCell, outputCell))
                sys.exit(107)

try:
    #open files associated with the variables
    with open(referenceData, 'r') as fileReferenceData :
        readerReferenceData = csv.reader(fileReferenceData, delimiter=',')
        inputData = [row for row in readerReferenceData]
    with open(testData, 'r') as fileTestData :
        readerTestData = csv.reader(fileTestData, delimiter=',')
        outputData = [row for row in readerTestData]
except IOError as err:
    print('missing input or ouput file')    
    print('missing file: %s'%(err.filename))
    sys.exit(106)

#check the file size of input and output files 
if len(inputData) != len(outputData) :
	print('Input (%s lines) and output (%s lines) files size are different :' % (len(inputData), len(outputData)))
	sys.exit(107)

# remove the headers and compare them
if inputData.pop(0) != outputData.pop(0) :
    print('Header is different')
    sys.exit(108)

#perform the cell comparison
compareCells(inputData, outputData)

print('All values are identiqual')

sys.exit()
