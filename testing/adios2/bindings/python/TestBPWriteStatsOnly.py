#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# TestBPWriteStatsOnly.py

import unittest
import numpy as np
from adios2 import Adios
import adios2.bindings as bindings

class TestBPWriterStatsOnly(unittest.TestCase):
    def test_exception_at_read(self):
        # User data
        Nx = 10
        Ny = 10

        count = [Nx, Ny]
        start = [0, 0]
        shape = [Nx, Ny]

        temperatures = np.zeros(count, dtype=np.int32)

        for i in range(0, Nx):
            for j in range(0, Ny):
                temperatures[i, j] = i * Nx + j

        adios = Adios()
        with adios.declare_io("ioWriter") as ioWrite:
            varTemperature = ioWrite.define_variable(
                name="temperature2D", content=temperatures,
                start=start, shape=shape, count=count)
            varTemperature.store_stats_only(True)

            with ioWrite.open("TestWriteStatsOnly_py.bp", bindings.Mode.Write) as wStream:
                wStream.put(varTemperature, temperatures)

        with adios.declare_io("ioReader") as ioRead:
            with ioRead.open("TestWriteStatsOnly_py.bp", bindings.Mode.ReadRandomAccess) as rStream:
                var_inTemperature = ioRead.inquire_variable("temperature2D")
                assert var_inTemperature is not None
                inTemperatures = np.zeros(Nx * Ny, dtype=np.int32)
                with self.assertRaises(RuntimeError):
                    rStream.get(var_inTemperature, inTemperatures, bindings.Mode.Sync)


if __name__ == "__main__":
    unittest.main()
