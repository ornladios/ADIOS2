#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# helloBPReaderHeatMap2D.py
#
#
#  Created on: Dec 5th, 2017
#      Author: William F Godoy godoywf@ornl.gov
#

from mpi4py import MPI
import numpy
import adios2

# MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

# User data
Nx = 10
Ny = 10

shape = [size * Nx, Ny]
start = [rank * Nx, 0]
count = [Nx, Ny]

temperatures = numpy.zeros(Nx * Ny, dtype=numpy.int16)

for i in range(0, Nx):
    iGlobal = start[0] + i

    for j in range(0, Ny):
        value = iGlobal * shape[1] + j
        temperatures[i * Nx + j] = value
        
# ADIOS2 high-level API for Write
fw = adios2.open("temperature.bp", "w", comm)
fw.write("varString", 'This is ADIOS2')
fw.write("temperature2D", temperatures, shape, start, count)
fw.close()


# ADIOS2 high-level API for Reading
if rank == 0:
  fr = adios2.open("temperature.bp", "r", MPI.COMM_SELF)
  
  inTemperatures = fr.read("temperature2D")
  
  if( inTemperatures is not None ):
  
      print( "Size " + str(inTemperatures.size) )
  
      for i in range(0,shape[0]):
          for j in range(0,shape[1]):
              print(str(inTemperatures[i][j]) + ' ' , end='')
          print()

  
  
  inString = fr.readstring("varString")
  if( inString is not None ):
      print( inString )  
  fr.close()
  
