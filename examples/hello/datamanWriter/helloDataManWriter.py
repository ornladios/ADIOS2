#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# helloDataManWriter.py
#
#  Created on: Sept 5, 2019
#      Author: Jason Wang <wangr1@ornl.gov>
#

from mpi4py import MPI
import numpy as np
import adios2

comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

Nx = 10
Ny = 10
steps = 100000

count = [Nx, Ny]
start = [rank * Nx, 0]
shape = [size * Nx, Ny]

floatArray = np.zeros(count, dtype=np.float32)

for i in range(0, Nx):
    for j in range(0, Ny):
        floatArray[i, j] = (start[0] + i) * shape[1] + (j + start[1])

adios = adios2.ADIOS(comm)
dataManIO = adios.DeclareIO("whatever")
dataManIO.SetEngine("DataMan")
dataManIO.SetParameters({"IPAddress": "127.0.0.1",
                         "Port": "12306",
                         "Timeout": "5"})

var = dataManIO.DefineVariable(
    "FloatArray", floatArray, shape, start, count, adios2.ConstantDims)

dataManWriter = dataManIO.Open('HelloDataMan', adios2.Mode.Write)

for i in range(steps):
    floatArray = floatArray + 1
    print("Step", i, floatArray)
    dataManWriter.BeginStep()
    dataManWriter.Put(var, floatArray)
    dataManWriter.EndStep()

dataManWriter.Close()
