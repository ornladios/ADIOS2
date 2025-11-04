#!/bin/env python3

# Demonstrating deferred read
# multiple f.read(...) can be scheduled and executed once by
# f.read_complete()
# which allows ADIOS to use more threads to read more data concurrently.

# Run this after running the example
# mpirun -n 4 ./bin/adios2_basics_globalArrayNDWrite_mpi

import adios2
import numpy

with adios2.FileReader("globalArray.bp") as f:
    v = f.inquire_variable("GlobalArray")
    print(f"Variable GlobalArray shape = {v.shape()}, steps = {v.steps()}")
    data = f.read(v, defer_read=True)
    print(
        f"WRONG before data is read: data size = {data.size} "
        f"min = {data.min()}, max = {data.max()}"
    )
    f.read_complete()
    print(
        f"CORRECT after data is read: data size = {data.size} "
        f"min = {data.min()}, max = {data.max()}"
    )
