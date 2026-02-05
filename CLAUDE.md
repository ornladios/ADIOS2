# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## About ADIOS2

ADIOS2 (Adaptable Input/Output System v2) is a high-performance framework for scientific data I/O. It provides unified interfaces for multiple transport modes (files, memory-to-memory, wide-area networks) and is designed for scalability in HPC applications. The codebase is primarily C++14 with bindings for C, Fortran, Python, and Matlab.

## Build System

ADIOS2 uses CMake (minimum version 3.14) with out-of-source builds required.

### Basic Build Commands

```bash
# Create build directory
mkdir build && cd build

# Minimal configuration (no MPI, minimal dependencies)
cmake -DCMAKE_INSTALL_PREFIX=../install \
      -DADIOS2_USE_MPI=OFF \
      -DBUILD_SHARED_LIBS=ON \
      -DCMAKE_BUILD_TYPE=Release \
      ..

# Full configuration with MPI and dependencies
cmake -DCMAKE_INSTALL_PREFIX=../install \
      -DADIOS2_USE_MPI=ON \
      -DADIOS2_USE_HDF5=AUTO \
      -DADIOS2_USE_Python=AUTO \
      -DADIOS2_USE_Fortran=AUTO \
      -DBUILD_TESTING=ON \
      -DADIOS2_BUILD_EXAMPLES=ON \
      -DCMAKE_BUILD_TYPE=Release \
      ..

# Build
cmake --build . -j8

# Install
cmake --build . --target install
```

See `scripts/runconf/runconf.sh` for a reference configuration script.

### Key CMake Options

- `ADIOS2_USE_MPI` - Enable MPI support (ON/OFF/AUTO)
- `ADIOS2_USE_<Feature>` - Enable specific features (HDF5, Python, Fortran, SST, ZFP, SZ, Blosc2, etc.)
- `BUILD_TESTING` - Build test suite (default: OFF)
- `ADIOS2_BUILD_EXAMPLES` - Build examples (default: OFF)
- `CMAKE_BUILD_TYPE` - Debug, Release, RelWithDebInfo, MinSizeRel

## Testing

### Running Tests

```bash
# From build directory
ctest

# Run tests in parallel
ctest -j8

# Run specific test
ctest -R TestBPWriteRead

# Run tests matching pattern
ctest -R "Engine.BP.*"

# Verbose output
ctest -V -R TestName

# Run only BP5 tests
ctest -R "\.BP5$"

# Run MPI tests with specific process count
# (configured via MPIEXEC_MAX_NUMPROCS CMake variable)
```

### Running Individual Test Executables

Test executables accept the engine type as an argument:

```bash
# From build/testing/adios2/engine/bp directory
./TestBPWriteRead BP5
./TestBPWriteRead BP4
./TestBPWriteRead BP3

# MPI tests
mpirun -n 4 ./TestBPWriteReadMPI BP5
```

### Test Organization

- `testing/adios2/engine/` - Engine-specific tests (bp/, sst/, hdf5/, inline/, etc.)
- `testing/adios2/unit/` - Unit tests for components
- `testing/adios2/bindings/` - Language binding tests (C, Fortran, Python)
- `testing/adios2/interface/` - API interface tests
- Tests use GoogleTest framework
- Test naming: `TestBP<Feature>.cpp` for BP engine tests
- CMake macros generate tests for multiple engine versions (BP3, BP4, BP5)

## Code Formatting

ADIOS2 uses **clang-format version 7** with a strict style guide:

```bash
# Format files
clang-format -i SourceFile.cpp SourceFile.h

# Check formatting (returns non-zero if changes needed)
clang-format --dry-run -Werror SourceFile.cpp
```

Key style requirements:
- **100 character line limit** (not 80 as mentioned in Contributing.md - see .clang-format)
- **4-space indentation** (no tabs)
- **Always use braces** for if/for/while blocks, even single-line
- **Braces on new lines** for classes, functions, namespaces, control statements
- Template declarations always break before `<>`

## Architecture Overview

### Core Abstraction Layers (source/adios2/)

1. **core/** - Public API classes that users interact with:
   - `ADIOS` - Factory class, entry point (creates IO objects)
   - `IO` - Factory for Variables, Attributes, Engines
   - `Engine` - Abstract base for all I/O operations
   - `Variable<T>` - Templated data containers with metadata
   - `Attribute<T>` - Metadata annotations
   - `Operator` - Compression/transformation base class
   - `Stream` - High-level streaming interface

2. **engine/** - Concrete Engine implementations:
   - **File engines**: `bp3/`, `bp4/`, `bp5/` (Binary Pack formats), `hdf5/`
   - **Streaming engines**: `sst/` (Sustainable Staging Transport), `ssc/`, `dataman/`, `dataspaces/`
   - **Special purpose**: `inline/` (in-memory), `null/` (no-op), `skeleton/` (template)
   - Each engine inherits from `core::Engine` and implements BeginStep/EndStep, Put/Get operations

3. **toolkit/** - Reusable components shared across engines:
   - `format/` - Serialization (BP3/BP4/BP5 formats, buffer management)
   - `transport/` - Low-level I/O (POSIX, stdio, HTTP, DAOS, shared memory)
   - `transportman/` - Manages multiple transports per engine
   - `aggregator/` - MPI-based aggregation strategies (reduces file count in parallel apps)
   - `profiling/` - I/O timing

4. **helper/** - Utility functions (comm, system, XML/YAML parsing)

5. **operator/** - Compression implementations (Blosc2, BZip2, ZFP, SZ, MGARD, PNG)

### Language Bindings Architecture (bindings/)

- **CXX/** - C++ public API using PIMPL pattern (wraps core:: classes for ABI stability)
- **C/** - Thin C wrapper with opaque handles
- **Fortran/** - ISO_C_BINDING interop layer
- **Python/** - pybind11 bindings (py11*.h files)
- All bindings wrap the same `core::` implementation

### Key Architectural Patterns

**Engine Factory Pattern**: `ADIOS` → creates → `IO` → creates → `Engine`
- IO.Open() returns specific engine type based on configuration
- Engine selection via parameter or runtime detection

**Step-Based Abstraction**: All engines use BeginStep()/EndStep() for iteration
- Unified API for both files (representing time steps) and streams (representing data steps)
- Supports append, update, and read modes

**Deferred vs Synchronous Operations**:
- `Put()/Get()` operations can be deferred (Mode::Deferred) or immediate (Mode::Sync)
- `PerformPuts()/PerformGets()` executes deferred operations
- Batching improves performance by reducing metadata overhead

**Template Separation** (.h / .inl / .tcc / .cpp pattern):
- `.h` - Class declarations only
- `.inl` - Inline template implementations (always included)
- `.tcc` - Template implementations with explicit instantiation
- `.cpp` - Non-template code + explicit instantiations
- Use `ADIOS2_FOREACH_STDTYPE_1ARG` macro to instantiate for all supported types

**Aggregation**: MPI ranks coordinate to reduce file count
- `MPIAggregator` strategies: EveryoneWrites, TwoLevelShm, EveryoneWritesSerial
- Configured via engine parameters: `AggregationType`, `NumAggregators`

**Metadata vs Data Separation** (BP formats):
- Metadata (variable schemas, shapes) separated from data blocks
- Enables efficient queries without reading all data
- BP5 uses FFS (Fast Serialization) for metadata marshaling

### Variable Shapes (Important Concept)

Variables have different `ShapeID` values that determine collective semantics:
- `GlobalValue` - Single value across all ranks (must be identical)
- `GlobalArray` - Multi-dimensional array with global dimensions
- `JoinedArray` - Arrays joined across ranks in first dimension
- `LocalValue` - Per-rank independent values
- `LocalArray` - Per-rank independent arrays

Understanding shapes is critical when working with parallel I/O code.

### BP5 Engine (Current Default)

- Uses FFS for virtual struct-based metadata
- Metadata format documented in `developer_docs/bp5format.md`
- Supports Min/Max statistics, compression operators
- AsyncWrite modes: Guided (metadata-driven), Naive (simple async)
- See `source/adios2/engine/bp5/BP5Engine.h` for base implementation

## Git Workflow

- Never commit directly to master. Always create a named branch for changes.
- Create branches from the user's fork, but ensure they are up-to-date with upstream/master.
- Branch names should be descriptive (e.g., `fix-bp5-thread-safety`, `add-selection-api`).
- Keep commits terse and focused.

## Common Development Tasks

### Adding Tests for BP Engine

1. Create test file: `testing/adios2/engine/bp/TestBP<Feature>.cpp`
2. Use GoogleTest framework with TYPED_TEST for multi-format support
3. Register in `testing/adios2/engine/bp/CMakeLists.txt`:
   ```cmake
   bp_gtest_add_tests_helper(WriteRead MPI_ALLOW)
   ```
4. Macros automatically create BP3/BP4/BP5 variants

### Working with Templates

When adding templated Variable operations:
1. Declare in `.h` with explicit instantiation declarations
2. Implement in `.tcc` file
3. Add explicit instantiation in `.cpp`:
   ```cpp
   #define declare_template(T) template void ClassName::Method<T>();
   ADIOS2_FOREACH_STDTYPE_1ARG(declare_template)
   #undef declare_template
   ```

### Adding New Engine

1. Create directory in `source/adios2/engine/<enginename>/`
2. Inherit from `core::Engine` base class
3. Implement virtual methods: `BeginStep`, `EndStep`, `DoPut`, `DoGet`, `DoClose`, etc.
4. Register in `source/adios2/core/EngineFactory.cpp`
5. Add tests in `testing/adios2/engine/<enginename>/`
6. See `source/adios2/engine/skeleton/` for template

### MPI Considerations

- Check `ADIOS2_HAVE_MPI` in code
- Use `helper::Comm` wrapper (not raw MPI_Comm) in core library
- Tests should work in both MPI and non-MPI builds
- MPI tests use `MPI_ALLOW` or `MPI_ONLY` in CMake macros

## Important Files

- `source/adios2/common/ADIOSTypes.h` - Core type definitions, enums (ShapeID, Mode, etc.)
- `source/adios2/common/ADIOSMacros.h` - Utility macros including ADIOS2_FOREACH_STDTYPE_1ARG
- `source/adios2/core/Engine.h` - Engine base class interface
- `source/adios2/core/IO.h` - IO class managing variables and engines
- `source/adios2/toolkit/format/bp5/BP5Serializer.h` - BP5 metadata serialization
- `cmake/ADIOSFunctions.cmake` - CMake utility functions

## Debugging Tips

- Enable verbose logging: set engine parameter `Verbose=5` (0-5 scale)
- Use `ADIOS2_DEBUG_MODE` CMake option for debug builds
- BP files can be inspected with `bpls` utility: `bpls -lva filename.bp`
- Check aggregation with engine parameter: `Profile=ON` for timing data
- Use `Engine=Null` to test without I/O overhead

## Documentation

- Main docs: https://adios2.readthedocs.io
- Developer docs: `developer_docs/` directory (BP5 format, reader implementation)
- Examples: `examples/` directory organized by category (basics, simulations, use cases)
