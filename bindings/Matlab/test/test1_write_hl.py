# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0


import numpy
import adios2

# User data
NRows = 5
NCols = 6

shape = [NRows, NCols]
start = [0, 0]
count = [NRows, NCols]

temperatures = numpy.zeros(NRows * NCols, dtype=numpy.int16)

value = (NRows * NCols) + 1
for i in range(0, NRows):
    for j in range(0, NCols):
        temperatures[i * NCols + j] = value
        value = value + 1

# ADIOS2 high-level API for Write
fw = adios2.open("test1.bp", "w")
fw.write("note", 'This is an ADIOS2 output')
fw.write("temperature2D", temperatures, shape, start, count)
fw.write("nrows", numpy.array([NRows]))
fw.write("ncols", numpy.array([NCols]))
fw.close()
