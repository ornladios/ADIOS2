"""License:
Distributed under the OSI-approved Apache License, Version 2.0.  See
accompanying file Copyright.txt for details.
"""

from functools import singledispatchmethod
from sys import maxsize
from adios2 import bindings, Adios, Stream, IO


# pylint: disable=W0221
# pylint: disable=R0902   # Too many instance attributes
class FileReader(Stream):
    """High level implementation of the FileReader class for read Random access mode"""

    def __repr__(self):
        return f"<adios.file named {self._io_name}>"

    @singledispatchmethod
    def __init__(self, path: str, comm=None):
        super().__init__(path, "rra", comm)

    # e.g. FileReader(io: adios2.IO, path, mode)
    # pylint: disable=E1121
    @__init__.register(IO)
    def _(self, io: IO, path: str, comm=None):
        super().__init__(io, path, "rra", comm)

    # e.g. FileReader(path, metadata)
    # pylint: disable=E1121
    @__init__.register(bytes)
    def _(self, metadata: bytes, path: str):
        self._adios = Adios()
        self._io_name = f"stream:{path}:mode:rra"
        # pylint: enable=E1121
        self._io = self._adios.declare_io(self._io_name)
        self._mode = bindings.Mode.ReadRandomAccess
        self._read_mode = True
        self._engine = self._io.open_with_metadata(path, metadata)
        self.index = -1
        self.max_steps = maxsize
        self._step_status = bindings.StepStatus.EndOfStream
        self._step_timeout_sec = self.DEFAULT_TIMEOUT_SEC

    # pylint: enable=E1121
