# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

"""The ADIOS2 high-level API module"""

from importlib.metadata import PackageNotFoundError, version as _dist_version

try:
    _dist_version("adios2")
    _dist_version("adios2-mpi")
except PackageNotFoundError:
    pass
else:
    raise ImportError(
        "Both 'adios2' and 'adios2-mpi' are installed and conflict with each other. "
        "Uninstall both and reinstall only one of them."
    )

import adios2.bindings

from adios2.adios import *
from adios2.attribute import *
from adios2.engine import *
from adios2.io import *
from adios2.operator import *
from adios2.stream import *
from adios2.variable import *
from adios2.file_reader import *
from adios2.bindings import (
    LocalValueDim,
    Mode,
    ShapeID,
    StepMode,
    StepStatus,
    DerivedVarType,
    Accuracy,
)

__license__ = "Apache-2.0"
__version__ = adios2.bindings.__version__
is_built_with_mpi = adios2.bindings.is_built_with_mpi

# JoinedDim = 2**64 - 2
JoinedDim = 18446744073709551614
