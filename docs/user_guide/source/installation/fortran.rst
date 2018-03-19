*****************************
Enabling the Fortran bindings
*****************************

To enable the Fortran bindings in ADIOS2 follow these guidelines:

1. **Explicitly passing -DADIOS2_USE_Fortran=ON to CMake:** Fortran bindings must be explicitly turned on.  

2. **Minimum requirements:** 

    * Compiler supporting Fortran 90 or more recent standards 
    * If MPI is enabled a MPI Fortran implementation must be installed.

3. **Linking the Fortran bindings:** `make install` will generate under the installation directory (from CMAKE_INSTALL_PREFIX) the required library and modules to link ADIOS2 with Fortran applications: 

    * Library: installation-dir/lib/libadios2_f.*
    * Modules: installation-dir/include/fortran/*.mod
    