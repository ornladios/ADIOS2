#!/usr/bin/env python

# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# TestBPWriteTypesHighLevelAPILocal.py: test Python numpy types in ADIOS2 File
#                      Write/Read High-Level API for Local Arrays
#  Created on: March 12, 2018
#      Author: William F Godoy godoywf@ornl.gov

from adios2NPTypes import SmallTestData
from mpi4py import MPI
import adios2


def check_array(np1, np2, hint):
    if (np1 == np2).all() is False:
        print("InData: " + str(np1))
        print("Data: " + str(np2))
        raise ValueError('Array read failed ' + str(hint))


comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

# Test data
data = SmallTestData()
nx = data.Nx

shape = []
start = []
count = [nx]

# Writer
with adios2.open("types_np_local.bp", "w", comm) as fw:

    for i in range(0, 5):

        data.update(rank, i, size)

        fw.write("varI8", data.I8, shape, start, count)
        fw.write("varI16", data.I16, shape, start, count)
        fw.write("varI32", data.I32, shape, start, count)
        fw.write("varI64", data.I64, shape, start, count)
        fw.write("varU8", data.U8, shape, start, count)
        fw.write("varU16", data.U16, shape, start, count)
        fw.write("varU32", data.U32, shape, start, count)
        fw.write("varU64", data.U64, shape, start, count)
        fw.write("varR32", data.R32, shape, start, count)
        fw.write("varR64", data.R64, shape, start, count)
        fw.end_step()


# Reader
data = SmallTestData()

with adios2.open("types_np_local.bp", "r", comm) as fr:

    for fr_step in fr:

        step = fr_step.current_step()

        for b in range(0, size):

            data.update(b, step, size)

            indataI8 = fr_step.read("varI8", b)
            indataI16 = fr_step.read("varI16", b)
            indataI32 = fr_step.read("varI32", b)
            indataI64 = fr_step.read("varI64", b)
            indataU8 = fr_step.read("varU8", b)
            indataU16 = fr_step.read("varU16", b)
            indataU32 = fr_step.read("varU32", b)
            indataU64 = fr_step.read("varU64", b)
            indataR32 = fr_step.read("varR32", b)
            indataR64 = fr_step.read("varR64", b)

            check_array(indataI8, data.I8, 'I8')
            check_array(indataI16, data.I16, 'I16')
            check_array(indataI32, data.I32, 'I32')
            check_array(indataI64, data.I64, 'I64')
            check_array(indataU8, data.U8, 'U8')
            check_array(indataU16, data.U16, 'U16')
            check_array(indataU32, data.U32, 'U32')
            check_array(indataU64, data.U64, 'U64')
            check_array(indataR32, data.R32, 'R32')
            check_array(indataR64, data.R64, 'R64')
