# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

from adios2 import Stream, LocalValueDim
from random import randint
import numpy as np

import unittest


class TestStream(unittest.TestCase):
    def test_basic(self):
        print("===========   test_basic ==================")
        with Stream("pythonstreamtest.bp", "w") as s:
            for _ in s.steps(10):
                # Single value string
                s.write("Outlook", "Good")
                # Global array
                s.write(
                    "temp",
                    content=[randint(15, 35), randint(15, 35), randint(15, 35)],
                    shape=[3, 1],
                    start=[0, 0],
                    count=[3, 1],
                )
                # Local Value
                s.write("Wind", [5], shape=[LocalValueDim])
                # Local Array
                s.write("Coords", [38, -46], [], [], [2])
                s.write("humidity", np.random.rand(3, 1))

        with Stream("pythonstreamtest.bp", "r") as s:
            for _ in s.steps():
                minv, maxv = s.minmax("temp")
                print(f"Min/Max: var:temp step {s.current_step()} min/max = {minv}/{maxv}")
                self.assertGreaterEqual(minv, 15)
                self.assertLessEqual(maxv, 35)

                for var_name in s.available_variables():
                    print(f"var:{var_name}\t{s.read(var_name)}")
                    minv, maxv = s.minmax(var_name)
                    print(
                        f"Min/Max: var:{var_name} step {s.current_step()} min/max = {minv}/{maxv}"
                    )
                self.assertEqual(s.read("Wind", block_id=0), 5)
                self.assertEqual(s.read("Coords", block_id=0)[0], 38)
                self.assertEqual(s.read("Coords", block_id=0)[1], -46)
                self.assertEqual(s.read("humidity", block_id=0).ndim, 2)

    def test_8bit_types(self):
        # numpy has no char dtype, so an 8-bit numpy array must land as int8_t
        # or uint8_t according to its own signedness, on every platform.  char
        # is signed on x86 and unsigned on aarch64 Linux, so a binding that
        # matches char against an array dtype claims arrays of the wrong
        # signedness on one of them and the values come back misread.
        print("===========   test_8bit_types ==================")
        i8 = np.array([0, 1, -2, 3, -128, 127, -6, 7], dtype=np.int8)
        u8 = np.array([0, 1, 2, 3, 128, 255, 254, 7], dtype=np.uint8)
        with Stream("pythonstream8bit.bp", "w") as s:
            s.write("i8", i8, [len(i8)], [0], [len(i8)])
            s.write("u8", u8, [len(u8)], [0], [len(u8)])
            s.write("scalar_i8", np.array(i8[4]))
            s.write("scalar_u8", np.array(u8[4]))
            s.write_attribute("attr_i8", i8)
            s.write_attribute("attr_u8", u8)
            self.assertEqual(s.inquire_variable("i8").type(), "int8_t")
            self.assertEqual(s.inquire_variable("u8").type(), "uint8_t")
            self.assertEqual(s.inquire_variable("scalar_i8").type(), "int8_t")
            self.assertEqual(s.inquire_variable("scalar_u8").type(), "uint8_t")

        with Stream("pythonstream8bit.bp", "r") as s:
            for _ in s.steps():
                got_i8 = s.read("i8")
                got_u8 = s.read("u8")
                self.assertEqual(got_i8.dtype, np.int8)
                self.assertEqual(got_u8.dtype, np.uint8)
                self.assertEqual(got_i8.tolist(), i8.tolist())
                self.assertEqual(got_u8.tolist(), u8.tolist())
                self.assertEqual(s.read("scalar_i8"), i8[4])
                self.assertEqual(s.read("scalar_u8"), u8[4])
                self.assertEqual(s.read_attribute("attr_i8").tolist(), i8.tolist())
                self.assertEqual(s.read_attribute("attr_u8").tolist(), u8.tolist())


if __name__ == "__main__":
    unittest.main()
