#!/usr/bin/env python

#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# TestBPWriteTypes.py: test Python numpy types in ADIOS2 File Write
#  Created on: Feb 2, 2017
#      Author: William F Godoy godoywf@ornl.gov


from adios2NPTypes import SmallTestData
import adios2


# Test data
data = SmallTestData()

adios = adios2.ADIOS()

bpIO = adios.DeclareIO("NPTypes")

# ADIOS Variable name, shape, start, offset, constant dims
# All local variables
varI8 = bpIO.DefineVariable(
    "varI8", [], [], [data.I8.size], True, data.I8.dtype)
varI16 = bpIO.DefineVariable(
    "varI16", [], [], [data.I16.size], True, data.I16.dtype)
varI32 = bpIO.DefineVariable(
    "varI32", [], [], [data.I32.size], True, data.I32.dtype)
# FIXME: Uncomment below when 64 bit ints work properly
# varI64 = bpIO.DefineVariable(
#     "varI64", [], [], [data.I64.size], True, data.I64.dtype)

varU8 = bpIO.DefineVariable(
    "varUI8", [], [], [data.U8.size], True, data.U8.dtype)
varU16 = bpIO.DefineVariable(
    "varUI16", [], [], [data.U16.size], True, data.U16.dtype)
varU32 = bpIO.DefineVariable(
    "varUI32", [], [], [data.U32.size], True, data.U32.dtype)
# FIXME: Uncomment below when 64 bit ints work properly
# varU64 = bpIO.DefineVariable(
#     "varUI64", [], [], [data.U64.size], True, data.U64.dtype)

varR32 = bpIO.DefineVariable(
    "varR32", [], [], [data.R32.size], True, data.R32.dtype)

varR64 = bpIO.DefineVariable(
    "varR64", [], [], [data.R64.size], True, data.R64.dtype)


# ADIOS Engine
bpFileWriter = bpIO.Open("npTypes.bp", adios2.OpenMode.Write)

bpFileWriter.Write(varI8, data.I8)
bpFileWriter.Write(varI16, data.I16)
bpFileWriter.Write(varI32, data.I32)
# FIXME: Uncomment below when 64 bit ints work properly
# bpFileWriter.Write(varI64, data.I64)

bpFileWriter.Write(varU8, data.U8)
bpFileWriter.Write(varU16, data.U16)
bpFileWriter.Write(varU32, data.U32)
# FIXME: Uncomment below when 64 bit ints work properly
# bpFileWriter.Write(varU64, data.U64)

bpFileWriter.Write(varR32, data.R32)
bpFileWriter.Write(varR64, data.R64)

# FIXME: Pass transportIndex until the default args works properly
bpFileWriter.Close(-1)
