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


# Test data
data = SmallTestData()

comm = MPI.COMM_WORLD
adios = adios2.ADIOS(comm)

bpIO = adios.DeclareIO("NPTypes")

# ADIOS Variable name, shape, start, offset, constant dims
# All local variables
varI8 = bpIO.DefineVariable(
    "varI8", [], [], [data.I8.size], adios2.ConstantDims, data.I8)
varI16 = bpIO.DefineVariable(
    "varI16", [], [], [data.I16.size], adios2.ConstantDims, data.I16)
varI32 = bpIO.DefineVariable(
    "varI32", [], [], [data.I32.size], adios2.ConstantDims, data.I32)
varI64 = bpIO.DefineVariable(
    "varI64", [], [], [data.I64.size], adios2.ConstantDims, data.I64)

varU8 = bpIO.DefineVariable(
    "varUI8", [], [], [data.U8.size], adios2.ConstantDims, data.U8)
varU16 = bpIO.DefineVariable(
    "varUI16", [], [], [data.U16.size], adios2.ConstantDims, data.U16)
varU32 = bpIO.DefineVariable(
    "varUI32", [], [], [data.U32.size], adios2.ConstantDims, data.U32)
varU64 = bpIO.DefineVariable(
    "varUI64", [], [], [data.U64.size], adios2.ConstantDims, data.U64)

varR32 = bpIO.DefineVariable(
    "varR32", [], [], [data.R32.size], adios2.ConstantDims, data.R32)

varR64 = bpIO.DefineVariable(
    "varR64", [], [], [data.R64.size], adios2.ConstantDims, data.R64)

attString = bpIO.DefineAttribute("attString", ["hello attribute"])
attI8 = bpIO.DefineAttribute("attI8", data.I8)

# ADIOS Engine
bpFileWriter = bpIO.Open("npTypes.bp", adios2.Mode.Write)

bpFileWriter.PutSync(varI8, data.I8)
bpFileWriter.PutSync(varI16, data.I16)
bpFileWriter.PutSync(varI32, data.I32)
bpFileWriter.PutSync(varI64, data.I64)

bpFileWriter.PutSync(varU8, data.U8)
bpFileWriter.PutSync(varU16, data.U16)
bpFileWriter.PutSync(varU32, data.U32)
bpFileWriter.PutSync(varU64, data.U64)

bpFileWriter.PutSync(varR32, data.R32)
bpFileWriter.PutSync(varR64, data.R64)

bpFileWriter.Close()
