# clang-format off
#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# test_hello.py
#  Created on: Feb 2, 2017
#      Author: wfg

from mpi4py import MPI
from adios2py import *
import numpy as np


# MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

# User data
myArray = np.array([0, 1, 2, 3, 4, 5, 6, 7, 8, 9])
nx = myArray.size

# ADIOS
adios = ADIOS(comm, adiosDebugON)

# IO
bpIO = adios.DeclareIO("BPN2N")

# Variables
bpArray = bpIO.DefineVariable("bpArray", [size * nx], [rank * nx], [nx],
                              adiosConstantDims)
bpTimeStep = bpIO.DefineVariable("bpTimeStep")

# Engine
bpFileWriter = bpIO.Open("myArray.bp", adiosOpenModeWrite)
# Doesn't work: bpFileWriter = bpIO.Open("myArray.bp", adiosOpenModeWrite,
#                                                      MPI.COMM_WORLD)

for t in range(0, 10):
    if(rank == 0):
        bpFileWriter.Write(bpTimeStep, np.array([t]))

    bpFileWriter.Write(bpArray, myArray)
    bpFileWriter.Advance()

bpFileWriter.Close()
