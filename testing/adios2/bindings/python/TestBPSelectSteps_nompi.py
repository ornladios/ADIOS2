#!/usr/bin/env python
#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# TestBPSelectSteps_nompi.py: test step selection by reading in Python
# in ADIOS2 File Write
#  Created on: Jan 29, 2021
#      Author: Dmitry Ganyushin ganyushindi@ornl.gov
import unittest
import shutil
import numpy as np
import adios2

TESTDATA_FILENAME = "steps_int32.bp"

class TestAdiosSelectSteps(unittest.TestCase):
    def setUp(self):
        total_steps = 10
        with adios2.open(TESTDATA_FILENAME, "w") as fh:
            for i in range(total_steps):
                fh.write("step", np.array([i], dtype=np.int32), [1], [0], [1])
                fh.end_step()

    def tearDown(self):
        shutil.rmtree(TESTDATA_FILENAME)

    def test_select_steps_reading_fullAPI(self):
        selected_steps = [3, 5, 7]
        param_string = ",".join([str(i) for i in selected_steps])
        adios = adios2.ADIOS()
        ioReadBP = adios.DeclareIO("hellopy")
        ioReadBP.SetParameter(TESTDATA_FILENAME, param_string)
        fh = ioReadBP.Open(TESTDATA_FILENAME, adios2.Mode.Read)
        var = ioReadBP.InquireVariable("step")
        var.SetStepSelection([0, len(selected_steps)])
        data = np.zeros(len(selected_steps), dtype=np.int32)
        fh.Get(var, data, adios2.Mode.Sync)
        self.assertTrue(all([data[i] == selected_steps[i] for i in range(len(selected_steps))]))

if __name__ == '__main__':
    unittest.main()
