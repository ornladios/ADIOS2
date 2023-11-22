from adios2 import FileReader, Stream
from random import randint

import unittest


class TestStream(unittest.TestCase):
    def test_basic(self):
        with Stream("pythonfiletest.bp", "w") as s:
            for _ in s.steps(10):
                if s.current_step() == 0:
                    s.write("Outlook", "Good")
                s.write(
                    "temp",
                    content=[randint(15, 35), randint(15, 35), randint(15, 35)],
                    shape=[3],
                    start=[0],
                    count=[3],
                )

        with FileReader("pythonfiletest.bp") as f:
            self.assertEqual(len(f.all_blocks_info("temp")), 10)
            for var in f.variables():
                if not var.type() == "string":
                    self.assertEqual(var.steps(), 10)
                    var.set_step_selection([0, var.steps()])

                output = f.read(var.name())
                print(f"var:{var.name()} output:{output}")

        with FileReader("pythonfiletest.bp") as f:
            output = f.read("temp", step_selection=[0, 10])
            self.assertEqual(len(output), 30)
            print(f"var:temp output:{output}")

            output = f.read("temp", step_selection=[0, 5])
            self.assertEqual(len(output), 15)
            print(f"var:temp output:{output}")

            output = f.read("temp", start=[0], count=[2], step_selection=[0, 10])
            self.assertEqual(len(output), 20)
            print(f"var:temp output:{output}")


if __name__ == "__main__":
    unittest.main()
