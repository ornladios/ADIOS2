from mpi4py import MPI
import numpy as np
import adios2

# MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

# ADIOS MPI Communicator, debug mode
adios = adios2.ADIOS(comm)

# ADIOS IO
sstIO = adios.DeclareIO("myIO")
sstIO.SetEngine('Sst')
myArray = np.array([0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0],
                   dtype='f')
myArray = 10.0 * rank + myArray
nx = len(myArray)
ioArray = sstIO.DefineVariable("bpFloats", myArray, [size * nx],
                               [rank * nx], [nx], adios2.ConstantDims)
sstFileWriter = sstIO.Open("helloSst", adios2.Mode.Write)
sstFileWriter.BeginStep()
sstFileWriter.Put(ioArray, myArray, adios2.Mode.Sync)
sstFileWriter.EndStep()
sstFileWriter.Close()
