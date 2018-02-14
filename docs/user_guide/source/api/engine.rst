*******
Engines
*******

The Engine abstraction component serves as the base interface to the actual IO Systems executing the heavy-load tasks performed when Producing and Consuming data.

Engine functionality works around two concepts from the application point-of-view:

1. Self-describing variables are published and consumed in "steps" in either "File" mode (all steps are available) or "Streaming" mode (steps are available are they are produced).
2. Self-describing variables are published and consumed using a "sync" or "deferred" (lazy evaluation) policy. 

.. caution::
   
   The ADIOS2 "step" is a logical abstraction that means different things depending on the application context. Examples: "time step", "iteration step", "inner loop step", or "interpolation step", "variable section", etc.
   
   
Engine API Functions
--------------------
   
Recall that Engines are created through the ``IO::Open`` function which must contain a Mode (Write, Read, Append). Therefore, the current functionalty of all Engines are provided through the basic API functions:

For Publishing data (Write, Append mode)

   * **PutSync**    Synchronized variable publishing. Data pointer memory becomes reusable immediately
   * **PutDeferred** Deferred, lazy evaluation publishing. Data pointer memory data must not be reused until first encounter with PerformPuts, EndStep or Close
   * **PerformsPuts**   Execute all pending PutDeferred functions until this call 

For Consuming data (Read mode)

   * **GetSync**        Synchronized variable publishing. Data pointer memory becomes reusable immediately
   * **GetDeferred**    Deferred, lazy evaluation publishing. Data pointer memory data can't be reused until after first PerformGets, EndStep or Close
   * **PerformsGets**   Execute all pending GetDeferred functions until this call

Common Functionality (Write, Read, Append modes)

   * **BeginStep**      Begin logical step and return status of stream to be read/written
   * **EndStep**        End logical step, flush to transports depending on IO parameters
   * **Close**          Close current engine and underlying transports. Engine object can't be used after this.

The following example illustrates the basic API usage in write mode for data generated at each application step:

.. code-block:: c++

   adios2::Engine &engine = io.Open("file.bp", adios2::Mode::Write);

   for( size_t i = 0; i < steps; ++i )
   {
      // ... Application *data generation
      
      engine.BeginStep(); //next "logical" step for this application
      
      engine.PutSync(variableT, dataT);
      // dataT memory already subscribed 
      // Application can modify its contents
   
      //deferred functions return immediately (lazy evaluation)
      //1st batch
      engine.PutDeferred(variableU, dataU);
      engine.PutDeferred(variableV, dataV);
      engine.PutDeferred(variableW, dataW);
      // dataU, dataV, dataW memory subscription is "deferred" 
      // until the first call toPerformPuts, EndStep or Close.      
      // Application MUST NOT modify their contents.
      
      engine.PerformPuts();
      // dataU, dataV, data4W subscribed 
      // Application can modify their contents
      
      // ... Application modifies dataU, dataV, dataW
      
      //2nd batch
      engine.PutDeferred(variableUi, dataU);
      engine.PutDeferred(variableVi, dataV);
      engine.PutDeferred(variableWi, dataW);
      // Application MUST NOT modify dataU, dataV and dataW
      
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

   Use Deferred (lazy evaluation) functions preferably as it has the potential to group several variables with the trade-off of not being able to reuse the pointers memory space until EndStep, Perform(Puts/Gets) or Close. Only use Sync if you have to (*e.g.* reuse memory space from pointer).

.. warning::

   Currently ADIOS2 supports one variable per deferred "batch" (until PerformPuts/Gets, EndStep). Allowing multiple pieces in a "batch" is under progress. Use Sync functions (PutSync, GetSync) in current version as a workaround.


Available Engines
-----------------

A particular engine is set within the IO object that creates it with the ``IO::SetEngine`` function in a case insensitive manner. If the SetEngine function is not invoked the default engine is the **BPFile** for writing and reading self-describing bp (binary-pack) files.
   
+-----------------------------+---------+---------------------------------------------+
| Application                 | Engine  | Description                                 |
+-----------------------------+---------+---------------------------------------------+
| File                        | BPFile  | DEFAULT write/read ADIOS2 native bp files   |
|                             +---------+---------------------------------------------+ 
|                             | ADIOS1  | write/read ADIOS1.x native bp files         |
|                             +---------+---------------------------------------------+
|                             | HDF5    | write/read interoperability with HDF5 files |
+-----------------------------+---------+---------------------------------------------+
| Wide-Area-Network (WAN)     | DataMan | write/read TCP/IP streams                   |
+-----------------------------+---------+---------------------------------------------+
| Staging                     | SST     | write/read to a "staging" area: *e.g.* RDMA |
+-----------------------------+---------+---------------------------------------------+


Engine Polymorphism has a two-fold goal:

1. Each Engine targets orthogonal IO scenarios targeting use cases (e.g. Files, WAN, InSitu MPI, etc) using a simple, unified API.

2. Allow developers to build their own custom system solution based on their particular requirements in the own playground space. Resusable toolkit objects are available inside ADIOS2 for common tasks: bp buffering, transport management, transports, etc.

A class that extends the Engine must be thought of a solution to a range of IO applications. Each engine must provide a list of supported parameters, set in the IO object creating this engine in ``IO::SetParameters, IO::SetParameter``, and supported transports (and their parameters) in ``IO::AddTransport``. Each Engine particular options are documented in Chapter XX.


