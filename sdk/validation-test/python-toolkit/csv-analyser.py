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
import re
import numpy as np

# test script parameter arg1: reference file csv 
# test script parameter arg2: test file csv
if len(sys.argv) < 3 :
    print('incorrect args')
    sys.exit(101)
# assign arguments to variable
ref_filename=sys.argv[1]
test_filename=sys.argv[2]

#open files associated with the variables

with open(ref_filename, 'r') as fileReferenceData :
    ref_header = next(fileReferenceData)
    ref_data = np.loadtxt(fileReferenceData, delimiter=",", usecols=[0, 1, 2, 3, 4])

min_epoch = np.amax(ref_data[ref_data[:,0] <= 1], 0)[1] + 1
start_index = np.where(ref_data[:,1] == min_epoch)[0][0]
max_epoch = np.amax(ref_data[ref_data[:,0] <= 2], 0)[1]
end_index = np.where(ref_data[:,1] == max_epoch)[0][-1] + 1
	
with open(test_filename, 'r') as fileTestData :
    test_header = next(fileTestData)
    test_data = np.loadtxt(fileTestData, delimiter=",", usecols=[0, 1, 2, 3, 4])

if test_header != ref_header or test_data.shape[0] != (end_index - start_index):
    print("Header or shape does not correspond")
    sys.exit(101)
if not np.allclose(test_data[:,[0, 2, 3, 4]], ref_data[start_index:end_index,[0, 2, 3, 4]]) :
    print("Data is different")
    sys.exit(100)
sys.exit()

