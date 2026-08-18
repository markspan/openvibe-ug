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
import sys, os, re

#arg 1 repository of the openvibe logs
#arg 2 keyword to search in the log file to get the line where the measure was
#arg 3 Threshold, the measure should be lower
if len(sys.argv) < 4 :
    print('incorrect args')
    sys.exit(101)
openvibeLogs=sys.argv[1]
keyWord=sys.argv[2]
threshold=float(sys.argv[3])

def selectLineContainingWord (filename, searchingWord):
    """ Function to select the line containing the keyword
    the exception 103 is raised if file is missing 
    """
    try:
        spaced_word = ' %s ' % searchingWord
        with open(filename) as file :
            for line in file:
                if spaced_word in line :
                    return line

    #Exception raised if the log file wasn't created
    except FileNotFoundError:
        print('missing %s'%file)
        sys.exit(103)

try:
    #Call the selectLineContainingWord function with the input parameter
    workingLine=selectLineContainingWord(openvibeLogs,keyWord)
	
    getPercentageValue = float(re.search('accuracy\s+is\s+(\d+(?:\.\d+))\%', workingLine).group(1))

    #check if the measure is under the threshold excpected
    if getPercentageValue >= threshold:
        print ('%s higher or equal to threshold %s'%(getPercentageValue,threshold))
    
    else:
        print ('%s lower to threshold %s'%(getPercentageValue,threshold))
        print ('reference line in the log %s' %workingLine)
        sys.exit(102)

#exception raised if the keyword wasn't in the log file
except AttributeError:
    print ('Missing searching word: %s in the file'%keyWord)
    sys.exit(104)
#exception raised if the word: is, wasn't in the log file
except ValueError:
    print ('error missing word: is in the selected line')
    sys.exit(105)

except:
    raise

sys.exit()

 