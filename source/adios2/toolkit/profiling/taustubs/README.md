# Profiling Interface for ADIOS2

_Note:_ This library in ADIOS is a stub wrapper library for the TAU performance measurement system.  In the near future, we will make it more generic for instrumentation-based tools.  In the meantime, it is TAU specific.

## Todo Items
- [ ] Make the interface generic.
    - [ ] Replace TAU-specific symbols with generic versions that will be implemented by interested measurement libraries (i.e. Score-P). 
    - [ ] New environment variable specifying location of library containing function implementations.

- [ ] Add CMake support for linking in measurement libraries when static linking.

- [ ] Investigate API call to trigger writing of performance data to the ADIOS2 archive (performance data stored with the science data).

## Overview

These files contain a thin stub interface for instrumenting ADIOS2 code.  The interface can be compiled away entirely by setting the `-DADIOS2_USE_Profiling=False` option when running cmake.  The function calls are "stubs" in the form of function pointers, initialized to ```nullptr```. The functions are optionally assigned at runtime using dlopen() (if necessary) and dlsym() calls, as is typical with plugin implementations. If the function pointers have the value ```nullptr```, then this library is a few more instructions than a no-op.  If the function pointers are assigned, the measurement library functions are called to perform the timing measurement.  The symbols are made available to the environment either through ```LD_PRELOAD``` settings or by linking in the measurement library.

Convenience macros are provided for constructing descriptive timer names using pre-processor definitions such as ```__FILE__```, ```__LINE__```, and ```__func__```.  For C++ codes, there are also scoped timers to minimize instrumentation text and ensure timers are stopped in functions with multiple return locations or exceptions that throw outside of scope.

## Known Issues

Because the implementation uses ```libdl.so``` there will be linker warnings.  The current implementation has only been tested with dynamically linked executables.  It is known that static executables that load shared-object measurement libraries with pthread support will crash, because of a known issue with thread local storage at program startup.  If this is the case, link a static version of the measurement library into the final static executable.

## How to instrument with the C API

For C code, you have the option of specifying the timer name or letting the API generate it based on source code location data.

### Timers

Option 1, explicit timer name:

```C
void function_to_time(void) {
    TAU_START("interesting loop");
    ...
    TAU_STOP("interesting loop");
}
```

Option 2, generated timer name:

```C
void function_to_time(void) {
    /* Will generate something like:
     * "function_to_time [{filename.c} {123,0}]"
     */
    TAU_START_FUNC();
    ...
    TAU_STOP_FUNC();
}
```

### Counters

The interface can be used to capture interesting counter values, too:

```C
TAU_SAMPLE_COUNTER("Bytes Written", 1024);
```

### Metadata

The interface can be used to capture interesting metadata:

```C
TAU_METADATA("ADIOS Method", "POSIX");
```

## How to instrument with the C++ API

The C++ API adds additional scoped timers for convenience:

```C++
void function_to_time(void) {
    /* Will generate something like:
     * "function_to_time [{filename.cpp} {123,0}]"
     */
    TAU_SCOPED_TIMER_FUNC();
    ...
}
```

```C++
do {
    TAU_SCOPED_TIMER("While Loop");
    ...
} while (!done);
```

## How to use at runtime

To use the API with TAU, the executable has to be linked dynamically.  Then the executable will be executed with the ```tau_exec``` wrapper script that will preload the TAU libraries and enable additional TAU features.  ```tau_exec``` should be in the user's ```PATH```:

```bash
mpirun -np 4 tau_exec -T mpi,papi,pthread,cupti -ebs -cupti -io ./executable
```

The example above will use a TAU configuration with PAPI, MPI and Pthread support, and will enable event based sampling (-ebs), CUDA (-cupti) and POSIX I/O measurement (-io).
