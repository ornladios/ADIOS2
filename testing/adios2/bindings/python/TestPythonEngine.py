
import adios2
import numpy as np

print('Inside TestPythonEngine module')

class TestPythonEngine(adios2.Engine):
    def __init__(self, engineType, io, name, openMode):
        adios2.Engine.__init__(self, engineType, io, name, openMode)
        print('Inside TestPythonEngine python ctor, just called parent ctor')

        # Calls Engine::Init() if no Init() method is defined here
        self.Init()

    # def Init(self):
    #     print('Inside TestPythonEngine Init()')
        
    def DoWrite(self, variable, values):
        print('Inside TestPythonEngine.DoWrite')
        print(variable)
        print(values)

    def Advance(self, timeoutSeconds):
        print('Inside TestPythonEngine.Advance')

    def Close(self, transportIndex):
        print('Inside TestPythonEngine.Close')
