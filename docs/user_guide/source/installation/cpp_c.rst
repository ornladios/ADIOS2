********************************************************
Installing the ADIOS2 library and the C++ and C bindings
********************************************************

ADIOS2 by default will build and install the C++11 `libadios2`  library and the C++11, C bindings.

1. **Minimum requirements:** 

    * C++ compiler supporting C++11  
    * If MPI is enabled, a MPI C (not MPI C++ bindings) implementation must be installed.

2. **Linking** `make install` will generate the required header and libraries to link applications with ADIOS2 under the installation directory (from CMAKE_INSTALL_PREFIX): 

    * Libraries: 
      
      - ```lib/libadios2.*```  C++11 and C bindings
    
    * Headers: 
      
      - ```include/adios2.h```       C++11 `namespace adios2`
      - ```include/adios2_c.h```     C  prefix `adios2_`
      
    * Config file: run this command to get installation info 
      
      - ```bin/adios2-config```  
    
    