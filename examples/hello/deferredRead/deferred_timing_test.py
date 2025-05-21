#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# Write multiple arrays to a file
# Compare times to
#    Reading them back one by one
#    Reading them back at once with Deferred read mode
#

import numpy as np
from adios2 import Stream, FileReader
from time import perf_counter as pc

# User data
Nx = 100
Ny = 1000
Nvars = 1000
fname = "deferred_test.bp"

count = [Nx, Ny]
start = [0, 0]
shape = [Nx, Ny]

with Stream(fname, "w") as outf:
    data = np.random.rand(Nvars, Nx, Ny)
    print(f"Write {Nvars} arrays of [{Nx}, {Ny}] shape")
    twrite_start = pc()
    for n in range(Nvars):
        outf.write(f"data_{n:0>3}", data[n:n + 1, :, :].squeeze(), shape, start, count)
    outf.write("N", [Nvars, Nx, Ny])  # will be an array in output
    outf.write("Nx", np.array(Nx))  # will be a scalar in output
    outf.write("Ny", Ny)  # will be a scalar in output
    outf.write("Nvars", Nvars)  # will be a scalar in output
twrite_end = pc()
print(f"Time of writing: {twrite_end - twrite_start:0.6f}s")

with FileReader(fname) as inpf:
    # scalar variables are read as a numpy array with 0 dimension
    in_nx = inpf.read("Nx")
    in_ny = inpf.read("Ny")
    in_nvars = inpf.read("Nvars")
    print(f"Incoming nx, ny, nvars = {in_nx}, {in_ny}, {in_nvars}")

    tread_start = pc()
    for n in range(Nvars):
        var = inpf.inquire_variable(f"data_{n:0>3}")
        if var is not None:
            var.set_selection([start, shape])
            data = inpf.read(var)
    tread_end = pc()
    print(f"Time of reading one by one (Sync mode): {tread_end - tread_start:0.6f}s")

with FileReader(fname) as inpf:
    # scalar variables are read as a numpy array with 0 dimension
    in_nx = inpf.read("Nx")
    in_ny = inpf.read("Ny")
    in_nvars = inpf.read("Nvars")
    print(f"Incoming nx, ny, nvars = {in_nx}, {in_ny}, {in_nvars}")

    in_data = np.random.rand(Nvars, Nx, Ny)

    tread_start = pc()
    for n in range(Nvars):
        var = inpf.inquire_variable(f"data_{n:0>3}")
        if var is not None:
            var.set_selection([start, shape])
            data = inpf.read_in_buffer(var, buffer=in_data[n:n + 1, :, :], defer_read=True)
    inpf.read_complete()
    tread_end = pc()
    print(f"Time of reading at once (Deferred mode): {tread_end - tread_start:0.6f}s")
