#!/usr/bin/env python

# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

import unittest
from mpi4py import MPI
import adios2.bindings as adios2

N_STEPS = 3


class TestAdiosWriteReadStringfullAPI(unittest.TestCase):
    def test_write_read_string_fullAPI(self):
        comm = MPI.COMM_WORLD
        theString = "hello adios"
        bpFilename = "string_test_fullAPI.bp"
        varname = "mystringvar"
        adios = adios2.ADIOS(comm)
        ioWrite = adios.DeclareIO("ioWriter")
        adEngine = ioWrite.Open(bpFilename, adios2.Mode.Write)
        varMyString = ioWrite.DefineVariable(varname)
        for step in range(N_STEPS):
            adEngine.BeginStep()
            adEngine.Put(varMyString, theString + str(step))
            adEngine.EndStep()
        adEngine.Close()

        ioRead = adios.DeclareIO("ioReader")
        adEngine = ioRead.Open(bpFilename, adios2.Mode.Read)
        for step in range(N_STEPS):
            adEngine.BeginStep()
            varReadMyString = ioRead.InquireVariable(varname)
            result = adEngine.Get(varReadMyString)
            adEngine.EndStep()
            self.assertEqual(result, theString + str(step))
        adEngine.Close()


if __name__ == "__main__":
    unittest.main()
