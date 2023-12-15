import adios2

import sys
import unittest


DATA_FILENAME = "hello-world-py.bp"

class TestSimpleReadWrite(unittest.TestCase):

    def _write(self, ad, greeting):
        """write a string to a bp file"""
        io = ad.DeclareIO("hello-world-writer")
        var_greeting = io.DefineVariable("Greeting")
        w = io.Open(DATA_FILENAME, adios2.Mode.Write)
        w.BeginStep()
        w.Put(var_greeting, greeting)
        w.EndStep()
        w.Close()
        return 0

    def _read(self, ad):
        """read a string from to a bp file"""
        io = ad.DeclareIO("hello-world-reader")
        r = io.Open(DATA_FILENAME, adios2.Mode.Read)
        r.BeginStep()
        var_greeting = io.InquireVariable("Greeting")
        message = r.Get(var_greeting)
        r.EndStep()
        r.Close()
        return message

    def test_simple_read_write(self):
        """driver function"""
        print("ADIOS2 version {0}".format(adios2.__version__))
        ad = adios2.ADIOS()
        greeting = "Hello World from ADIOS2"
        self._write(ad, greeting)
        message = self._read(ad)
        print("{}".format(message))
        self.assertEqual(greeting, message)
        return 0


if __name__ == '__main__':
    unittest.main()
