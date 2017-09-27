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
adios = adios2.ADIOS(comm, adios2.DebugON)

bpIO = adios.DeclareIO("NPTypes")

# ADIOS Variable name, shape, start, offset, constant dims
# All local variables
varI8 = bpIO.DefineVariable(
    "varI8", [], [], [data.I8.size], adios2.ConstantDims)
varI16 = bpIO.DefineVariable(
    "varI16", [], [], [data.I16.size], adios2.ConstantDims)
varI32 = bpIO.DefineVariable(
    "varI32", [], [], [data.I32.size], adios2.ConstantDims)
varI64 = bpIO.DefineVariable(
    "varI64", [], [], [data.I64.size], adios2.ConstantDims)

varU8 = bpIO.DefineVariable(
    "varUI8", [], [], [data.U8.size], adios2.ConstantDims)
varU16 = bpIO.DefineVariable(
    "varUI16", [], [], [data.U16.size], adios2.ConstantDims)
varU32 = bpIO.DefineVariable(
    "varUI32", [], [], [data.U32.size], adios2.ConstantDims)
varU64 = bpIO.DefineVariable(
    "varUI64", [], [], [data.U64.size], adios2.ConstantDims)

varR32 = bpIO.DefineVariable(
    "varR32", [], [], [data.R32.size], adios2.ConstantDims)

varR64 = bpIO.DefineVariable(
    "varR64", [], [], [data.R64.size], adios2.ConstantDims)


# ADIOS Engine
bpFileWriter = bpIO.Open("npTypes.bp", adios2.ModeWrite)

bpFileWriter.Write(varI8, data.I8)
bpFileWriter.Write(varI16, data.I16)
bpFileWriter.Write(varI32, data.I32)
bpFileWriter.Write(varI64, data.I64)

bpFileWriter.Write(varU8, data.U8)
bpFileWriter.Write(varU16, data.U16)
bpFileWriter.Write(varU32, data.U32)
bpFileWriter.Write(varU64, data.U64)

bpFileWriter.Write(varR32, data.R32)
bpFileWriter.Write(varR64, data.R64)

bpFileWriter.Close()
