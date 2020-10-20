#!/usr/bin/env python
#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# TestBPWriteReadString.py: test writing/reading Python string type
# in ADIOS2 File Write
#  Created on: Oct 19, 2020
#      Author: Dmitry Ganyushin ganyushindi@ornl.gov
import unittest
from mpi4py import MPI
import adios2

class TestAdiosWriteReadString(unittest.TestCase):

    def TestAdiosWriteReadString(self):
        # MPI
        comm = MPI.COMM_WORLD
        theString = 'hello adios'
        bpFilename = 'string_test.bp'
        varname = 'mystringvar'
        NSteps = 10
        adios = adios2.ADIOS(comm)
        ioWrite = adios.DeclareIO('ioWriter')
        adEngine = ioWrite.Open(bpFilename, adios2.Mode.Write)
        varMyStringVar = ioWrite.DefineVariable(varname)
        for step in range(NSteps):
            adEngine.BeginStep()
            adEngine.Put(varMyStringVar, theString + str(step))
            adEngine.EndStep()
        adEngine.Close()

        ioRead = adios.DeclareIO('ioReader')
        adEngine = ioRead.Open(bpFilename, adios2.Mode.Read)
        varReadMyStringVar = ioRead.InquireVariable(varname)
        for step in range(NSteps):
            adEngine.BeginStep()
            result = adEngine.Get(varReadMyStringVar)
            adEngine.EndStep()
            self.assertEqual(result, theString + str(step))
        adEngine.Close()


if __name__ == '__main__':
    unittest.main()
