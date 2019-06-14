**
IO
**

The IO component is the connection between how applications set up their input/output options by selecting an Engine and its specific parameters, subscribing variables to self-describe the data, and setting supported transport modes to a particular Engine. Think of IO as a control panel for all the user-defined parameters that specific applications would like to be able to fine tune. None of the IO operations are heavyweight until the Open function that generates an Engine is called. Its API allows for:

* Being a factory of Variable and Attribute components containing self-describing information about the overall data in the input output process
* Setting Engine-specific parameters and adding supported modes of transport
* Being a factory of (same type) Engine to execute the actual IO tasks

.. note::
   If two different engine types are needed (*e.g.* BPFile, SST) define two IOs. Also, at reading always define separate IOs to avoid name-clashing of Variables.


.. blockdiag::

   diagram {
      default_fontsize = 17;
      default_shape = roundedbox;
      default_linecolor = blue;
      span_width = 220;

      IO -> Var_1, B, Var_N [label = "DefineVariable<T>",fontsize = 13];
      B [shape = "dots"];
      IO -> B [style = "none"];

      IO -> Att_1, C, Att_N [label = "DefineAttribute<T>",fontsize = 13];
      C [shape = "dots"];
      IO -> C [style = "none"];

      IO -> Transport_1, D, Transport_N [label = "AddTransport",fontsize = 13];
      D [shape = "dots"];
      IO -> D [style = "none"];

      IO -> Engine_1, E, Engine_N [label = "Open",fontsize = 13];
      E [shape = "dots"];
      IO -> E [style = "none"];
   }


Setting a Particular Engine and its Parameters
----------------------------------------------

Engines are the actual components executing the heavy operations in ADIOS2. Each IO must select a type of Engine depending on the application needs through the SetEngine function. The default is the BPFile engine, for writing and reading bp files, if SetEngine is not called.

.. code-block:: c++

   /** Signature */
   void adios2::IO::SetEngine( const std::string engineType );

   /** Example */
   bpIO.SetEngine("BPFileWriter");

Each Engine allows the user to fine tune execution of buffering and output tasks through passing parameters to the IO object that will then propagate to the Engine. For a list of parameters allowed by each engine see :ref:`Available Engines`.

.. note::

   ``adios2::Params`` is an alias to ``std::map<std::string,std::string>`` to pass parameters as key-value string pairs, which can be initialized with curly-brace initializer lists.

.. code-block:: c++

    /** Signature */
    /** Passing several parameters at once */
    void SetParameters(const adios2:Params& parameters);
    /** Passing one parameter key-value pair at a time */
    void SetParameter(const std::string key, const std::string value);

    /** Examples */
    io.SetParameters( { {"Threads", "4"},
                        {"ProfilingUnits", "Milliseconds"},
                        {"MaxBufferSize","2Gb"},
                        {"BufferGrowthFactor", "1.5" }
                        {"FlushStepsCount", "5" }
                      } );
    io.SetParameter( "Threads", "4 );


Adding Supported Transports with Parameters
-------------------------------------------

The AddTransport function returns an unsigned int handler for each transport that can be used with the Engine Close function at different times. AddTransport must provide library specific settings that the low-level system library interface allows. These options are expected to become more complex as new modes of transport are allowed beyond files (*e.g.* RDMA).

.. code-block:: c++

    /** Signature */
    unsigned int AddTransport( const std::string transportType,
                               const adios2::Params& parameters );

    /** Examples */
    const unsigned int file1 = io.AddTransport( "File",
                                                { {"Library", "fstream"},
                                                  {"Name","file1.bp" }
                                                } );

    const unsigned int file2 = io.AddTransport( "File",
                                                { {"Library", "POSIX"},
                                                  {"Name","file2.bp" }
                                                } );

    const unsigned int wan = io.AddTransport( "WAN",
                                              { {"Library", "Zmq"},
                                                {"IP","127.0.0.1" },
                                                {"Port","80"}
                                              } );


Defining, Inquiring and Removing Variables and Attributes
---------------------------------------------------------

The template functions ``DefineVariable<T>`` allows subscribing self-describing data into ADIOS2 by returning a reference to a Variable class object whose scope is the same as the IO object that created it.
The user must provide a unique name (among Variables), the dimensions: MPI global: shape, MPI local: start and offset, optionally a flag indicating that dimensions are known to be constant, and a data pointer if defined in the application.
Note: actual data is not passed at this stage.
This is done by the Engine functions ``Put``/``Get`` for Variables.
See the :ref:`Variable` section for supported types and shapes.

.. tip::
   ``adios2::Dims`` is an alias to ``std::vector<std::size_t>``, while ``adios2::ConstantDims`` is an alias to bool ``true``. Use them for code clarity.

.. code-block:: c++

    /** Signature */
    adios2::Variable<T>
        DefineVariable<T>(const std::string name,
                          const adios2::Dims &shape = {}, // Shape of global object
                          const adios2::Dims &start = {}, // Where to begin writing
                          const adios2::Dims &count = {}, // Where to end writing
                          const bool constantDims = false);

    /** Example */
    /** global array of floats with constant dimensions */
    adios2::Variable<float> varFloats =
        io.DefineVariable<float>("bpFloats",
                                 {size * Nx},
                                 {rank * Nx},
                                 {Nx},
                                 adios2::ConstantDims);

Attributes are extra-information associated with the current IO object. The function ``DefineAttribute<T>`` allows for defining single value and array attributes. Keep in mind that Attributes apply to all Engines created by the IO object and, unlike Variables which are passed to each Engine explicitly, their definition contains their actual data.

.. code-block:: c++

    /** Signatures */

    /** Single value */
    adios2::Attribute<T> DefineAttribute(const std::string &name,
                                  const T &value);

    /** Arrays */
    adios2::Attribute<T> DefineAttribute(const std::string &name,
                                  const T *array,
                                  const size_t elements);

In situations in which a variable and attribute has been previously defined:
1) a variable/attribute reference goes out of scope, or 2) when reading from an incoming stream, IO can inquire the current variables and attributes and return a pointer acting as reference. If the inquired variable/attribute is not found, then ``nullptr`` is returned.

.. code-block:: c++

    /** Signature */
    adios2::Variable<T> InquireVariable<T>(const std::string &name) noexcept;
    adios2::Attribute<T> InquireAttribute<T>(const std::string &name) noexcept;

    /** Example */
    adios2::Variable<float> varPressure = io.InquireVariable<T>("pressure");
    if( varPressure ) // it exists
    {
      ...
    }


.. note::
   The reason for returning a pointer when inquiring, unlike references when defining, is because ``nullptr`` is a valid state (e.g. variables hasn't arrived in a stream, wasn't previously defined or wasn't written in a file).

   Always check for ``nullptr`` in the pointer returned by ``InquireVariable<T>`` or ``InquireAttribute<T>``

.. caution::

   Since Inquire are template functions, name and type must both match the variable/attribute you are looking for.


Removing Variables and Attributes can be done one at a time or by removing all existing variables or attributes in IO.

.. code-block:: c++

    /** Signature */
    bool IO::RemoveVariable(const std::string &name) noexcept;
    void IO::RemoveAllVariables( ) noexcept;

    bool IO::RemoveAttribute(const std::string &name) noexcept;
    void IO::RemoveAllAttributes( ) noexcept;

.. caution::

   Remove functions must be used with caution as they generate dangling Variable/Attributes pointers or references if they didn't go out of scope.

.. tip::

   It is good practice to check the bool flag returned by ``RemoveVariable`` or ``RemoveAttribute``.


Opening an Engine
-----------------

The ``IO::Open`` function creates a new derived object of the abstract Engine class and returns a reference handler to the user. A particular Engine type is set to the current IO component with the ``IO::SetEngine`` function. Engine polymorphism is handled internally by the IO class, which allows subscribing future derived Engine types without changing the basic API.

.. note::

   Currently only ``adios2::Mode:Write`` and ``adios2::Mode::Read`` are supported, ``adios2::Mode::Append`` is under development


.. code-block:: c++

    /** Signatures */
    /** Provide a new MPI communicator other than from ADIOS->IO->Engine */
    adios2::Engine &adios2::IO::Open( const std::string &name,
                                      const adios2::Mode mode,
                                      MPI_Comm mpiComm );

    /** Reuse the MPI communicator from ADIOS->IO->Engine \n or non-MPI serial mode */
    adios2::Engine &adios2::IO::Open(const std::string &name,
                                     const adios2::Mode mode);


    /** Examples */

    /** Engine derived class, spawned to start Write operations */
    adios2::Engine bpWriter = io.Open("myVector.bp", adios2::Mode::Write);

    if(bpWriter) // good practice
    {
      ...
    }


    /** Engine derived class, spawned to start Read operations on rank 0 */
    if( rank == 0 )
    {
        adios2::Engine bpReader = io.Open("myVector.bp",
                                           adios2::Mode::Read,
                                           MPI_COMM_SELF);
        if(bpReader) // good practice
        {
         ...
        }
    }

.. tip::

   It is good practice to always check the validity of each ADIOS object before operating on it using the explicit bool operator.
   ``if( engine ){ }``

.. caution::

   Always pass ``MPI_COMM_SELF`` if an ``Engine`` lives in only one MPI process. ``Open`` and ``Close`` are collective operations.


