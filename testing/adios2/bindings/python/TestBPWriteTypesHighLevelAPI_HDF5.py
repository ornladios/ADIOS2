#!/usr/bin/env python

#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# TestBPWriteTypes.py: test Python numpy types in ADIOS2 File
#                      Write/Read High-Level API
#  Created on: March 12, 2018
#      Author: William F Godoy godoywf@ornl.gov


from adios2NPTypes import SmallTestData
from mpi4py import MPI
import numpy as np
import adios2

comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

# Test data
data = SmallTestData()
nx = data.Nx

shape = [size * nx]
start = [rank * nx]
count = [nx]

# Writer
with adios2.open("types_np.bp", "w", comm, "HDF5") as fw:

    for i in range(0, 5):
        
        data.update(rank, i, size)
        
        if(rank == 0 and i == 0):
            fw.write("tag", "Testing ADIOS2 high-level API")
            fw.write("gvarI8", np.array(data.I8[0]))
            fw.write("gvarI16", np.array(data.I16[0]))
            fw.write("gvarI32", np.array(data.I32[0]))
            fw.write("gvarI64", np.array(data.I64[0]))
            fw.write("gvarU8", np.array(data.U8[0]))
            fw.write("gvarU16", np.array(data.U16[0]))
            fw.write("gvarU32", np.array(data.U32[0]))
            fw.write("gvarU64", np.array(data.U64[0]))
            fw.write("gvarR32", np.array(data.R32[0]))
            fw.write("gvarR64", np.array(data.R64[0]))

        print("i: " + str(i))
        fw.write("steps", "Step:" + str(i))
        fw.write("varI8", data.I8, shape, start, count)
        fw.write("varI16", data.I16, shape, start, count)
        fw.write("varI32", data.I32, shape, start, count)
        fw.write("varI64", data.I64, shape, start, count)
        fw.write("varU8", data.U8, shape, start, count)
        fw.write("varU16", data.U16, shape, start, count)
        fw.write("varU32", data.U32, shape, start, count)
        fw.write("varU64", data.U64, shape, start, count)
        fw.write("varR32", data.R32, shape, start, count)
        fw.write("varR64", data.R64, shape, start, count, endl=True)

comm.Barrier()
# Reader
data = SmallTestData()

with adios2.open("types_np.bp", "r", comm, "HDF5") as fr:
    
    for fr_step in fr:

        step = fr_step.currentstep()
        data.update(rank, step, size)
        
        print("Step: " + str(step))

        step_vars = fr_step.availablevariables()

        for name, info in step_vars.items():
            print("variable_name: " + name)
            for key, value in info.items():
                print("\t" + key + ": " + value)
            print("\n")

        if(step == 0):
            inTag = fr_step.readstring("tag")
            if(inTag != "Testing ADIOS2 high-level API"):
                print("InTag: " + str(inTag))
                raise ValueError('tag read failed')

            inI8 = fr_step.read("gvarI8")
            inI16 = fr_step.read("gvarI16")
            inI32 = fr_step.read("gvarI32")
            inI64 = fr_step.read("gvarI64")
            inU8 = fr_step.read("gvarU8")
            inU16 = fr_step.read("gvarU16")
            inU32 = fr_step.read("gvarU32")
            inU64 = fr_step.read("gvarU64")
            inR32 = fr_step.read("gvarR32")
            inR64 = fr_step.read("gvarR64")

            if(inI8[0] != data.I8[0]):
                raise ValueError('gvarI8 read failed')

            if(inI16[0] != data.I16[0]):
                raise ValueError('gvarI16 read failed')

            if(inI32[0] != data.I32[0]):
                raise ValueError('gvarI32 read failed')

            if(inI64[0] != data.I64[0]):
                raise ValueError('gvarI64 read failed')

            if(inU8[0] != data.U8[0]):
                raise ValueError('gvarU8 read failed')
            
            if(inU16[0] != data.U16[0]):
                raise ValueError('gvarU16 read failed')

            if(inU32[0] != data.U32[0]):
                raise ValueError('gvarU32 read failed')
            
            if(inU64[0] != data.U64[0]):
                raise ValueError('gvarU64 read failed')

            if(inR32[0] != data.R32[0]):
                raise ValueError('gvarR32 read failed')

            if(inR64[0] != data.R64[0]):
                raise ValueError('gvarR64 read failed')

        stepStr = "Step:" + str(step)

        instepStr = fr_step.readstring("steps")
        if(instepStr != stepStr):
            raise ValueError('steps variable read failed: ' +
                             instepStr + " " + stepStr)

        indataI8 = fr_step.read("varI8", start, count)
        if((indataI8 == data.I8).all() is False):
            print("InData: " + str(indataI8))
            print("Data: " + str(data.I8))
            raise ValueError('I8 array read failed')

        indataI16 = fr_step.read("varI16", start, count)
        indataI32 = fr_step.read("varI32", start, count)
        indataI64 = fr_step.read("varI64", start, count)
        indataU8 = fr_step.read("varU8", start, count)
        indataU16 = fr_step.read("varU16", start, count)
        indataU32 = fr_step.read("varU32", start, count)
        indataU64 = fr_step.read("varU64", start, count)
        indataR32 = fr_step.read("varR32", start, count)
        indataR64 = fr_step.read("varR64", start, count)

        if((indataI16 == data.I16).all() is False):
            print("InData: " + str(indataI16))
            print("Data: " + str(data.I16))
            raise ValueError('I16 array read failed')

        if((indataI32 == data.I32).all() is False):
            print("InData: " + str(indataI32))
            print("Data: " + str(data.I32))
            raise ValueError('I32 array read failed')

        if((indataI64 == data.I64).all() is False):
            print("InData: " + str(indataI64))
            print("Data: " + str(data.I64))
            raise ValueError('I64 array read failed')

        if((indataU8 == data.U8).all() is False):
            print("InData: " + str(indataU8))
            print("Data: " + str(data.U8))
            raise ValueError('U8 array read failed')

        if((indataU16 == data.U16).all() is False):
            print("InData: " + str(indataU16))
            print("Data: " + str(data.U16))
            raise ValueError('U16 array read failed')

        if((indataU32 == data.U32).all() is False):
            print("InData: " + str(indataU32))
            print("Data: " + str(data.U32))
            raise ValueError('U32 array read failed')

        if((indataU64 == data.U64).all() is False):
            print("InData: " + str(indataU64))
            print("Data: " + str(data.U64))
            raise ValueError('U64 array read failed')

        if((indataR32 == data.R32).all() is False):
            print("InData: " + str(indataR32))
            print("Data: " + str(data.R32))
            raise ValueError('R32 array read failed')

        if((indataR64 == data.R64).all() is False):
            print("InData: " + str(indataR64))
            print("Data: " + str(data.R64))
            raise ValueError('R64 array read failed')
