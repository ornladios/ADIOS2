from Adios2PythonTestBase import Adios2PythonTestBase

import numpy
import adios2

# Import this module so the IO factory can find the engine class it needs
from TestPythonEngine import TestPythonEngine

if Adios2PythonTestBase.isUsingMpi():
    from mpi4py import MPI


#
# Create a testcase class with some tests
#
class TestOpenPythonEngineFromPython(Adios2PythonTestBase):
    def testCreateEngine(self):
        rank = 0
        size = 1

        if self.isUsingMpi():
            # MPI
            comm = MPI.COMM_WORLD
            rank = comm.Get_rank()
            size = comm.Get_size()
            adios = adios2.ADIOS(comm)
        else:
            adios = adios2.ADIOS()

        # User data
        myArray = numpy.array([0., 1., 2., 3., 4., 5., 6., 7., 8., 9.])
        Nx = myArray.size

        # ADIOS IO
        bpIO = adios.DeclareIO("BPFile_N2N")

        # ADIOS Variable name, shape, start, offset, constant dims
        ioArray = bpIO.DefineVariable(
            "bpArray", [size * Nx], [rank * Nx], [Nx], True, myArray.dtype)

        # Engine derived class, spawned to start IO operations
        bpIO.EngineType = "PythonEngine"

        # Since we import the TestPythonEngine class directly, we should not
        # need to provide a "PluginModule" parameter
        bpIO.Parameters = {
            "PluginName": "DoNotReallyCare",
            "PluginClass": "TestPythonEngine"
        }

        # ADIOS Engine
        bpFileWriter = bpIO.Open("npArray.bp", adios2.OpenMode.Write)
        bpFileWriter.Write(ioArray, myArray)

        bpFileWriter.Close()


#
# Trigger the tests
#
if __name__ == '__main__':
    Adios2PythonTestBase.main()
