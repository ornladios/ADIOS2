#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# bpReaderHeatMap2D.py
#
#
#  Created on: Dec 5th, 2017
#      Author: William F Godoy godoywf@ornl.gov
#              Norbert Podhorszki pnorbert@ornl.gov
#

from mpi4py import MPI
import numpy
from adios2 import Stream

# MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

# User data
Nx = 10
Ny = 10

count = [Nx, Ny]
start = [rank * Nx, 0]
shape = [size * Nx, Ny]

temperatures = numpy.zeros(Nx * Ny, dtype=int)

for i in range(0, Nx):
    iGlobal = start[0] + i

    for j in range(0, Ny):
        value = iGlobal * shape[1] + j
        temperatures[i * Nx + j] = value
#        print(str(i) + "," + str(j) + " " + str(value))

with Stream("HeatMap2D_py.bp", "w", comm) as obpStream:
    obpStream.write("temperature2D", temperatures, shape, start, count)

if rank == 0:
    with Stream("HeatMap2D_py.bp", "r", MPI.COMM_SELF) as ibpStream:
        for _ in ibpStream.steps():
            var_inTemperature = ibpStream.inquire_variable("temperature2D")
            if var_inTemperature is not None:
                var_inTemperature.set_selection([[2, 2], [4, 4]])
                inTemperatures = ibpStream.read(var_inTemperature)

            print("Incoming temperature map")
            for i in range(0, inTemperatures.shape[1]):
                print(str(inTemperatures[i]) + " ")
