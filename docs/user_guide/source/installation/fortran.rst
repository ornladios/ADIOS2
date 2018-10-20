*****************************
Enabling the Fortran bindings
*****************************

To enable the Fortran bindings in ADIOS2 follow these guidelines:

1. **Minimum requirements:** 

    * Compiler supporting Fortran 90 or more recent standards 
    * If MPI is enabled a MPI Fortran implementation must be installed.

2. **Linking the Fortran bindings:** ``make install`` will generate under the installation directory (from CMAKE_INSTALL_PREFIX) the required library and modules to link ADIOS2 with Fortran applications: 

    * Library (note that libadios2 must also be linked)
      -  ``lib/libadios2_f.*``
      -  ``lib/libadios2.*``
      
    * Modules 
      -  ``include/fortran/*.mod``  

3. ***Module adios2:*** only module required to be used in an application ```use adios```
