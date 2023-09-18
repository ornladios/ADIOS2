#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# hello-world.py : adios2 high-level API example to write and read a
#                   string Variable with a greeting
#
#  Created on: 2/2/2021
#      Author: Dmitry Ganyushin ganyushindi@ornl.gov
#
import sys
from mpi4py import MPI
import adios2

DATA_FILENAME = "hello-world-hl-py.bp"
# MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()


def writer(greeting):
    """write a string to a bp file"""
    with adios2.open(DATA_FILENAME, "w", comm) as fh:
        fh.write("Greeting", greeting, end_step=True)
    return 0


def reader():
    """read a string from to a bp file"""
    with adios2.open(DATA_FILENAME, "r", comm) as fh:
        for fstep in fh:
            message = fstep.read_string("Greeting")
    return message


def main():
    """driver function"""
    greeting = "Hello World from ADIOS2"
    writer(greeting)
    message = reader()
    print("{}".format(message))
    return 0


if __name__ == "__main__":
    sys.exit(main())
