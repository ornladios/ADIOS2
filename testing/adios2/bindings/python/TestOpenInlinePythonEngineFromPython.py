from Adios2PythonTestBase import Adios2PythonTestBase

if Adios2PythonTestBase.isUsingMpi():
    from mpi4py import MPI

import numpy
import adios2

MODULE_LEVEL_VARIABLE = 0

###
### Define a new engine type in python
###
class TestInlinePythonEngine(adios2.Engine):
    def __init__(self, engineType, io, name, openMode):
        adios2.Engine.__init__(self, engineType, io, name, openMode)
        print('Inside TestInlinePythonEngine python ctor, just called parent ctor')

        # Calls Engine::Init() if no Init() method is defined here
        self.Init()

    # def Init(self):
    #     print('Inside TestPythonEngine Init()')
        
    def DoWrite(self, variable, values):
        global MODULE_LEVEL_VARIABLE
        MODULE_LEVEL_VARIABLE += 1
        print('Inside TestInlinePythonEngine.DoWrite')
        print(variable)
        print(values)

    def Advance(self, timeoutSeconds):
        print('Inside TestInlinePythonEngine.Advance')

    def Close(self, transportIndex):
        print('Inside TestInlinePythonEngine.Close')

###
### Create a testcase class with some tests
###
class TestOpenInlinePythonEngineFromPython(Adios2PythonTestBase):
    def testCreateEngine(self):
        global MODULE_LEVEL_VARIABLE
        self.assertEqual(MODULE_LEVEL_VARIABLE, 0)
        MODULE_LEVEL_VARIABLE += 1
        self.assertEqual(MODULE_LEVEL_VARIABLE, 1)

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
        bpIO.Parameters = {
            "PluginName": "FirstPythonPlugin",
            "PluginClass": "TestInlinePythonEngine"
        }

        # ADIOS Engine
        bpFileWriter = bpIO.Open("npArray.bp", adios2.OpenMode.Write)
        bpFileWriter.Write(ioArray, myArray)

        self.assertEqual(MODULE_LEVEL_VARIABLE, 2)

        bpFileWriter.Close()

    def testCreateEngineWithWrongName(self):
        if self.isUsingMpi():
            # MPI
            comm = MPI.COMM_WORLD
            adios = adios2.ADIOS(comm)
        else:
            adios = adios2.ADIOS()

        bpIO = adios.DeclareIO("BPFile_N2N")

        bpIO.EngineType = "PythonEngine"
        bpIO.Parameters = {
            "PluginName": "FirstPythonPlugin", 
            "PluginClass": "MissingEngine"
        }

        # Make sure exception is raised due to wrong engine name
        with self.assertRaises(RuntimeError):
            bpFileWriter = bpIO.Open("npArray.bp", adios2.OpenMode.Write)

###
### Trigger the tests
###
if __name__ == '__main__':
    Adios2PythonTestBase.main()
