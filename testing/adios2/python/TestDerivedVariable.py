from adios2 import Stream
from adios2.bindings import DerivedVarType
import unittest
import numpy as np


class TestDerivedVariable(unittest.TestCase):
    FILENAME = "pythontestderivedvariable.bp"
    EXPR = "t = temps\nt+t"
    TEMP = np.array([35, 40, 30, 45], dtype=np.int64)

    def test_01_create_write(self):
        with Stream(self.FILENAME, "w") as f:
            temps = f.io.define_variable("temps", self.TEMP, self.TEMP.shape, [0], self.TEMP.shape)
            dmo = f.io.define_derived_variable("derived/metadataonly", self.EXPR)
            ds = f.io.define_derived_variable(
                "derived/storedata", self.EXPR, DerivedVarType.StoreData
            )
            de = f.io.define_derived_variable(
                "derived/expressionstring", self.EXPR, DerivedVarType.ExpressionString
            )
            f.write("temps", self.TEMP)
            self.assertEqual(temps.name(), "temps")
            self.assertEqual(temps.block_id(), 0)
            self.assertEqual(temps.count(), [4])
            self.assertEqual(temps.shape(), [4])
            self.assertEqual(temps.sizeof(), 8)
            self.assertEqual(dmo.name(), "derived/metadataonly")
            self.assertEqual(dmo.type(), DerivedVarType.MetadataOnly)
            self.assertEqual(ds.name(), "derived/storedata")
            self.assertEqual(ds.type(), DerivedVarType.StoreData)
            self.assertEqual(de.name(), "derived/expressionstring")
            self.assertEqual(de.type(), DerivedVarType.ExpressionString)

    def test_02_create_reader(self):
        with Stream(self.FILENAME, "r") as f:
            for _ in f.steps():
                temps = f.inquire_variable("temps")
                temps_ds = f.inquire_variable("derived/storedata")
                temps_dm = f.inquire_variable("derived/metadataonly")

                self.assertEqual(temps.name(), "temps")
                self.assertEqual(temps.block_id(), 0)
                self.assertEqual(temps.count(), [4])
                self.assertEqual(temps.sizeof(), 8)
                self.assertEqual(temps.steps(), 1)
                self.assertEqual(temps.steps_start(), 0)

                self.assertEqual(temps_ds.name(), "derived/storedata")
                self.assertEqual(temps_ds.block_id(), 0)
                self.assertEqual(temps_ds.count(), [4])
                self.assertEqual(temps_ds.sizeof(), 8)
                self.assertEqual(temps_ds.steps(), 1)
                self.assertEqual(temps_ds.steps_start(), 0)

                self.assertEqual(temps_dm.name(), "derived/metadataonly")
                self.assertEqual(temps_dm.block_id(), 0)
                self.assertEqual(temps_dm.count(), [4])
                self.assertEqual(temps_dm.sizeof(), 8)
                self.assertEqual(temps_dm.steps(), 1)
                self.assertEqual(temps_dm.steps_start(), 0)

                t = f.read("temps", start=[0], count=temps.count())
                if not (t == self.TEMP).all():
                    raise ValueError(
                        "ERROR: Reading 'temps' failed. "
                        f"Data does not match original. data = {t}"
                    )

                ts = f.read("derived/storedata", start=[0], count=temps_ds.count())
                if not (ts == 2 * self.TEMP).all():
                    raise ValueError(
                        "ERROR: Reading 'derived/storedata' failed. "
                        f"Data does not match expected values. data = {ts}"
                    )


if __name__ == "__main__":
    unittest.main()
