# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

"""ADIOS2 FileReader Python wrapper."""

from functools import singledispatchmethod
from sys import maxsize
from adios2 import bindings, Adios, Stream, IO


class FileReader(Stream):  # noqa: PLR0902
    """High level implementation of the FileReader class for read Random access mode"""

    def __repr__(self):
        return f"<adios.file named {self._io_name}>"

    @singledispatchmethod
    def __init__(self, path: str, comm=None):
        super().__init__(path, "rra", comm)

    # e.g. FileReader(io: adios2.IO, path, mode)
    @__init__.register(IO)
    def _(self, io: IO, path: str, comm=None):
        super().__init__(io, path, "rra", comm)

    # e.g. FileReader(path, metadata)
    @__init__.register(bytes)
    def _(self, metadata: bytes, path: str):
        self._adios = Adios()
        self._io_name = f"stream:{path}:mode:rra"
        self._io = self._adios.declare_io(self._io_name)
        self._mode = bindings.Mode.ReadRandomAccess
        self._read_mode = True
        self._engine = self._io.open_with_metadata(path, metadata)
        self.index = -1
        self.max_steps = maxsize
        self._step_status = bindings.StepStatus.EndOfStream
        self._step_timeout_sec = self.DEFAULT_TIMEOUT_SEC
