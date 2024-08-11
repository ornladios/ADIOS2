from adios2 import Stream, Adios
from adios2.bindings import DerivedVarType
import unittest
import numpy as np


class TestVariable(unittest.TestCase):

    FILENAME = "pythontestvariable.bp"
    TEMP = np.array([35, 40, 30, 45], dtype=np.int64)

    def test_01_create_write(self):
        with Stream(self.FILENAME, "w") as f:
            temps = f.io.define_variable("temps", self.TEMP, self.TEMP.shape, [0], self.TEMP.shape)
            f.write(temps, self.TEMP)
            self.assertEqual(temps.name(), "temps")
            self.assertEqual(temps.block_id(), 0)
            self.assertEqual(temps.count(), [4])
            self.assertEqual(temps.shape(), [4])
            self.assertEqual(temps.sizeof(), 8)

    def test_02_create_reader(self):
        with Stream(self.FILENAME, "r") as f:
            for _ in f.steps():
                temps = f.inquire_variable("temps")
                self.assertEqual(temps.name(), "temps")
                self.assertEqual(temps.block_id(), 0)
                self.assertEqual(temps.count(), [4])
                self.assertEqual(temps.sizeof(), 8)
                self.assertEqual(temps.steps(), 1)
                self.assertEqual(temps.steps_start(), 0)

    def test_03_operators(self):
        adios = Adios()
        op1 = adios.define_operator("noop", "null")
        iow = adios.declare_io("BPWriter")

        with Stream(iow, "pythontestvariableoperators.bp", "w") as f:
            temps = f.io.define_variable("temps", self.TEMP, self.TEMP.shape, [0], self.TEMP.shape)
            temps.add_operation(op1)
            f.write(temps, self.TEMP)

        op2 = adios.define_operator("noop2", "null")
        ior = adios.declare_io("BPReader")
        with Stream(ior, "pythontestvariableoperators.bp", "r") as f:
            for _ in f.steps():
                temps = f.inquire_variable("temps")
                temps.add_operation(op2)


if __name__ == "__main__":
    unittest.main()
