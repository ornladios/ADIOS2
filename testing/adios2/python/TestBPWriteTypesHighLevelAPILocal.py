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
from adios2 import Stream


def check_array(np1, np2, hint):
    if (np1 == np2).all() is False:
        print("InData: " + str(np1))
        print("Data: " + str(np2))
        raise ValueError("Array read failed " + str(hint))


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
with Stream("types_np_local.bp", "w", comm) as s:
    for step in s.steps(5):
        data.update(rank, step.current_step(), size)
        s.write("varI8", data.i8, shape, start, count)
        s.write("varI16", data.i16, shape, start, count)
        s.write("varI32", data.i32, shape, start, count)
        s.write("varI64", data.i64, shape, start, count)
        s.write("varU8", data.u8, shape, start, count)
        s.write("varU16", data.u16, shape, start, count)
        s.write("varU32", data.u32, shape, start, count)
        s.write("varU64", data.u64, shape, start, count)
        s.write("varR32", data.r32, shape, start, count)
        s.write("varR64", data.r64, shape, start, count)

# Reader
data = SmallTestData()

with Stream("types_np_local.bp", "r", comm) as s:
    for fr_step in s.steps():
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

            check_array(indataI8, data.i8, "i8")
            check_array(indataI16, data.i16, "i16")
            check_array(indataI32, data.i32, "i32")
            check_array(indataI64, data.i64, "i64")
            check_array(indataU8, data.u8, "u8")
            check_array(indataU16, data.u16, "u16")
            check_array(indataU32, data.u32, "u32")
            check_array(indataU64, data.u64, "u64")
            check_array(indataR32, data.r32, "r32")
            check_array(indataR64, data.r64, "r64")
