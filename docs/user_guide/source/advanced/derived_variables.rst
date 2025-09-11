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
        StatsOnly, // only stats are saved
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
                                adios2::DerivedVarType::StatsOnly);

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

Build ADIOS2 with support for derived variables
-----------------------------------------------

By default the derived variables are ``OFF``. Building ADIOS2 with derived variables turned on requires ``-DADIOS2_USE_Derived_Variables=ON``.


Supported derived operations
----------------------------

In the current implementation, all input variables for a derived operation need to have the same type.

.. list-table:: Supported derived operations
   :header-rows: 1

   * - Operation
     - Input
     - Output
     - Expression
   * - Addition, Subtraction, Multiplication, Division
     - All inputs must have the same dimension. Can work on multiple variables at once.
     - Output variables will have the same type and dimension as the input variables.
     - a+b, add(a,b), a-b, subtract(a,b), a*b, multily(a,b), a/b, divide(a,b)
   * - Sqrt, Power
     - Can only be applied on single variables.
     - Return variables of the same dimension as the input variable, but of type ``long double`` (for ``long double`` input variable) or ``double`` (for the anything else).
     - sqrt(a), pow(a)
   * - Magnitude
     - All inputs must have the same dimension. Can work on multiple variables at once.
     - Output variables will have the same type and dimension as the input variables.
     - magnitude(a, b)
   * - Curl3D
     - All inputs must have the same dimension. Must receive 3 variables.
     - Output variables will have the same type and dimension as the input variables. The shape of the variable will have an extra dimension equal to 3 (e.g. for inputs of shape (d1, d2, d3), the curl variable will have shape (d1, d2, d3, 3))
     - curl(a, b, c)


The math operations in the table above can be combined to create complex derived expressions that are evaluated one by one. The dimensions and types need to correspond to the requirements of each operation (like in the following example).

.. code-block:: text

   expression= "sqrt(curl(a,b,c)) + y"

The variables corresponding to a, b and c need to have the same shape and same type (example ``<int>(d1, d2, d3)``). The curl operation will generate a variable of shape (d1, d2, d3, 3) and the sqrt will generate a double typed variable of shape (d1, d2, d3, 3). For the add operation to be applied, the y variable needs to be of type double and shape (d1, d2, d3, 3).
