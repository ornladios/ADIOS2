# Adaptable Input / Output System (ADIOS) v2.0
This is v2.0 of the ADIOS I/O system, developed as part of the
U.S. Department of Energy Exascale Computing Program.

## License
ADIOS >= 2.0 is licensed under the Apache License v2.0.  See the accompanying
Copyright.txt for more details.

## Directory layout

* cmake - Project specific CMake modules
* examples - ADIOS Examples
* include - Public header files 
* scripts - Project maintenance and development scripts
* source - Main ADIOS source
* testing - Tests

## Documentation
Please find ADIOS2 software documentation online under the project GitHub wiki:
[https://github.com/ornladios/ADIOS2/wiki](https://github.com/ornladios/ADIOS2/wiki)

To generate Doxygen application programming interface (API) documentation see 
instructions under: [doc/ReadMe.md](doc/ReadMe.md)  

## Getting Started

ADIOS 2.0.0.alpha uses CMake for its build environment.  CMake expects projects 
to use "out-of-source" builds, which means keeping a separate build and source 
directory (different from autotools, which usually uses an in-source build). 

To build ADIOS:

1. Clone the repository:
		
	```
	$ mkdir ADIOS2
	$ cd ADIOS2
	$ git clone https://github.com/ornladios/adios2.git source
	```
		
2. Create a separate build directory:

	```
	$ mkdir build
	```
	
3. ***Configure the project with CMake***. The following options can be specified as `ON` or `OFF` with cmake's `-DVAR=VALUE` syntax, where VAR options are:

	* `ADIOS2_BUILD_SHARED_LIBS` - Build shared libraries (`OFF` for static)
	* `ADIOS2_BUILD_EXAMPLES   ` - Build examples
	* `ADIOS2_BUILD_TESTING    ` - Build test code
	* `ADIOS2_USE_MPI          ` - Enable MPI
	* `ADIOS2_USE_BZip2        ` - Enable [BZip2](http://www.bzip.org/) compression (not implemented)
	* `ADIOS2_USE_ZFP          ` - Enable [ZFP](https://github.com/LLNL/zfp) compression (not implemented)
	* `ADIOS2_USE_ADIOS1       ` - Enable the [ADIOS 1.x](https://www.olcf.ornl.gov/center-projects/adios/) engine
	* `ADIOS2_USE_DataMan      ` - Enable the DataMan engine for WAN transports
	* `ADIOS2_USE_Python       ` - Enable the Python bindings

    ***Important, automatic discovery***: ADIOS 2.0 CMake has an AUTO discovery "ON" default option. If a certain 
    dependency is found in the system installation path (_e.g._ /usr/), not a custom one (_e.g._ /home , /opt ) it will turn on installation for that dependency automatically 

    In addition, the -DCMAKE_VAR frequent options can be selected:
	    * `CMAKE_INSTALL_PREFIX   ` - Prefix location for installation with `make install`, default depends on system (_e.g._ /usr/local)
	    * `CMAKE_BUILD_TYPE       ` - Debug (default, debugging symbols), or Release (compiler optimizations)

	Example:
	```
	$ cd build
	$ cmake -DADIOS_USE_MPI=ON -DCMAKE_BUILD_TYPE=Debug ../ADIOS2
	-- The C compiler identification is GNU 6.3.1
	-- The CXX compiler identification is GNU 6.3.1
	-- Check for working C compiler: /usr/bin/cc
	-- Check for working C compiler: /usr/bin/cc -- works
	-- Detecting C compiler ABI info
	-- Detecting C compiler ABI info - done
	-- Detecting C compile features
	-- Detecting C compile features - done
	-- Check for working CXX compiler: /usr/bin/c++
	-- Check for working CXX compiler: /usr/bin/c++ -- works
	-- Detecting CXX compiler ABI info
	-- Detecting CXX compiler ABI info - done
	-- Detecting CXX compile features
	-- Detecting CXX compile features - done
	-- Found BZip2: /usr/lib64/libbz2.so (found version "1.0.6") 
	-- Looking for BZ2_bzCompressInit
	-- Looking for BZ2_bzCompressInit - found

	ADIOS2 build configuration:
	  C++ Compiler: GNU 6.3.1 
	  /usr/bin/c++
	
	  Installation prefix: /usr/local
	  Features:
	    Library Type: shared
	    Build Type:   Debug
	    Testing: ON
	    MPI:     OFF
	    BZip2:   OFF
	    ADIOS1:  OFF
	    DataMan: OFF
	
	-- Configuring done
	-- Generating done
	-- Build files have been written to: /path/to/adios/build
	$
	```

	You can also use CMake's curses-base UI with `ccmake ../source`.

4. Compile:

	```
	$ make -j8
	```

5. Run tests:

	```
	$ make test
	```

## Developers

To summit changes to ADIOS 2.0: please see the [wiki's](https://github.com/ornladios/ADIOS2/wiki)  
Contributing to ADIOS section, or the local [Contributors Guide](Contributing.md).
