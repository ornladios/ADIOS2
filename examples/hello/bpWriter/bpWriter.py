#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# bpWriter.py : only works with MPI version
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
myArray = numpy.array([0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0])
Nx = myArray.size

# ADIOS MPI Communicator
adios = adios2.ADIOS(comm)

# ADIOS IO
bpIO = adios.DeclareIO("BPFile_N2N")
bpIO.SetEngine("BPFile")
# bpIO.SetParameters( {"Threads" : "2", "ProfileUnits" : "Microseconds",
# "InitialBufferSize" : "17Kb"} )
bpIOParams = {}
bpIOParams["Threads"] = "2"
bpIOParams["ProfileUnits"] = "Microseconds"
bpIOParams["InitialBufferSize"] = "17Kb"
bpIO.SetParameters(bpIOParams)

fileID = bpIO.AddTransport("File", {"Library": "fstream"})

# ADIOS Variable name, shape, start, offset, constant dims
ioArray = bpIO.DefineVariable(
    "bpArray", myArray, [size * Nx], [rank * Nx], [Nx], adios2.ConstantDims
)

# ADIOS Engine
bpFileWriter = bpIO.Open("npArray.bp", adios2.Mode.Write)
bpFileWriter.BeginStep()
bpFileWriter.Put(ioArray, myArray, adios2.Mode.Sync)
bpFileWriter.EndStep()
bpFileWriter.Close()
