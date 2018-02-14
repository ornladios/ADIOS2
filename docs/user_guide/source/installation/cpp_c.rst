************************************************
Enabling the C++11 native library and C bindings
************************************************

ADIOS2 by default will build and install the native C++11 and C bindings under the libadios2 library.

1. **Minimum requirements:** 

    * C++ compiler supporting C++11
    * If MPI is enabled, a MPI C (not C++ bindings) implementation must be installed.

2. **Linking** `make install` will generate under the installation directory (from CMAKE_INSTALL_PREFIX) the required config file, the C++11 and C libadios2 library, and the single `adios2.h` header to link applications with ADIOS2: 

    * Library: installation-dir/lib/libadios2.*
    * Single Header: installation-dir/include/adios2.h
    * Config: installation-dir/bin/adios2-config
    
    