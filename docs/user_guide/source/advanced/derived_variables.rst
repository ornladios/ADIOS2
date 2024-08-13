#################
Derived variables
#################
   
Derived quantities are obtained by mathematical transformations of primary data, and they typically allow researchers to focus on specific aspects of the simulation.
For instance, in combustion simulations that generate velocity data, calculating the magnitude of the velocity creates a derived variable that effectively identifies areas of high interest, such as regions with intense burning.
Applications can offload the computation of derived variables to ADIOS2.

.. note::
  Examples for defining and storing derived variables can be found in examples/hello/bpStepsWriteReadDerived folder 

The API to define a derived variable on the write side requires providing a math expression over primary variables and the desired type of derived variable.

The math expression defines aliases for ADIOS2 variables that will be used in the expression and math operations over the aliases (example provided below). A list of supported math operations is provided at the bottom of this page.

.. code-block:: c++

    enum class DerivedVarType
    {
        MetadataOnly, // only stats are saved
        StoreData     // data is stored in addition to metadata
    }

There are currently two types of derived variables accepted by ADIOS2, one that saves only stats about the variables (min/max for each block) and one that saves data in addition to the stats, just like a primary variable.

.. code-block:: c++

    auto Ux = bpOut.DefineVariable<float>("var/Ux", {Nx, Ny}, {0, 0}, {Nx, Ny});
    auto Uy = bpOut.DefineVariable<float>("var/Uy", {Nx, Ny}, {0, 0}, {Nx, Ny});
    bpOut.DefineDerivedVariable("derived/magnitude,
                                "x = var/Ux \n"
                                "y = var/Uy \n"
                                "magnitude(x, y)",
                                adios2::DerivedVarType::MetadataOnly);

Derived variables can be defined at any time and are computed (and potentially stored) during the ``EndStep`` operation.

The ``bpls`` utility can identify derived variables and show the math expression when the ``--show-derived`` option. 
Derived variables that store data become primary data once the write operation is done, so they are not identified as derive variables by bpls.

.. code-block:: text

    $ bpls StepsWriteReadDerived.bp --show-derived -l
  float    var/Ux          10*{60000} = 0 / 45
  float    var/Uy          10*{60000} = 0 / 90
  float    derived/magnitude  10*{60000} = 0 / 100.623
    Derived variable with expression: MAGNITUDE({var/Ux},{var/Uy})

.. note::
   Derived variables are currently supported only by the BP5 engine 

#################
Build ADIOS2 with support for derived variables
#################

#################
Supported derived operations 
#################

