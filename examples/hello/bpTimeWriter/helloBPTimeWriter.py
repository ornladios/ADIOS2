#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# test_hello.py
#  Created on: Feb 2, 2017
#      Author: William F Godoy godoywf@ornl.gov

from mpi4py import MPI
import adios2
import numpy as np


# MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

# User data
myArray = np.array([0, 1, 2, 3, 4, 5, 6, 7, 8, 9])
nx = myArray.size

# ADIOS
adios = adios2.ADIOS(comm, adios2.DebugON)

# IO
bpIO = adios.DeclareIO("BPN2N")

# Variables
bpArray = bpIO.DefineVariable("bpArray", [size * nx], [rank * nx], [nx],
                              adios2.ConstantDims, myArray)
bpTimeStep = bpIO.DefineVariable("bpTimeStep")

# Engine
bpFileWriter = bpIO.Open("myArray.bp", adios2.OpenModeWrite)
# Doesn't work: bpFileWriter = bpIO.Open("myArray.bp", adiosOpenModeWrite,
#                                                      MPI.COMM_WORLD)

for t in range(0, 10):
    bpFileWriter.BeginStep()
    if(rank == 0):
        bpFileWriter.Put(bpTimeStep, np.array([t]))
    bpFileWriter.Put(bpArray, myArray)
    bpFileWriter.EndStep()

bpFileWriter.Close()
