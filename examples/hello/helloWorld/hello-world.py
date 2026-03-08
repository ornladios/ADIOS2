# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

import sys
from mpi4py import MPI
from adios2 import Stream, FileReader

DATA_FILENAME = "hello-world-py.bp"
# MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()


def writer(greeting):
    """write a string to a bp file"""
    with Stream(DATA_FILENAME, "w", comm) as fh:
        fh.write("Greeting", greeting)
    return 0


def reader():
    """read a string from a bp file as Stream"""
    message = f"variable Greeting not found in {DATA_FILENAME}"
    with Stream(DATA_FILENAME, "r", comm) as fh:
        for _ in fh.steps():
            message = fh.read("Greeting")
    return message


def filereader():
    """read a string from a bp file using random access read mode"""
    with FileReader(DATA_FILENAME, comm) as fh:
        message = fh.read("Greeting")
    return message


def main():
    """driver function"""
    greeting = "Hello World from ADIOS2"
    writer(greeting)
    message = reader()
    print("As read from adios2.Stream: {}".format(message))
    message2 = filereader()
    print("As read from adios2.FileReader: {}".format(message2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
