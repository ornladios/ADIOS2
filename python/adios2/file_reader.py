"""License:
  Distributed under the OSI-approved Apache License, Version 2.0.  See
  accompanying file Copyright.txt for details.
"""

from adios2.stream import Stream


class FileReader(Stream):
    """High level implementation of the FileReader class for read Random access mode"""

    def __repr__(self):
        return f"<adios.file named {self._io_name}>"

    def __init__(self, path, comm=None, *, engine_type=None, config_file=None, io_name=None):
        super().__init__(
            path, "rra", comm, engine_type=engine_type, config_file=config_file, io_name=io_name
        )

    def variables(self):
        """Returns the list of variables contained in the opened file"""
        return [self._io.inquire_variable(var) for var in self.available_variables()]
