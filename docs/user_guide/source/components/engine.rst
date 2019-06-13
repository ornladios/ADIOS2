******
Engine
******

The Engine abstraction component serves as the base interface to the actual IO Systems executing the heavy-load tasks performed when Producing and Consuming data.

Engine functionality works around two concepts from the application's point-of-view:

1. Self-describing variables are published and consumed in "steps" in either "File" random-access (all steps are available) or "Streaming" (steps are available as they are produced in a step-by-step fashion).
2. Self-describing variables are published (Put) and consumed (Get) using a "sync" or "deferred" (lazy evaluation) policy.

.. caution::

   The ADIOS 2 "step" is a logical abstraction that means different things depending on the application context. Examples: "time step", "iteration step", "inner loop step", or "interpolation step", "variable section", etc. It only indicates how the variables were passed into ADIOS 2 (e.g. I/O steps) without the user having to index this information on their own.

.. tip::
   
   Publishing/Consuming data can be seen as a round-trip in ADIOS 2. Put and Get APIs for write/append and read modes aim to be "symmetric". Hence, reusing similar functions, objects, semantics as much as possible.  

The rest of the section explains the important concepts 

BeginStep
---------
       
   Begin logical step and return the status (via an enum) of the stream to be read/written. In streaming engines BeginStep in where the receiver tries to acquire a new step in the reading process. The full signature allows for a mode and timeout parameters. See :ref:`Supported Engines` for more information on what engine allows. A simplified signature allows each engine to pick reasonable defaults.
   
.. code-block:: c++

   // Full signature
   StepStatus BeginStep(const StepMode mode,
                        const float timeoutSeconds = -1.f); 
   
   // Simplified signature
   StepStatus BeginStep();

EndStep
-------
        
   Ends logical step, flush to transports depending on IO parameters and engine default behavior.


.. tip::
   
   To write portable code for a step-by-step access across adios2 engines (file and streaming engines) use BeginStep and EndStep. 

.. danger:: 
   
   Accessing random steps in read mode (e.g. Variable<T>::SetStepSelection in file engines) will create a conflict with BeginStep and EndStep and will throw an exception. In file engines, data is either consumed in a random-access or step-by-step mode, but not both.  


Close
-----

   Close current engine and underlying transports. Engine object can't be used after this call.
   
.. tip::
   
   C++11: despite the fact that we use RAII, always use Close for explicit resource management and guarantee that the Engine data transport operations are concluded. 


Put: modes and memory contracts
-------------------------------

``Put`` is the generalized abstract function for publishing data in adios2 when an Engine is created using Write, or Append, mode at ``IO::Open``. Optionally, adios2 Engines can provide direct access to its buffer memory using an overload that returns a span to a variable block (based on a subset of the upcoming `C++20 std::span <https://en.cppreference.com/w/cpp/container/span>`_, non-owning contiguous memory piece). Spans act as a 1D memory container meant to be filled out by the application. See :ref:`Supported Engines` for engines that support the span feature (e.g. BP3).

The following are the current Put signatures:

1. Deferred (default) or Sync mode, data is contiguous memory 

   .. code-block:: c++

      Put(Variable<T> variable, const T* data, const adios2::Mode = adios2::Mode::Deferred);
   
2. Return a span setting a default T() value into a default buffer
 
   .. code-block:: c++
   
      Variable<T>::Span Put(Variable<T> variable);
   
3. Return a span setting a constant fill value into a certain buffer

   .. code-block:: c++

      Variable<T>::Span Put(Variable<T> variable, const size_t bufferID, const T fillValue);


The following table summarizes the memory contracts required by adios2 engines between Put signatures and the data memory coming from an application:

+----------+-------------+----------------------------------------------------+
| Put      | Data Memory | Contract                                           |
+----------+-------------+----------------------------------------------------+
|          | Pointer     | do not modify until PerformPuts/EndStep/Close      |
| Deferred |             |                                                    |
|          | Contents    | consumed at PerformPuts/EndStep/Close              |
+----------+-------------+----------------------------------------------------+
|          | Pointer     | modify after Put                                   |
| Sync     |             |                                                    |
|          | Contents    | consumed at Put                                    |
+----------+-------------+----------------------------------------------------+
|          | Pointer     | modified by new Spans, updated span iterators/data |
| Span     |             |                                                    |
|          | Contents    | consumed at PerformPuts/EndStep/Close              |
+----------+-------------+----------------------------------------------------+


.. note::

   In Fortran (array) and Python (numpy array) avoid operations that modify the internal structure of an array (size) to preserve the address. 
   
   
Each Engine will give a concrete meaning to  each functions signatures, but all of them must follow the same memory contracts to the "data pointer": the memory address itself, and the "data contents": memory bits (values).
   
1. **Put in Deferred or lazy evaluation mode (default)**: this is the preferred mode as it allows Put calls to be "grouped" before potential data transport at the first encounter of ``PerformPuts``, ``EndStep`` or ``Close``.
   
     .. code-block:: c++
         
         Put(variable, data);
         Put(variable, data, adios2::Mode::Deferred);
         

   Deferred memory contracts: 
      
   - "data pointer" do not modify (e.g. resize) until first call to ``PerformPuts``, ``EndStep`` or ``Close``.
      
   - "data contents" consumed at first call to ``PerformPuts``, ``EndStep`` or ``Close``. It's recommended practice to set all data contents before Put.


   Usage:

      .. code-block:: c++
         
         // recommended use: 
         // set "data pointer" and "data contents"
         // before Put
         data[0] = 10;  
         
         // Puts data pointer into adios2 engine
         // associated with current variable metadata
         engine.Put(variable, data);
         
         // valid but not recommended
         // risk of changing "data pointer" (e.g. resize) 
         data[1] = 10; 
         
         // "data contents" must be ready
         // "data pointer" must be the same as in Put
         engine.EndStep();   
         //engine.PerformPuts();  
         //engine.Close();
         
         // now data pointer can be reused or modified
        
   .. tip::

      It's recommended practice to set all data contents before Put in deferred mode to minimize the risk of modifying the data pointer (not just the contents) before PerformPuts/EndStep/Close.


2.  **Put in Sync mode**: this is the special case, data pointer becomes reusable right after Put. Only use it if absolutely necessary (*e.g.* memory bound application or out of scope data, temporary).
   
      .. code-block:: c++
         
         Put(variable, *data, adios2::Mode::Sync);
         

   Sync memory contracts:
      
   - "data pointer" and "data contents" can be modified after this call.
   
   
   Usage:

      .. code-block:: c++
         
         // set "data pointer" and "data contents"
         // before Put in Sync mode
         data[0] = 10;  
         
         // Puts data pointer into adios2 engine
         // associated with current variable metadata
         engine.Put(variable, data, adios2::Mode::Sync);
         
         // data pointer and contents can be reused
         // in application 
   
   
3. **Put returning a Span**: signature that allows access to adios2 internal buffer. 

   Use cases: 
   
   -  population from non-contiguous memory structures
   -  memory-bound applications 


   Limitations:
   
   -  does not allow operations (compression)
   -  must keep engine and variables within scope of span usage 
     


   Span memory contracts: 
      
   - "data pointer" provided by the engine and returned by span.data(), might change with the generation of a new span. It follows iterator invalidation rules from std::vector. Use `span.data()` or iterators, `span.begin()`, `span.end()` to keep an updated data pointer.
      
   - span "data contents" are published at the first call to ``PerformPuts``, ``EndStep`` or ``Close``


   Usage:

       .. code-block:: c++
         
         // return a span into a block of memory
         // set memory to default T()
         adios2::Variable<int32_t>::Span span1 = Put(var1);
         
         // just like with std::vector::data()
         // iterator invalidation rules
         // dataPtr might become invalid
         // always use span1.data() directly
         T* dataPtr = span1.data();
         
         // set memory value to -1 in buffer 0
         adios2::Variable<float>::Span span2 = Put(var2, 0, -1);

         // not returning a span just sets a constant value 
         Put(var3);
         Put(var4, 0, 2);
         
         // fill span1
         span1[0] = 0;
         span1[1] = 1;
         span1[2] = 2;
         
         // fill span2
         span2[1] = 1;
         span2[2] = 2;
         
         // here collect all spans
         // they become invalid
         engine.EndStep();
         //engine.PerformPuts();  
         //engine.Close();
         
         // var1 = { 0, 1, 2 };
         // var2 = { -1., 1., 2.};
         // var3 = { 0, 0, 0};
         // var4 = { 2, 2, 2};


PerformsPuts
------------
   
   Executes all pending Put calls in deferred mode ad collect spans data


Get: modes and memory contracts
-------------------------------

``Get`` is the generalized abstract function for consuming data in adios2 when an Engine is created using Read mode at ``IO::Open``. ADIOS 2 Put and Get APIs semantics are as symmetric as possible considering that they are opposite operations (*e.g.* Put passes ``const T*``, while Get populates a non-const ``T*``). 

..
   Optionally, adios2 Engines can provide direct access to its buffer memory using an overload that returns a span to a variable block (base on a subset of the upcoming `C++20 std::span <https://en.cppreference.com/w/cpp/container/span>`_, non-owning contiguous memory piece). Spans act as a 1D memory container meant to be filled out by the application. See :ref:`Supported Engines` for engines that support the span feature (e.g. BP3).

The following are the current Get signatures:

1. Deferred (default) or Sync mode, data is contiguous pre-allocated memory 

   .. code-block:: c++

      Get(Variable<T> variable, const T* data, const adios2::Mode = adios2::Mode::Deferred);
      
      
2. C++11 only, dataV is automatically resized by adios2 based on Variable selection

   .. code-block:: c++
   
      Get(Variable<T> variable, std::vector<T>& dataV, const adios2::Mode = adios2::Mode::Deferred);
   
   
The following table summarizes the memory contracts required by adios2 engines between Get signatures and the pre-allocated (except when using C++11 ``std::vector``) data memory coming from an application:

+----------+-------------+-----------------------------------------------+
| Get      | Data Memory | Contract                                      |
+----------+-------------+-----------------------------------------------+
|          | Pointer     | do not modify until PerformPuts/EndStep/Close |
| Deferred |             |                                               |
|          | Contents    | populated at PerformPuts/EndStep/Close        |
+----------+-------------+-----------------------------------------------+
|          | Pointer     | modify after Put                              |
| Sync     |             |                                               |
|          | Contents    | populated at Put                              |
+----------+-------------+-----------------------------------------------+


1. **Get in Deferred or lazy evaluation mode (default)**: this is the preferred mode as it allows Get calls to be "grouped" before potential data transport at the first encounter of ``PerformPuts``, ``EndStep`` or ``Close``.
   
     .. code-block:: c++
         
         Get(variable, data);
         Get(variable, data, adios2::Mode::Deferred);
         

   Deferred memory contracts: 
      
   - "data pointer": do not modify (e.g. resize) until first call to ``PerformPuts``, ``EndStep`` or ``Close``.
      
   - "data contents": populated at first call to ``PerformPuts``, ``EndStep`` or ``Close``.

   Usage:

      .. code-block:: c++
         
         std::vector<double> data;
         
         // resize memory to expected size 
         data.resize(varBlockSize);
         // valid if all memory is populated 
         // data.reserve(varBlockSize);
         
         // Gets data pointer to adios2 engine
         // associated with current variable metadata
         engine.Get(variable, data.data() );
         
         // optionally pass data std::vector 
         // leave resize to adios2
         //engine.Get(variable, data);
         
         // "data contents" must be ready
         // "data pointer" must be the same as in Get
         engine.EndStep();   
         //engine.PerformPuts();  
         //engine.Close();
         
         // now data pointer can be reused or modified
        
   .. caution::

      Use uninitialized memory at your own risk (e.g. vector reserve, new, malloc). Accessing unitiliazed memory is undefined behavior.


2.  **Put in Sync mode**: this is the special case, data pointer becomes reusable right after Put. Only use it if absolutely necessary (*e.g.* memory bound application or out of scope data, temporary).
   
      .. code-block:: c++
         
         Get(variable, *data, adios2::Mode::Sync);
         

   Sync memory contracts:
      
   - "data pointer" and "data contents" can be modified after this call.
   
   
   Usage:

      .. code-block:: c++
         
         .. code-block:: c++
         
         std::vector<double> data;
         
         // resize memory to expected size 
         data.resize(varBlockSize);
         // valid if all memory is populated 
         // data.reserve(varBlockSize);
         
         // Gets data pointer to adios2 engine
         // associated with current variable metadata
         engine.Get(variable, data.data() );
         
         // "data contents" are ready
         // "data pointer" can be reused by the application

.. note::
   
   As of v2.4 Get doesn't support returning spans. This is future work required in streaming engines if the application wants a non-owning view into the data buffer for a particular variable block.


PerformsGets
------------
   
   Executes all pending Get calls in deferred mode
   

Engine usage example
--------------------

The following example illustrates the basic API usage in write mode for data generated at each application step:

.. code-block:: c++

   adios2::Engine engine = io.Open("file.bp", adios2::Mode::Write);

   for( size_t i = 0; i < steps; ++i )
   {
      // ... Application *data generation

      engine.BeginStep(); //next "logical" step for this application

      engine.Put(varT, dataT, adios2::Mode::Sync);
      // dataT memory already consumed by engine
      // Application can modify dataT address and contents
      
      // deferred functions return immediately (lazy evaluation),
      // dataU, dataV and dataW pointers must not be modified
      // until PerformPuts, EndStep or Close.
      // 1st batch
      engine.Put(varU, dataU);
      engine.Put(varV, dataV);
      
      // in this case adios2::Mode::Deferred is redundant,
      // as this is the default option
      engine.Put(varW, dataW, adios2::Mode::Deferred);

      // effectively dataU, dataV, dataW are "deferred"
      // until the first call to PerformPuts, EndStep or Close.
      // Application MUST NOT modify the data pointer (e.g. resize memory).
      engine.PerformPuts();

      // dataU, dataV, dataW pointers/values can now be reused
      
      // ... Application modifies dataU, dataV, dataW 

      //2nd batch
      engine.Put(varU, dataU);
      engine.Put(varV, dataV);
      engine.Put(varW, dataW);
      // Application MUST NOT modify dataU, dataV and dataW pointers (e.g. resize),
      // optionally data can be modified, but not recommended
      dataU[0] = 10
      dataV[0] = 10
      dataW[0] = 10 
      engine.PerformPuts();
      
      // dataU, dataV, dataW pointers/values can now be reused
      
      // Puts a varP block of zeros
      adios2::Variable<double>::Span spanP = Put<double>(varP);
      
      // Not recommended mixing static pointers, 
      // span follows 
      // the same pointer/iterator invalidation  
      // rules as std::vector
      T* p = spanP.data();

      // Puts a varMu block of 1e-6
      adios2::Variable<double>::Span spanMu = Put<double>(varMu, 0, 1e-6);
      
      // p might be invalidated 
      // by a new span, use spanP.data() again
      foo(spanP.data());

      // Puts a varRho block with a constant value of 1.225
      Put<double>(varMu, 0, 1.225);
      
      // it's preferable to start modifying spans 
      // after all of them are created
      foo(spanP.data());
      bar(spanMu.begin(), spanMu.end()); 
      
      
      engine.EndStep();
      // spanP, spanMu are consumed by the library
      // end of current logical step,
      // default behavior: transport data
   }

   engine.Close();
   // engine is unreachable and all data should be transported
   ...

.. tip::

   Prefer default Deferred (lazy evaluation) functions as they have the potential to group several variables with the trade-off of not being able to reuse the pointers memory space until ``EndStep``, ``PerformPuts``, ``PerformGets``, or ``Close``.
   Only use Sync if you really have to (*e.g.* reuse memory space from pointer).
   ADIOS2 prefers a step-based IO in which everything is known ahead of time when writing an entire step.


.. danger::
   The default behavior of adios2 ``Put`` and ``Get`` calls IS NOT synchronized, but rather deferred.
   It's actually the opposite of ``MPI_Put`` and more like ``MPI_rPut``.
   Do not assume the data pointer is usable after a ``Put`` and ``Get``, before ``EndStep``, ``Close`` or the corresponding ``PerformPuts``/``PerformGets``.
   Avoid using TEMPORARIES, r-values, and out-of-scope variables in ``Deferred`` mode, use adios2::Mode::Sync if required.


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


