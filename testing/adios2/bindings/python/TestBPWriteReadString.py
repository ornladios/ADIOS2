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

    def test_adios_read_string(self):
        # MPI
        comm = MPI.COMM_WORLD
        the_string = 'hello adios'
        bpfilename = 'string_test.bp'
        varname = 'mystringvar'
        N_steps = 10
        adios = adios2.ADIOS(comm)
        ioWrite = adios.DeclareIO('ioWriter')
        ad_engine = ioWrite.Open(bpfilename, adios2.Mode.Write)
        var_mystringvar = ioWrite.DefineVariable(varname)
        for step in range(N_steps):
            ad_engine.BeginStep()
            ad_engine.Put(var_mystringvar, the_string + str(step))
            ad_engine.EndStep()
        ad_engine.Close()

        ioRead = adios.DeclareIO('ioReader')
        ad_engine = ioRead.Open(bpfilename, adios2.Mode.Read)
        var_read_mystringvar = ioRead.InquireVariable(varname)
        for step in range(N_steps):
            ad_engine.BeginStep()
            result = ad_engine.Get(var_read_mystringvar)
            ad_engine.EndStep()
            self.assertEqual(result, the_string + str(step))
        ad_engine.Close()


if __name__ == '__main__':
    unittest.main()
