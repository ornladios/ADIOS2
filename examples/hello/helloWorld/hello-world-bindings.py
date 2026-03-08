# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

import sys
from mpi4py import MPI
import adios2.bindings as adios2

DATA_FILENAME = "hello-world-py-bindings.bp"
# MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()


def writer(ad, greeting):
    """write a string to a bp file"""
    io = ad.DeclareIO("hello-world-writer")
    var_greeting = io.DefineVariable("Greeting")
    w = io.Open(DATA_FILENAME, adios2.Mode.Write)
    w.BeginStep()
    w.Put(var_greeting, greeting)
    w.EndStep()
    w.Close()
    return 0


def reader(ad):
    """read a string from to a bp file"""
    io = ad.DeclareIO("hello-world-reader")
    r = io.Open(DATA_FILENAME, adios2.Mode.Read)
    r.BeginStep()
    var_greeting = io.InquireVariable("Greeting")
    message = r.Get(var_greeting)
    r.EndStep()
    r.Close()
    return message


def main():
    """driver function"""
    ad = adios2.ADIOS(comm)
    greeting = "Hello World from ADIOS2"
    writer(ad, greeting)
    message = reader(ad)
    print("{}".format(message))
    return 0


if __name__ == "__main__":
    sys.exit(main())
