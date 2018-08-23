********
Variable
********

Self-describing Variables are the atomic unit of data representation in the ADIOS2 library when interacting with applications. Thus, the Variable component is the link between a piece of data coming an application and its self-describing information or metadata. This component handles all application variables classified by data type and shape type.

Each IO holds its own set of Variables, each Variable is identified with a unique name. They are created using the reference from ``IO::DefineVariable<T>`` or retrieve using the pointer from ``IO::InquireVariable<T>`` functions in :ref:`IO`.

Variables Data Types
--------------------

Currently, only primitive types are supported in ADIOS2 with plans to extend to plain-old-data (POD) struct data types. Therefore, acceptable values for T in the Variable<T> (C++ only); are:

.. code-block:: c++

   Data types Variables supported by ADIOS2:

   std::string (As of ADIOS 2.2.0 only used as single values, not arrays)
   char
   signed char  
   unsigned char  
   short  
   unsigned short  
   int  
   unsigned int  
   long int  
   long long int  
   unsigned long int  
   unsigned long long int  
   float  
   double  
   long double  
   std::complex<float>   
   std::complex<double>  


Any fixed width size integer defined in header <cstdint> should map to any of the primitive types above depending on the system. In 64-bit systems: ``uint32_t -> unsigned int``, ``std::int64_t -> long int or long long int``. 

.. tip::
   
   It's recommended to be consistent when using types for portability. If data is defined as a  fixed-width integer, define variables in ADIOS2 using a fixed-width type, *e.g.*  for int32_t data use DefineVariable<int32_t>. Mapping to a primitive variable is already handle automatically by the compiler.


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
         engine.PutDeferred( varNodes, nodes );
         engine.PutDeferred( varFlag, "increased" );
         // ...
      }       

   .. note::
   
      Variables of type string are defined just like global single values. In the current adios2 version multidimensional strings are supported through variables or type `char`.
   

2. **Global Array**: the most common shape used for storing self-describing data used for analysis that lives in several MPI processes. The image below illustrates the definitions of the dimension components in a global array: shape, start, and count. 

   .. image:: http://i66.tinypic.com/1zw15xx.png : alt: my-picture2

   Start and Count can be later modified with the ``Variable::SetSelection`` function if it is not a constant dimensions variable.

   .. warning::
   
      The C++ interface doesn't separate the public API from the private implementation (`PIMPL idiom <https://isocpp.org/blog/2018/01/the-pimpl-pattern-what-you-should-know-bartlomiej-filipek>`_). Users must be careful in accessing the m_Shape, m_Start and m_Count public members directly (*e.g.* ``variable.m_Shape`` or ``variable->m_Shape``). 


3. **Local Single Value**: single value variables that are local to the MPI process. They are defined by passing the ``adios2::LocalValueDim`` enum as follows:  

   .. code-block:: c++

      adios2::Variable<int> varProcessID =
            io.DefineVariable<int>("ProcessID", {adios2::LocalValueDim})   
      //...
      engine.PutDeferred<int>(varProcessID, rank);


4. **Local Array**: single array variables that are local to the MPI process. These are more commonly used to write Checkpoint data, that is later read for Restart. Reading, however, needs to be handled differently: each process' array has to be read separately, using SetSelection per rank. The size of each process selection should be discovered by the reading application by inquiring per-block size information of the variable, and allocate memory accordingly.

  .. image:: http://i64.tinypic.com/732neq.png : alt: my-picture3


5. **Joined Array**: in certain circumstances every process has an array that is different only in one dimension. ADIOS2 allows user to present them as a global array by joining the arrays together. For example, if every process has a table with a different number of rows, and one does not want to do a global communication to calculate the offsets in the global table, one can just write the local arrays and let ADIOS2 calculate the offsets at read time (when all sizes are known by any process). 

   .. code-block:: c++
   
      adios2::Variable<double> varTable = io.DefineVariable<double>(
            "table", {adios2::JoinedDim, Ncolumns}, {}, {Nrows, Ncolumns});

   .. note::
      
      Only one dimension can be joinable, every other dimension must be the same on each process.
 
   .. note: 
      
      The local dimension size in the joinable dimension is allowed to change over time within each processor. However, if the sum of all local sizes changes over time, the result will look like a local array. Since global arrays with changing global dimension over time can only be handled as local arrays in ADIOS2.


.. note::
   
   Constants are not handled separately from step-varying values in ADIOS2. Simply write them only once.


