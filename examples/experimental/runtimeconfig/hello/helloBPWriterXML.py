#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# helloBPWriterXML.py
#  Created on: Feb 2, 2017
#      Author: William F Godoy godoywf@ornl.gov

from mpi4py import MPI
import adios2
import numpy

# MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

# User data
myArray = numpy.array([0., 1., 2., 3., 4., 5., 6., 7., 8., 9.])
Nx = myArray.size

# ADIOS config file, MPI communicator, debug mode
adios = adios2.ADIOS("helloBPWriter.xml", comm)

# ADIOS IO, name must be the same as in helloBPWriter.xml for runtime settings
bpIO = adios.DeclareIO("BPFile_N2N")

# Variable name, shape, start, count, ConstantDims = True
ioArray = bpIO.DefineVariable(
    "bpArray", [size * Nx], [rank * Nx], [Nx], adios2.ConstantDims)

# Engine name, open mode
bpFileWriter = bpIO.Open("npArray.bp", adios2.OpenModeWrite)
# Write  variable, numpy object
bpFileWriter.Write(ioArray, myArray)
bpFileWriter.Close()
