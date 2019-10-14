********
Variable
********

Self-describing Variables are the atomic unit of data representation in the ADIOS2 library when interacting with applications. Thus, the Variable component is the link between a piece of data coming from an application and its self-describing information or metadata. This component handles all application variables classified by data type and shape type.

Each IO holds its own set of Variables, each Variable is identified with a unique name. They are created using the reference from ``IO::DefineVariable<T>`` or retrieved using the pointer from ``IO::InquireVariable<T>`` functions in :ref:`IO`.

Variables Data Types
--------------------

Currently, only primitive types are supported in ADIOS 2. 
Fixed-width types from `<cinttypes> and <cstdint> <https://en.cppreference.com/w/cpp/types/integer>`_  should be preferred when writing portable code. ADIOS 2 maps primitive "natural" types to its equivalent fixed-width type (e.g. ``int`` -> ``int32_t``). Acceptable values for the type ``T`` in ``Variable<T>`` (this is C++ only, see below for other bindings) along with their preferred fix-width equivalent in 64-bit platforms:

.. code-block:: c++

   Data types Variables supported by ADIOS2 Variable<T>

   std::string (only used for global and local values, not arrays)
   char                      -> int8_t or uint8_t depending on compiler flags
   signed char               -> int8_t 
   unsigned char             -> uint8_t
   short                     -> int16_t
   unsigned short            -> uint16_t
   int                       -> int32_t
   unsigned int              -> uint32_t 
   long int                  -> int32_t or int64_t (Linux)
   long long int             -> int64_t 
   unsigned long int         -> uint32_t or uint64_t (Linux)
   unsigned long long int    -> uint64_t  
   float                     -> always 32-bit = 4 bytes  
   double                    -> always 64-bit = 8 bytes
   long double               -> platform dependent
   std::complex<float>       -> always  64-bit = 8 bytes = 2 * float
   std::complex<double>      -> always 128-bit = 16 bytes = 2 * double

.. tip::

   It's recommended to be consistent when using types for portability. If data is defined as a fixed-width integer, define variables in ADIOS2 using a fixed-width type, *e.g.*  for ``int32_t`` data types use ``DefineVariable<int32_t>``.

.. note::

   C, Fortran APIs: the enum and parameter adios2_type_XXX only provides fixed-width types
   
.. note::

   Python APIs: use the equivalent fixed-width types from numpy. If dtype is not specified, ADIOS 2 would handle numpy defaults just fine as long as primitive types are passed.


Variables Shape Types
---------------------

.. note::
   As of beta release version 2.2.0 local variable reads are not supported, yet. This is work in progress. Please use global arrays and values as a workaround.

ADIOS2 is designed *out-of-the-box* for MPI applications. Thus different application data shape types must be covered depending on their scope within a particular MPI communicator. The shape type is defined at creation from the IO object by providing the dimensions: shape, start, count in the ``IO::DeclareVariable<T>`` template function. The supported Variables by shape types can be classified as:


1. **Global Single Value**: only name is required in their definition. This variables are helpful for storing global information, preferably managed by only one MPI process, that may or may not change over steps: *e.g.* total number of particles, collective norm, number of nodes/cells, etc.

   .. code-block:: c++

      if( rank == 0 )
      {
         adios2::Variable<unsigned int> varNodes = adios2::DefineVariable<unsigned int>("Nodes");
         adios2::Variable<std::string> varFlag = adios2::DefineVariable<std::string>("Nodes flag");
         // ...
         engine.Put( varNodes, nodes );
         engine.Put( varFlag, "increased" );
         // ...
      }

   .. note::

      Variables of type string are defined just like global single values. In the current adios2 version multidimensional strings are supported for fixed size strings through variables of type ``char``.


2. **Global Array**: the most common shape used for storing self-describing data used for analysis that lives in several MPI processes. The image below illustrates the definitions of the dimension components in a global array: shape, start, and count.

   .. image:: https://i.imgur.com/MKwNe5e.png : alt: my-picture2
   
   .. warning::

      Be aware of data ordering in your language of choice (Row-Major or Column-Major) as depicted in the above figure. Data decomposition is done by the application based on their requirements, not by adios2.

   Start and Count local dimensions can be later modified with the ``Variable::SetSelection`` function if it is not a constant dimensions variable.


3. **Local Value**: single value-per-rank variables that are local to the MPI process. They are defined by passing the ``adios2::LocalValueDim`` enum as follows:

   .. code-block:: c++

      adios2::Variable<int> varProcessID =
            io.DefineVariable<int>("ProcessID", {adios2::LocalValueDim})
      //...
      engine.Put<int>(varProcessID, rank);


4. **Local Array**: single array variables that are local to the MPI process. These are more commonly used to write Checkpoint data, that is later read for Restart. Reading, however, needs to be handled differently: each process' array has to be read separately, using SetSelection per rank. The size of each process selection should be discovered by the reading application by inquiring per-block size information of the variable, and allocate memory accordingly.

  .. image:: https://i.imgur.com/XLh2TUG.png : alt: my-picture3


5. **Joined Array (NOT YET SUPPORTED)**: in certain circumstances every process has an array that is different only in one dimension. ADIOS2 allows user to present them as a global array by joining the arrays together. For example, if every process has a table with a different number of rows, and one does not want to do a global communication to calculate the offsets in the global table, one can just write the local arrays and let ADIOS2 calculate the offsets at read time (when all sizes are known by any process).

   .. code-block:: c++

      adios2::Variable<double> varTable = io.DefineVariable<double>(
            "table", {adios2::JoinedDim, Ncolumns}, {}, {Nrows, Ncolumns});

   .. note::

      Only one dimension can be joinable, every other dimension must be the same on each process.

   .. note:

      The local dimension size in the joinable dimension is allowed to change over time within each processor. However, if the sum of all local sizes changes over time, the result will look like a local array. Since global arrays with changing global dimension over time can only be handled as local arrays in ADIOS2.


.. note::

   Constants are not handled separately from step-varying values in ADIOS2. Simply write them only once from one rank.

