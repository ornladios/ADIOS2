
import sys

# When importing from C++, this path is missing, and thus numpy is
# unable to import the __future__ module.  This is a temporary
# workaround to allow that to happen.
sys.path.append('/usr/lib/python2.7')

import adios2
import numpy      # Required when importing this module from C++

print('Inside TestPythonEngine module')

class TestPythonEngine(adios2.Engine):
    def __init__(self, engineType, io, name, openMode):
        adios2.Engine.__init__(self, engineType, io, name, openMode)
        print('Inside TestPythonEngine python ctor, just called parent ctor')
        
    def DoWrite(self, variable, values):
        print('Inside TestPythonEngine.DoWrite')
        print(variable)
        print(values)

    def Advance(self, timeoutSeconds):
        print('Inside TestPythonEngine.Advance')

    def Close(self, transportIndex):
        print('Inside TestPythonEngine.Close')
