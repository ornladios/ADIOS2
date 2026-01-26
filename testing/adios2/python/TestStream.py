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


if __name__ == "__main__":
    unittest.main()
