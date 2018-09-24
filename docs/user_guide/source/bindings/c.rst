**********
C bindings
**********

The C bindings are specifically designed for C applications and those using an older C++ standard (98 and 03). If you are using a C++11 or more recent standard, please use the C++11 bindings.  




.. note::
    
    use `#include "adios2_c.h"` for the C bindings, `adios2.h` is the C++11 header


adios2_adios handler functions
------------------------------

.. doxygenfile:: adios2_c_adios.h
   :project: C
   :path: ../../bindings/C/c/
   
adios2_io handler functions
---------------------------

.. doxygenfile:: adios2_c_io.h
   :project: C
   :path: ../../bindings/C/c/
   
adios2_variable handler functions
---------------------------------

.. doxygenfile:: adios2_c_variable.h
   :project: C
   :path: ../../bindings/C/c/
   
adios2_attribute handler functions
----------------------------------

.. doxygenfile:: adios2_c_attribute.h
   :project: C
   :path: ../../bindings/C/c/
   
adios2_engine handler functions
-------------------------------

.. doxygenfile:: adios2_c_engine.h
   :project: C
   :path: ../../bindings/C/c/
   
adios2_operator handler functions
---------------------------------

.. doxygenfile:: adios2_c_operator.h
   :project: C
   :path: ../../bindings/C/c/