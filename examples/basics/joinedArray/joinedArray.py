#
# Demonstrates writing joined array with ADIOS.
# Joined Array is a Global Array, where in one dimension we don't need to
# calculate starting offsets for each processes, nor the global dimension.
# Instead, the per-process blocks will be virtually joined together in the
# specified dimension, in the order of the blocks in the output file.
# User should not care of the order of blocks, but it will always be consistent
# ordering across multiple joined array variables as long as every process
# always writes the same number of blocks into each variable.
#
from mpi4py import MPI
import numpy
import adios2
import random

# MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

NSTEPS = 5
NCOLS = 4

with adios2.Stream("joinedArray-py.bp", "w", comm) as fh:
    # Either use begin_step/end_step explicitely within a loop, or
    # use stepping ADIOS stream, then use fh.current_step() for the step value
    vtable = fh._io.define_variable(
        "table", numpy.zeros([1], dtype=numpy.double), [adios2.JoinedDim, NCOLS], [], [1, NCOLS]
    )
    for _ in fh.steps(NSTEPS):
        # random size per process, 5..10 each
        nrows = random.randint(0, 5) + 5
        mytable = numpy.zeros([nrows, NCOLS], dtype=numpy.double)
        for row in range(0, nrows):
            for col in range(0, NCOLS):
                mytable[row, col] = rank * 1.0 + row * 0.1 + col * 0.01

        vtable.set_selection([[], [nrows, NCOLS]])
        fh.write("table", mytable, [adios2.JoinedDim, NCOLS], [], [nrows, NCOLS])


# Read content:
# bpls -l joinedArray-py.bp/ -D -t
# bpls -l joinedArray-py.bp/ -Dd -n 4 -f "%5.2f"
