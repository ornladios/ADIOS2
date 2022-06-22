#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# helloBPWriterXML_nompi.py serial non-MPI version of helloBPWriter.py
#  Created on: Feb 2, 2017
#      Author: William F Godoy godoywf@ornl.gov

import numpy
import adios2


# User data
myArray = numpy.array([0., 1., 2., 3., 4., 5., 6., 7., 8., 9.])
Nx = myArray.size

# ADIOS config file, debug mode
adios = adios2.ADIOS("helloBPWriter.xml")

# ADIOS IO, name must be the same as in helloBPWriter.xml for runtime settings
bpIO = adios.DeclareIO("BPFile_N2N")

# ADIOS local array: Variable name, shape, start, offset
ioArray = bpIO.DefineVariable(
    "bpArray", [], [], [Nx], adios2.ConstantDims)

# ADIOS Engine
bpFileWriter = bpIO.Open("npArray.bp", adios2.OpenModeWrite)
bpFileWriter.Write(ioArray, myArray)
bpFileWriter.Close()
