****
Null 
****

The ``Null`` Engine by-passes any heavy I/O operations that other Engines might potentially execute, for example, memory allocations, buffering, transport data movement. Calls to the Null engine would effectively return immediately without doing any effective operations.

The overall goal is to provide a mechanism to isolate an application behavior without the ADIOS 2 footprint. Use this engine to have an idea of the overhead cost of using a certain ADIOS 2 Engine (similar to writing to `/dev/null`) in an application.
