#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# ensembleRead.py
# A Use Case for reading metadata by one process only, then distribute that
# among other processes/applications, which can "open" the dataset faster
# by processing metadata from memory.
#
# Data is still read from disk by each process/application.
#
# This is an MPI application but every process acts as a separate entity as far as
# reading with ADIOS goes.
#
# Created on: Aug 12, 2025
#      Author: pnorbert
#

from mpi4py import MPI
import numpy
import pickle
import sys
import os.path
from adios2 import Stream, FileReader

if len(sys.argv) < 1:
    print(f"Usage: {sys.argv[0]} filename")
    sys.exit(1)

filename = sys.argv[1]

if not os.path.exists(filename):
    print(f"Not found {filename}")
    sys.exit(1)

# MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

if not rank:
    with FileReader(filename, MPI.COMM_SELF) as f:
        md = f.get_metadata()
        print(f"md type {type(md)}  len = {len(md)}  id = {id(md)}")
else:
    md = None

comm.Barrier()
md = comm.bcast(md)
print(f"rank {rank:<4}: md type {type(md)}  len = {len(md)}  id = {id(md)}")

# Process file in a sequentialized order only for pretty printing
token = numpy.zeros(1)
status = MPI.Status()
if rank > 0:
    comm.Recv(token, rank - 1, 0, status)

with FileReader(md, filename) as f:
    vars = f.available_variables()
    print(f"File info on rank {rank}:")
    print(f"  Steps in file: {f.num_steps()}:")
    print(f"  Total number of variables: {len(vars)}:")
    max_name_length = 1
    for var in vars:
        max_name_length = max(max_name_length, len(vars))
    for var in vars:
        v = f.inquire_variable(var)
        print(f"    {v.type():<8}  {var:<{max_name_length}} ", end="")
        if len(v.shape()) > 0:
            print(f"  {v.shape()}", end="")
        print()

if rank < size - 1:
    comm.Send(token, rank + 1, 0)
