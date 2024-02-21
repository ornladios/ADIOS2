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
from adios2 import Stream, FileReader

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
    if not rank:
        obpStream.write("N", [size, Nx, Ny])  # will be an array in output
        obpStream.write("Nx", numpy.array(Nx))  # will be a scalar in output
        obpStream.write("Ny", Ny)  # will be a scalar in output
        obpStream.write_attribute("dimensions", [size * Nx, Ny], "temperature2D")

if not rank:
    with FileReader("HeatMap2D_py.bp", MPI.COMM_SELF) as ibpFile:
        var_inTemperature = ibpFile.inquire_variable("temperature2D")
        if var_inTemperature is not None:
            var_inTemperature.set_selection([[2, 2], [4, 4]])
            inTemperatures = ibpFile.read(var_inTemperature)

        in_nx = ibpFile.read("Nx")  # scalar is read as a numpy array with 1 element
        in_ny = ibpFile.read("Ny")  # scalar is read as a numpy array with 1 element
        print(f"Incoming nx, ny = {in_nx}, {in_ny}")

        print("Incoming temperature map")
        for i in range(0, inTemperatures.shape[1]):
            print(str(inTemperatures[i]))
