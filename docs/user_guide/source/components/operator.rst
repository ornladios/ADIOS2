********
Operator
********

The Operator abstraction allows ADIOS2 to act upon the user application data, either from a ``adios2::Variable`` or a set of Variables in an ``adios2::IO`` object. Current supported operations are:

1. Data compression/decompression, lossy and lossless.
2. Callback functions (C++11 bindings only) supported by specific engines

ADIOS2 enables the use of third-party libraries to execute these tasks.

.. warning::

   Make sure your ADIOS2 library installation used for writing and reading was linked with a compatible version of a third-party dependency when working with operators. ADIOS2 will issue an exception if an operator library dependency is missing.
