import os

if os.getenv("OMPI_COMM_WORLD_SIZE") is not None:
    try:
        from .adios2_openmpi import *
    except ImportError:
        from .adios2_serial import *
else:
    from .adios2_serial import *
