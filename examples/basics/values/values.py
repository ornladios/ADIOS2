#
# Demonstrates writing Values (single element variables) with ADIOS
# GlobalValue:  a single value written by one process
# LocalValue:   one value per process, represented as 1D GlobalArray at reading
#   LocalValue must be defined as an adios2.Variable first and used in
#       subsequent write() calls.
#
from mpi4py import MPI
import numpy
import adios2
import random

# MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

# User data
myArray = numpy.array([0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0])
myArray = myArray + (rank * 10)
Nx = myArray.size

NSTEPS = 5

with adios2.Stream("values-py.bp", "w", comm) as fh:
    # Either use begin_step/end_step explicitely within a loop, or
    # use stepping ADIOS stream, then use fh.current_step() for the step value
    vnparts = fh._io.define_variable("Nparts", rank, [adios2.LocalValueDim])
    vprocid = fh._io.define_variable("ProcessID", rank, [adios2.LocalValueDim])
    for _ in fh.steps(NSTEPS):
        if fh.current_step() == 0:
            # This does not work:
            #    fh.write("ProcessID", rank, [adios2.LocalValueDim], [], [])
            fh.write(vprocid, rank)
            if rank == 0:
                fh.write("Nproc", size)

        if rank == 0:
            fh.write("GlobalString", f"This is step {fh.current_step()}")
            fh.write("Step", fh.current_step())

        # random size per process, 5..10 each
        Nparts = random.randint(0, 5) + 5
        fh.write(vnparts, Nparts)

        fh.write("a", myArray, [size * Nx], [rank * Nx], [Nx])
        fh.write("ajoined", myArray, [adios2.JoinedDim], [], [Nx])


# Read content:
# bpls -l values-py.bp
# bpls -l values-py.bp -D
# bpls -l values-py.bp -d
