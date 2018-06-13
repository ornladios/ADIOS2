#!/usr/bin/env python

#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# TestBPWriteTypes.py: test Python numpy types in ADIOS2 File Write
#  Created on: Feb 2, 2017
#      Author: William F Godoy godoywf@ornl.gov


from adios2NPTypes import SmallTestData
from mpi4py import MPI
import adios2


def check_object(adios2_object, name):
    if adios2_object is None:
        raise ValueError(str(name) + ' not found')


def check_name(name, name_list):
    if name not in name_list:
        raise ValueError(str(name) + ' not found in list')


# MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()
Nx = 8

# Start ADIOS
adios = adios2.ADIOS(comm)
ioWriter = adios.DeclareIO("writer")

shape = [size * Nx]
start = [rank * Nx]
count = [Nx]

data = SmallTestData()

# ADIOS Variable name, shape, start, offset, constant dims
# All local variables
varStr = ioWriter.DefineVariable("varStr", data.Str)

varI8 = ioWriter.DefineVariable(
    "varI8", shape, start, count, adios2.ConstantDims, data.I8)
varI16 = ioWriter.DefineVariable(
    "varI16", shape, start, count, adios2.ConstantDims, data.I16)
varI32 = ioWriter.DefineVariable(
    "varI32", shape, start, count, adios2.ConstantDims, data.I32)
varI64 = ioWriter.DefineVariable(
    "varI64", shape, start, count, adios2.ConstantDims, data.I64)

varU8 = ioWriter.DefineVariable(
    "varU8", shape, start, count, adios2.ConstantDims, data.U8)
varU16 = ioWriter.DefineVariable(
    "varU16", shape, start, count, adios2.ConstantDims, data.U16)
varU32 = ioWriter.DefineVariable(
    "varU32", shape, start, count, adios2.ConstantDims, data.U32)
varU64 = ioWriter.DefineVariable(
    "varU64", shape, start, count, adios2.ConstantDims, data.U64)

varR32 = ioWriter.DefineVariable(
    "varR32", shape, start, count, adios2.ConstantDims, data.R32)

varR64 = ioWriter.DefineVariable(
    "varR64", shape, start, count, adios2.ConstantDims, data.R64)

attString = ioWriter.DefineAttribute("attrString", ["hello attribute"])
attI8 = ioWriter.DefineAttribute("attrI8", data.I8)

# ADIOS Engine
writer = ioWriter.Open("npTypes.bp", adios2.Mode.Write)

for i in range(0, 3):

    data.update(rank, i, size)

    writer.Put(varStr, data.Str)
    writer.Put(varI8, data.I8)
    writer.Put(varI16, data.I16)
    writer.Put(varI32, data.I32)
    writer.Put(varI64, data.I64)

    writer.Put(varU8, data.U8)
    writer.Put(varU16, data.U16)
    writer.Put(varU32, data.U32)
    writer.Put(varU64, data.U64)

    writer.Put(varR32, data.R32)
    writer.Put(varR64, data.R64)

writer.Close()

# Start reader
ioReader = adios.DeclareIO("reader")

reader = ioReader.Open("npTypes.bp", adios2.Mode.Read)

attrString = ioReader.InquireAttribute("attrString")
attrI8 = ioReader.InquireAttribute("attrI8")

varStr = ioReader.InquireVariable("varStr")
varI8 = ioReader.InquireVariable("varI8")
varI16 = ioReader.InquireVariable("varI16")
varI32 = ioReader.InquireVariable("varI32")
varI64 = ioReader.InquireVariable("varI64")
varU8 = ioReader.InquireVariable("varU8")
varU16 = ioReader.InquireVariable("varU16")
varU32 = ioReader.InquireVariable("varU32")
varU64 = ioReader.InquireVariable("varU64")
varR32 = ioReader.InquireVariable("varR32")
varR64 = ioReader.InquireVariable("varR64")

check_object(attrString, "attrString")
check_object(attrString, "attrI8")

check_object(varStr, "varStr")
check_object(varI8, "varI8")
check_object(varI16, "varI16")
check_object(varI32, "varI32")
check_object(varI64, "varI64")
check_object(varU8, "varU8")
check_object(varU16, "varU16")
check_object(varU32, "varU32")
check_object(varU64, "varU64")
check_object(varR32, "varR32")
check_object(varR64, "varR64")


attr_names = ["attrString", "attrI8"]
var_names = ["varStr", "varI8", "varI16", "varI32", "varI64",
             "varU8", "varU16", "varU32", "varU64",
                      "varR32", "varR64"]

attributesInfo = ioReader.AvailableAttributes()
for name, info in attributesInfo.items():
    check_name(name, attr_names)
    if rank == 0:
        print("attribute_name: " + name)
        for key, value in info.items():
            print("\t" + key + ": " + value)
        print("\n")

variablesInfo = ioReader.AvailableVariables()
for name, info in variablesInfo.items():
    check_name(name, var_names)
    if rank == 0:
        print("variable_name: " + name)
        for key, value in info.items():
            print("\t" + key + ": " + value)
        print("\n")

# here tests reader data
reader.Close()
