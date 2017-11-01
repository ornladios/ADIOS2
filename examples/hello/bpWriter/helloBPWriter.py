#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# helloBPWriter.py
#  Created on: Feb 2, 2017
#      Author: William F Godoy godoywf@ornl.gov

from mpi4py import MPI
import numpy
import adios2

# MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

# User data
myArray = numpy.array([0, 1., 2., 3., 4., 5., 6., 7., 8., 9.])
Nx = myArray.size

# ADIOS MPI Communicator, debug mode
adios = adios2.ADIOS(comm, adios2.DebugON)

# ADIOS IO
bpIO = adios.DeclareIO("BPFile_N2N")

# ADIOS Variable name, shape, start, offset, constant dims
ioArray = bpIO.DefineVariable(
    "bpArray", [size * Nx], [rank * Nx], [Nx], adios2.ConstantDims)

# ADIOS Engine
bpFileWriter = bpIO.Open("npArray.bp", adios2.OpenModeWrite)
# doesn't work: bpFileWriter = bpIO.Open("npArray.bp", adios2.OpenModeWrite, newcomm)
bpFileWriter.PutSync(ioArray, myArray)
bpFileWriter.Close()
