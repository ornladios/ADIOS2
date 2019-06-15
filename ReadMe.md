[![GitHub (pre-)release](https://img.shields.io/github/release/ornladios/adios2/all.svg)]()
[![GitHub license](http://dmlc.github.io/img/apache2.svg)](./LICENSE)


[![Circle CI](https://circleci.com/gh/ornladios/ADIOS2.svg?style=shield)](https://circleci.com/gh/ornladios/ADIOS2)
[![Travis CI](https://travis-ci.org/ornladios/ADIOS2.svg)](https://travis-ci.org/ornladios/ADIOS2)
[![AppVeyor CI](https://ci.appveyor.com/api/projects/status/0s2a3qp57hgbvlhj?svg=true)](https://ci.appveyor.com/project/ornladios/adios2)

[![Coverity Scan Build Status](https://scan.coverity.com/projects/11116/badge.svg)](https://scan.coverity.com/projects/ornladios-adios2)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/6eeb5a8ac3e34d2599cfdea5bdc3390f)](https://www.codacy.com/app/chuckatkins/ADIOS2?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=ornladios/ADIOS2&amp;utm_campaign=Badge_Grade)


# The Adaptable Input Output System (ADIOS) v2.4.0-rc1
This is v2.4.0-rc1 of the ADIOS Input/Output (I/O) system, developed as part of the
United States Department of Energy's Exascale Computing Program.

ADIOS2 is a framework designed for scientific data I/O to publish and subscribe (put/get) data when and where required. ADIOS2 focuses on:

1. **Performance** I/O scalability in high performance computing (HPC) applications.
2. **Adaptability** unified interfaces to allow for several modes of transport (files, memory-to-memory)  
3. **Ease of Use** two-level application programming interface (APIs)
    * Full APIs for HPC applications: C++11, Fortran90, C99, Python 2 and 3 
    * Simplified APIs for data analysis: Python 2 and 3, C++11, Matlab  
    
ADIOS2 would Transport and Transform your data as groups of self-describing variables and attributes across different media (file, wide-area-network, memory-to-memory, etc.) using a common API for all transport modes. ADIOS2 uses range from HPC supercomputers to personal computers and cloud based applications.

In addition, ADIOS2 is:

* **MPI-based** out-of-box MPI-based, can be used in non-MPI serial code.

* **Data Group-based** ADIOS2 favors a deferred/prefetch/grouped variables transport mode by default. Sync mode, one variable at a time, is treated as the special case.

* **Data Step-based** ADIOS2 APIs resembles the actual production/consumption of data in “steps” of variable groups, for either streaming (memory-to-memory) or random-access (file systems) media.

* **Community-oriented** ADIOS2 will always be free and open-source. We invite the community to contribute , see [Contributor's Guidelines to ADIOS 2](Contributing.md). 

* **Easy to use**

## Documentation
Please find [ADIOS2 User Guide at readthedocs](https://adios2.readthedocs.io)

## Directory layout
* cmake - Project specific CMake modules
* examples - ADIOS2 Examples
* scripts - Project maintenance and development scripts
* source - ADIOS2 source  
    * adios2 - source directory for the ADIOS2 library to be installed under install-dir/lib/libadios2.  
    * utils  - source directory for the binary utilities, to be installed under install-dir/bin  
* bindings - public interface language bindings (C++11, C, Fortran, Python and Matlab)
* testing - Tests using [gtest](https://github.com/google/googletest)

## Getting ADIOS2

* From source: [Build and Install ADIOS2 with CMake v3.6 or above](http://adios2.readthedocs.io/en/latest/installation/installation.html)


* As conda packages: [https://anaconda.org/williamfgc](https://anaconda.org/williamfgc)


  Once ADIOS2 is installed refer to: 

* [Linking ADIOS2 in your own projects](https://adios2.readthedocs.io/en/latest/using/using.html)


## Releases

* Latest release: [v2.4.0-rc1](https://github.com/ornladios/ADIOS2/releases/tag/v2.4.0-rc1)

* Previous releases: [https://github.com/ornladios/ADIOS2/releases](https://github.com/ornladios/ADIOS2/releases)

## Reporting Bugs

If you found a bug, please open an [issue on ADIOS2 github repository](https://github.com/ornladios/ADIOS2/issues)

## Contributing

We invite the community to contribute, see [Contributor's Guide to ADIOS 2](Contributing.md) for instructions on how to contribute. 

## License
ADIOS >= 2.0 is licensed under the Apache License v2.0.  See the accompanying
[Copyright.txt](Copyright.txt) for more details.
