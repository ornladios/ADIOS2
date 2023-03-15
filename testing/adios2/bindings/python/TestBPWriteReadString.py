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

N_STEPS = 3


class TestAdiosWriteReadStringfullAPI(unittest.TestCase):

    def test_write_read_string_fullAPI(self):
        comm = MPI.COMM_WORLD
        theString = 'hello adios'
        bpFilename = 'string_test_fullAPI.bp'
        varname = 'mystringvar'
        adios = adios2.ADIOS(comm)
        ioWrite = adios.DeclareIO('ioWriter')
        adEngine = ioWrite.Open(bpFilename, adios2.Mode.Write)
        varMyString = ioWrite.DefineVariable(varname)
        for step in range(N_STEPS):
            adEngine.BeginStep()
            adEngine.Put(varMyString, theString + str(step))
            adEngine.EndStep()
        adEngine.Close()

        ioRead = adios.DeclareIO('ioReader')
        adEngine = ioRead.Open(bpFilename, adios2.Mode.Read)
        for step in range(N_STEPS):
            adEngine.BeginStep()
            varReadMyString = ioRead.InquireVariable(varname)
            result = adEngine.Get(varReadMyString)
            adEngine.EndStep()
            self.assertEqual(result, theString + str(step))
        adEngine.Close()

    def test_write_read_string_highAPI(self):
        comm = MPI.COMM_WORLD
        theString = 'hello adios'
        bpFilename = 'string_test_highAPI.bp'
        varname = 'mystringvar'

        with adios2.open(bpFilename, "w", comm) as fh:

            for step in range(N_STEPS):
                fh.write(varname, theString + str(step), end_step=True)

        with adios2.open(bpFilename, "r", comm) as fh:
            for fstep in fh:
                step = fstep.current_step()
                result = fstep.read_string(varname)
                self.assertEqual(result, [theString + str(step)])

    def test_read_strings_all_steps(self):
        comm = MPI.COMM_WORLD
        fileName = 'string_test_all.bp'
        with adios2.open(fileName, "w", comm) as fh:
            for i in range(N_STEPS):
                fh.write("string_variable", "written {}".format(i))
                fh.end_step()

        with adios2.open(fileName, "rra", comm) as fh:
            n = fh.steps()
            name = "string_variable"
            result = fh.read_string(name, 0, n)
            expected_str = ["written {}".format(i) for i in range(n)]
            self.assertEqual(result, expected_str)


if __name__ == '__main__':
    unittest.main()
