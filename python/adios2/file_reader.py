"""License:
  Distributed under the OSI-approved Apache License, Version 2.0.  See
  accompanying file Copyright.txt for details.
"""
from functools import singledispatchmethod
from adios2 import Stream, IO


# pylint: disable=W0221
class FileReader(Stream):
    """High level implementation of the FileReader class for read Random access mode"""

    def __repr__(self):
        return f"<adios.file named {self._io_name}>"

    @singledispatchmethod
    def __init__(self, path, comm=None):
        super().__init__(path, "rra", comm)

    # e.g. FileReader(io: adios2.IO, path, mode)
    # pylint: disable=E1121
    @__init__.register(IO)
    def _(self, io: IO, path, comm=None):
        super().__init__(io, path, "rra", comm)

    # pylint: enable=E1121

    def variables(self):
        """Returns the list of variables contained in the opened file"""
        return [self._io.inquire_variable(var) for var in self.available_variables()]
