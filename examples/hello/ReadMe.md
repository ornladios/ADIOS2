## ADIOS2 hello examples

The _hello_ examples are meant to introduce you to ADIOS2's IO capabilities and engines.

They can be found in the following subdirectories, and they should be explored in the order that they are listed:

1. [helloWorld](helloWorld): The _helloWorld_ example demonstrates how to write a simple word and read it back using
   ADIOS2's BP engine.
   * Languages: C++, C++ using high-level API, C, Python, Python using high-level API
2. [bpWriter](bpWriter): The _bpWriter_ examples demonstrate how to write a variable to using ADIOS2's BP engine.
   * Languages: C++, C, Fortran, Python
3. [bpReader](bpReader): The _bpReader_ examples demonstrate how to read a 1D/2D/3D variable using ADIOS2's BP engine.
   * Languages: C++, Fortran, Python
4. [bpFWriteCRead](bpFWriteCRead): The _bpFWriteCRead_ example demonstrates how to write a 2D variable with Fortran and
   read a subset of it with C++, and vice versa using ADIOS2's BP engine.
   * Languages: C++, Fortran
5. [bpTimeWriter](bpTimeWriter): The _bpTimeWriter_ example demonstrates how to write two Variables (one is timestep)
   using time aggregation using ADIOS2's BP engine.
   * Languages: C++, Python
6. [bpAttributeWriter](bpAttributeWriter): The _bpAttributeWriter_ example demonstrates how to write attributes using
   ADIOS2's BP engine.
   * Languages: C++
7. [bpFlushWriter](bpFlushWriter): The _bpFlushWriter_ example demonstrates how to flush a variable using ADIOS2's BP
   engine.
   * Languages: C++
8. [bpWriteReadCuda](bpWriteReadCuda): The _bpWriteReadCuda_ example demonstrates how to write and read a variable with
   multiple time steps using ADIOS2's BP engine and leveraging CUDA.
   * Languages: C++
9. [bpWriteReadHip](bpWriteReadHip): The _bpWriteReadHip_ example demonstrates how to write and read a variable with
   multiple time steps using ADIOS2's BP engine and leveraging HIP.
   * Languages: C++
10. [bpWriteReadKokkos](bpWriteReadKokkos): The _bpWriteReadOmp_ example demonstrates how to write and read a variable
    with multiple time steps using ADIOS2's BP engine and leveraging Kokkos.
    * Languages: C++
11. [datamanReader](datamanReader): The _datamanReader_ example demonstrates how to read variables in real-time WAN
    streams using ADIOS's DataMan engine.
    * Languages: C++, Python
12. [datamanWriter](datamanWriter): The _datamanWriter_ example demonstrates how to write variables in real-time WAN
    streams using ADIOS's DataMan engine.
    * Languages: C++, Python
13. [dataspacesReader](dataspacesReader): The _dataspacesReader_ example demonstrates how to read a variable using
    ADIOS2's DATASPACES engine.
    * Languages: C++
14. [dataspacesWriter](dataspacesWriter): The _dataspacesWriter_ example demonstrates how to write a variable using
    ADIOS2's DATASPACES engine.
    * Languages: C++
15. [hdf5Reader](hdf5Reader): The _hdf5Reader_ example demonstrates how to read variables using ADIOS2's HDF5 engine.
    * Languages: C++
16. [hdf5Writer](hdf5Writer): The _hdf5Writer_ example demonstrates how to write variables using ADIOS2's HDF5 engine.
    * Languages: C++
17. [hdf5SubFile](hdf5SubFile): The _hdf5SubFile_ example demonstrates how to write variables using ADIOS2's parallel
    HDF5 engine leveraging the subfile feature.
    * Languages: C++
18. [inlineMWE](inlineMWE): The _inlineMWE_ example demonstrates how to write and read a variable using ADIOS2's inline
    engine.
    * Languages: C++
19. [inlineFWriteCppRead](inlineFWriteCppRead): The _inlineFWriteCppRead_ example demonstrates how to write a 2D
    variable with Fortran and read it back a subset of it with C++ using ADIOS2's inline engine.
    * Languages: C++, Fortran
20. [inlineReaderWriter](inlineReaderWriter): The _inlineReaderWriter_ example demonstrates how to write two Variables
    (one is timestep) using time aggregation and ADIOS2's inline engine.
    * Languages: C++
21. [sstReader](sstReader): The _sstReader_ example demonstrates how to read a variable using ADIOS2's SST engine.
    * Languages: C++
22. [sstWriter](sstWriter): The _sstWriter_ example demonstrates how to write a variable using ADIOS2's SST engine.
    * Languages: C++
23. [skeleton](skeleton): The _skeleton_ example demonstrates how to write and read a variable using an ADIOS2 skeleton
    engine.
    * Languages: C++
