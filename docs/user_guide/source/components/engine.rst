******
Engine
******

The Engine abstraction component serves as the base interface to the actual IO Systems executing the heavy-load tasks performed when Producing and Consuming data.

Engine functionality works around two concepts from the application point-of-view:

1. Self-describing variables are published and consumed in "steps" in either "File" random-access (all steps are available) or "Streaming" (steps are available as they are produced in a step-by-step fashion).
2. Self-describing variables are published and consumed using a "sync" or "deferred" (lazy evaluation) policy.

.. caution::

   The ADIOS2 "step" is a logical abstraction that means different things depending on the application context. Examples: "time step", "iteration step", "inner loop step", or "interpolation step", "variable section", etc.


Engine API Functions
--------------------

Recall that Engines are created through the ``IO::Open`` function which must contain a Mode (``Write``, ``Read``, ``Append``). Therefore, the current functionalty of all Engines are provided through the basic API functions:

For Publishing data (Write, Append mode)

* ``Put`` : unique polymorphic abstraction used to pass variables data into an adios2 engine. Optionally, adios2 can provide direct access to its buffer memory 
using an overload that returns a span to a variable block (non-owning contiguous memory piece) to be filled out by the application.

Each engine will give a concrete meaning to  each functions signatures, but all of them must follow the same memory contracts to the "data pointer": the memory address itself, and the "data contents": memory bits (values).
   
   -  Put in Deferred or lazy evaluation mode (default and preferred way)
   
      .. code-block:: c++
         
         Put(variable, *data);
         Put(variable, *data, adios2::Mode::Deferred);
         
      Deferred memory contracts: 
      "data pointer" must not be modified (e.g. resize) until first encounter to `PerformPuts``, ``EndStep`` or ``Close``.
      "data contents" might be modified until first encounter to `PerformPuts``, ``EndStep`` or ``Close``, it's recommended practice to set all data contents before Put.
         
   -  Put in Sync mode
   
      .. code-block:: c++
         
         Put(variable, *data, adios2::Mode::Sync);
         
      Sync memory contracts: 
      "data pointer" and "data contents" can be modified after this call.
   
   - Put returning a ``adios2::Variable<T>::Span`` to access adios2 internal buffer

      .. code-block:: c++
         
         // return a span into a block of memory for this variable dimensions filled with default values T()
         adios2::Variable<T>::Span span = Put(variable);
         // return a span into a block of memory for this variable dimensions with memory set to a pre-filled value
         adios2::Variable<T>::Span span = Put(variable, bufferID, fill_value);
         // not returning a span just sets a constant value to a variable block
         Put(variable); // T()
         Put(variable, bufferID, fill_value); 
         
      
      Span memory contracts: 
      "data pointer" must not be modified (e.g. resize) until first encounter to `PerformPuts``, ``EndStep`` or ``Close``.
      span "data contents" must be modified until first encounter to `PerformPuts``, ``EndStep`` or ``Close``
         

* ``PerformsPuts``
   Executes all pending Put calls in deferred mode until this line.


For Consuming data (Read mode)

* ``Get``
   **Default mode: deferred (lazy evaluation).** Data pointer (or array) to memory must not be reused until first encounter with ``PerformPuts``, ``EndStep`` or ``Close``. Use sync mode to populate the data pointer memory immediately. This is enabled by passing the flag ``adios2::Mode::Sync`` as the 3rd argument.

* ``PerformsGets``
   Executes all pending deferred Get calls in deferred mode until this line.

Common Functionality (Write, Read, Append modes)

   * ``BeginStep``      Begin logical step and return status of stream to be read/written.
   * ``EndStep``        End logical step, flush to transports depending on IO parameters and engine default behavior.
   * ``Close``          Close current engine and underlying transports. Engine object can't be used after this.

The following example illustrates the basic API usage in write mode for data generated at each application step:

.. code-block:: c++

   adios2::Engine engine = io.Open("file.bp", adios2::Mode::Write);

   for( size_t i = 0; i < steps; ++i )
   {
      // ... Application *data generation

      engine.BeginStep(); //next "logical" step for this application

      engine.Put(variableT, dataT, adios2::Mode::Sync);
      // dataT memory already subscribed
      // Application can modify its contents

      //deferred functions return immediately (lazy evaluation),
      // dataU, dataV and dataW must not be resued
      //1st batch
      engine.Put(variableU, dataU);
      engine.Put(variableV, dataV);
      // in this case adios2::Mode::Deferred is redundant,
      // as this is the default option
      engine.Put(variableW, dataW, adios2::Mode::Deferred);
      // effectively dataU, dataV, dataW memory subscription is "deferred"
      // until the first call to PerformPuts, EndStep or Close.
      // Application MUST NOT modify the data pointer (e.g. resize memory).
      engine.PerformPuts();
      // dataU, dataV, data4W subscribed
      // Application can modify their contents

      // ... Application modifies dataU, dataV, dataW

      //2nd batch
      engine.Put(variableUi, dataU);
      engine.Put(variableVi, dataV);
      engine.Put(variableWi, dataW);
      // Application MUST NOT modify dataU, dataV and dataW pointers (e.g. resize),
      // optionally data can be modified, but not recommended
      dataU[0] = 10
      dataV[0] = 10
      dataW[0] = 10 

      engine.EndStep();
      // end of current logical step,
      // default behavior: transport data
      // if buffering is not fine-tuned with io.SetParameters

      // dataU, dataV, data4W subscribed
      // Application can modify their contents
   }

   engine.Close();
   // engine is unreachable and all data should be transported
   ...

.. tip::

   Prefer default Deferred (lazy evaluation) functions as they have the potential to group several variables with the trade-off of not being able to reuse the pointers memory space until ``EndStep``, ``Perform``(``Puts``/``Gets``) or ``Close``.
   Only use Sync if you really have to (*e.g.* reuse memory space from pointer).
   ADIOS2 prefers a step-based IO in which everything is known ahead of time when writing an entire step.


.. danger::
   The default behavior of adios2 ``Put`` and ``Get`` calls IS NOT synchronized, but rather deferred.
   It's actually the opposite of ``MPI_Put`` and more like ``MPI_rPut``.
   Do not assume the data pointer is usable after a ``Put`` and ``Get``, before ``EndStep``, ``Close`` or the corresponding ``PerformPuts``/``PerformGets``.
   Be SAFE and consider using the ``adios2::Mode::Sync`` in the 3rd argument.
   Avoid using TEMPORARIES, r-values, and out-of-scope variables in ``Deferred`` mode.


Available Engines
-----------------

A particular engine is set within the IO object that creates it with the ``IO::SetEngine`` function in a case insensitive manner. If the SetEngine function is not invoked the default engine is the ``BPFile`` for writing and reading self-describing bp (binary-pack) files.

+-------------------------+---------+---------------------------------------------+
| Application             | Engine  | Description                                 |
+-------------------------+---------+---------------------------------------------+
| File                    | BP3     | DEFAULT write/read ADIOS2 native bp files   |
|                         |         |                                             |
|                         | HDF5    | write/read interoperability with HDF5 files |
+-------------------------+---------+---------------------------------------------+
| Wide-Area-Network (WAN) | DataMan | write/read TCP/IP streams                   |
+-------------------------+---------+---------------------------------------------+
| Staging                 | SST     | write/read to a "staging" area: *e.g.* RDMA |
+-------------------------+---------+---------------------------------------------+


Engine Polymorphism has a two-fold goal:

1. Each Engine implements an orthogonal IO scenario targeting a use case (e.g. Files, WAN, InSitu MPI, etc) using a simple, unified API.

2. Allow developers to build their own custom system solution based on their particular requirements in the own playground space. Resusable toolkit objects are available inside ADIOS2 for common tasks: bp buffering, transport management, transports, etc.

A class that extends Engine must be thought of as a solution to a range of IO applications. Each engine must provide a list of supported parameters, set in the IO object creating this engine using ``IO::SetParameters, IO::SetParameter``, and supported transports (and their parameters) in ``IO::AddTransport``. Each Engine's particular options are documented in :ref:`Supported Engines`.


