from adios2.adios import Adios
import adios2.bindings as bindings

import unittest


class TestIO(unittest.TestCase):
    def test_io_empty(self):
        adios = Adios()
        adios.declare_io("BPWriter")

    def test_io_define_attribute(self):
        adios = Adios()
        writer = adios.declare_io("BPWriter")
        ts = writer.define_attribute("timestamp", "20231122")
        self.assertIsNot(ts, None)

    def test_io_inquire_attribute(self):
        adios = Adios()
        writer = adios.declare_io("BPWriter")
        ts = writer.define_attribute("timestamp", "20231122")
        coords = writer.define_attribute("coords", "43N74W")
        x = writer.inquire_attribute("coords")
        self.assertNotEqual(ts, coords)
        self.assertNotEqual(ts, x)
        self.assertEqual(coords, x)

    def test_available_attribute(self):
        adios = Adios()
        writer = adios.declare_io("BPWriter")
        writer.define_attribute("timestamp", "20231122")
        writer.inquire_attribute("timestamp")
        self.assertIs(writer.inquire_attribute("coords"), None)

    def test_remove_attribute(self):
        adios = Adios()
        writer = adios.declare_io("BPWriter")
        writer.define_attribute("timestamp", "20231122")
        writer.remove_attribute("timestamp")
        self.assertIs(writer.inquire_attribute("timestamp"), None)

    def test_remove_all_attribute(self):
        adios = Adios()
        writer = adios.declare_io("BPWriter")
        writer.define_attribute("timestamp", "20231122")
        writer.define_attribute("coords", "43N74W")
        writer.remove_all_attributes()
        self.assertIs(writer.inquire_attribute("timestamp"), None)
        self.assertIs(writer.inquire_attribute("coords"), None)

    def test_io_define_variable(self):
        adios = Adios()
        writer = adios.declare_io("BPWriter")
        temp = writer.define_variable("temp")
        self.assertNotEqual(temp, None)

    def test_io_inquire_variable(self):
        adios = Adios()
        writer = adios.declare_io("BPWriter")
        temp = writer.define_variable("temp")
        presure = writer.define_variable("pressure")
        x = writer.inquire_variable("pressure")
        self.assertNotEqual(temp, presure)
        self.assertNotEqual(temp, x)
        self.assertEqual(presure, x)

    def test_available_variable(self):
        adios = Adios()
        writer = adios.declare_io("BPWriter")
        writer.define_variable("temp")
        writer.inquire_variable("temp")
        self.assertIs(writer.inquire_attribute("pressure"), None)

    def test_remove_variable(self):
        adios = Adios()
        writer = adios.declare_io("BPWriter")
        writer.define_variable("temp")
        writer.remove_variable("temp")
        self.assertIs(writer.inquire_attribute("temp"), None)

    def test_remove_all_variable(self):
        adios = Adios()
        writer = adios.declare_io("BPWriter")
        writer.define_variable("temp")
        writer.define_variable("pressure")
        writer.remove_all_variables()
        self.assertIs(writer.inquire_attribute("pressure"), None)
        self.assertIs(writer.inquire_attribute("temp"), None)

    def test_open_engine(self):
        adios = Adios()
        writer = adios.declare_io("BPWriter")
        writer.set_engine("BPFile")
        writer.set_parameter("threads", "2")
        writer.set_parameters({"AsyncOpen": "On", "MaxOpenFilesAtOnce": "512"})
        writer.add_transport("File", {"Library": "POSIX"})
        engine = writer.open("pythontest.bp", bindings.Mode.Write)
        self.assertNotEqual(engine, None)


if __name__ == "__main__":
    unittest.main()
