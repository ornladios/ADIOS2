#!/usr/bin/env python

#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# TestBPWriteTypes.py: test Python numpy types in ADIOS2 File
#                      Write/Read High-Level API
#  Created on: Aug 11, 2019
#      Author: Kai Germaschewski <kai.germaschewski@unh.edu>

import unittest
import adios2
import numpy as np

n_times = 3
n_blocks = 4
filename = 'TestHighLevelAPI.bp'

# The following are the test data, with an additional dimension for timestep
# prepended

global_values = np.arange(n_times)
global_arrays = np.arange(n_times * 2 * 16).reshape(n_times, 2, 16)
local_values = np.arange(n_times * n_blocks).reshape(n_times, n_blocks)
local_arrays = np.arange(n_times * n_blocks * 5 *
                         3).reshape(n_times, n_blocks, 5, 3)


def setUpModule():
    with adios2.open(filename, 'w') as fh:
        for t in range(n_times):
            fh.write('global_value', np.array(global_values[t]))
            fh.write('global_array', global_arrays[t], global_arrays[t].shape,
                     (0, 0), global_arrays[t].shape)
            # We're kinda faking a local array, since the blocks all written
            # from one proc
            for b in range(n_blocks):
                fh.write('local_value', np.array(
                    local_values[t][b]), True)
            for b in range(n_blocks):
                fh.write(
                    'local_array', local_arrays[t][b], (), (),
                    local_arrays[t][b].shape)
            fh.end_step()


class TestReadBasic(unittest.TestCase):
    # FIXME, would be nicer to return scalar (0-d array), as above
    def test_GlobalValue1d(self):
        with adios2.open(filename, 'r') as fh:
            for fh_step in fh:
                t = fh_step.current_step()
                val = fh_step.read('global_value')
                self.assertTrue(val == global_values[t])

    def test_GlobalArray(self):
        with adios2.open(filename, 'r') as fh:
            for fh_step in fh:
                t = fh_step.current_step()
                val = fh_step.read('global_array')
                self.assertTrue(np.array_equal(val, global_arrays[t]))

    def test_LocalValue(self):
        with adios2.open(filename, 'r') as fh:
            for fh_step in fh:
                t = fh_step.current_step()
                val = fh.read('local_value')
                self.assertTrue(np.array_equal(val, local_values[t]))

    def test_LocalArray(self):
        with adios2.open(filename, 'r') as fh:
            for fh_step in fh:
                t = fh_step.current_step()
                for b in range(n_blocks):
                    val = fh_step.read('local_array', b)
                    self.assertTrue(np.array_equal(
                        val, local_arrays[t][b]))


class TestReadSelection(unittest.TestCase):
    def test_GlobalValue(self):
        with adios2.open(filename, 'r') as fh:
            for fh_step in fh:
                t = fh_step.current_step()
                val = fh_step.read("global_value", (), ())
                self.assertTrue(val == global_values[t])

    def test_GlobalArray(self):
        with adios2.open(filename, 'r') as fh:
            for fh_step in fh:
                t = fh_step.current_step()
                val = fh_step.read("global_array", (0, 1), (2, 3))
                self.assertTrue(np.array_equal(
                    val, global_arrays[t][0:2, 1:4]))

    def test_LocalValue(self):
        with adios2.open(filename, 'r') as fh:
            for fh_step in fh:
                t = fh_step.current_step()
                val = fh.read("local_value", (1,), (2,))
                self.assertTrue(np.array_equal(val, local_values[t][1:3]))

    def test_LocalValueDefault(self):
        with adios2.open(filename, 'r') as fh:
            for fh_step in fh:
                t = fh_step.current_step()
                val = fh.read("local_value", (), ())
                self.assertTrue(np.array_equal(val, local_values[t]))

    def test_LocalArray(self):
        with adios2.open(filename, 'r') as fh:
            for fh_step in fh:
                t = fh_step.current_step()
                for b in range(n_blocks):
                    val = fh_step.read("local_array", (1, 1), (4, 2), b)
                    self.assertTrue(np.array_equal(
                        val, local_arrays[t][b][1:6, 1:3]))

    def test_LocalArrayDefault(self):
        with adios2.open(filename, 'r') as fh:
            for fh_step in fh:
                t = fh_step.current_step()
                for b in range(n_blocks):
                    val = fh_step.read("local_array", (), (), b)
                    self.assertTrue(np.array_equal(val, local_arrays[t][b]))


class TestReadStepSelection(unittest.TestCase):
    def test_GlobalValue(self):
        with adios2.open(filename, 'r') as fh:
            val = fh.read("global_value", (), (), 1, 2)
            self.assertTrue(np.array_equal(val, global_values[1:3]))

    def test_GlobalArray(self):
        with adios2.open(filename, 'r') as fh:
            val = fh.read("global_array", (1, 0), (1, 3), 1, 2)
            self.assertTrue(np.array_equal(val, global_arrays[1:3, 1:2, 0:3]))

    def test_LocalValue(self):
        with adios2.open(filename, 'r') as fh:
            val = fh.read("local_value", (1,), (3,), 1, 2)
            self.assertTrue(np.array_equal(val, local_values[1:3, 1:4]))

    def test_LocalValueDefault(self):
        with adios2.open(filename, 'r') as fh:
            val = fh.read("local_value", (), (), 1, 2)
            self.assertTrue(np.array_equal(val, local_values[1:3]))

    def test_LocalArray(self):
        with adios2.open(filename, 'r') as fh:
            for b in range(n_blocks):
                val = fh.read("local_array", (1, 1), (4, 2), 1, 2, b)
                self.assertTrue(np.array_equal(
                    val, local_arrays[1:3, b, 1:5, 1:3]))


if __name__ == '__main__':
    unittest.main()
