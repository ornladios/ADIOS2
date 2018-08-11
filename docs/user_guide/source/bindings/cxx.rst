************************
C++11 and C++98 bindings
************************

.. role:: cpp(code)
   :language: c++
   :class: highlight
   
C++11 and C++98 public API bindings are closely related. They both use the same oriented-oriented and template structure, names, private implementation, `Pimpl <https://isocpp.org/blog/2018/01/the-pimpl-pattern-what-you-should-know-bartlomiej-filipek>`_ , thus they are object and member function based (no pointer, references, member variables, etc.). 

They differ in their usage due to the newly added C++11 features: `std::function` for callback functions, list initializers, and strongly typed enum classes. The C++11 uses the `adios2 namespace`, while the C++98 requires the use of the `adios2::cxx98` namespace.

.. caution::

   DO NOT use the clause ``using namespace adios2`` or ``using namespace adios2::cxx98`` in your code. It might create name conflicts.
   Always use adios2:: explicitly, *e.g.* adios2::ADIOS, adios2::IO.


.. tip::

   Prefer the C++11 bindings to take advantage of added functionality and to avoid application binary interface (ABI) incompatabilities between C++98 and the native C++11 library. Use C++98 if, and only if, it's your only choice.
   

ADIOS2 objects
--------------

ADIOS2 C++ bindings objects are mapped 1-to-1 to the ADIOS components described in the :ref:`Application Programmer Interface` section. Only the ``adios2::ADIOS`` object is "heavy", all other components are light-weigth objects refering to a component inside the "factory" ``adios2::ADIOS`` object.`
 
.. code-block:: c++
   
   c++11                 
   adios2::ADIOS           
   adios2::IO            
   adios2::Variable<T>   
   adios2::Attribute<T>  
   adios2::Engine
   adios2::Operator
   
   c++98
   adios2::cxx98::ADIOS
   adios2::cxx98::IO
   adios2::cxx98::Variable<T>
   adios2::cxx98::Attribute<T>
   adios2::cxx98::Engine


The following section provides a summary of the available functionality for each class. Users can generate their own API reference documentation via doxygen, see ADIOS2/docs/ReadMe.md.

:ref:`ADIOS` class
------------------

* :cpp:`adios2::ADIOS` (Constructors) starting point for the adios2 library component 

   .. code-block:: c++
   
      //MPI constructors
      adios2::ADIOS (const std::string &configFile, MPI_Comm mpiComm, 
                     const bool debugMode=true);
      adios2::ADIOS (MPI_Comm mpiComm, const bool debugMode=true);
      
      //non-MPI constructors
      adios2::ADIOS (const std::string &configFile, const bool debugMode=true);
      adios2::ADIOS (const bool debugMode=true);
      
      /* where:
      configFile runtime config file
      mpiComm    defines domain scope from application
      debugMode  true: extra user-input debugging information, 
                 false: run without checking user-input (stable workflows)
      */           

* :cpp:`operator bool`  allows for checking if the ADIOS object is valid

   .. code-block:: c++
      
      // in C++11 
      explicit operator bool() const noexcept;
      
      // in C++98
      operator bool() const;

         
* :cpp:`DeclareIO` spawn IO objects. This function is required even if the IO is passed in configFile of the ADIOS object

   .. code-block:: c++
   
      adios2::IO ADIOS::DeclareIO(const std::string ioName);
      
      //where:
      ioName unique IO name identifier
      return adios2::IO object
      throws std::invalid_argument if IO previously declared with DeclareIO
      
* :cpp:`AtIO` retrieves an IO object previously created with DeclareIO 

   .. code-block:: c++
   
      adios2::IO ADIOS::AtIO(const std::string ioName);
      
      //where:
      ioName unique IO name identifier
      return adios2::IO object
      throws std::invalid_argument if IO not previosly created with DeclareIO
     
* :cpp:`DefineOperator` defines a supported ADIOS2 library operator

   .. code-block:: c++
   
      // signature used for compression operation 
      adios2::Operator DefineOperator(const std::string name, 
                                      const std::string type, 
                                      const Params &parameters = Params());
      
      /* where:
      name        unique operator identifier with the ADIOS object scope
      type        supported operator in the ADIOS2 library
      parameters  set of specific key/value parameters to apply to this
                  particular returned adios2::Operator
      return      adios2::Operator
      throws      std::invalid_argument 
      */
      
      // C++11 only...uses variadic templates
      // this is the signature used for Callback functions
      template <class R, class... Args>
      Operator DefineOperator(const std::string name,
                              const std::function<R(Args...)> &function,
                              const Params &parameters = Params());
      
      //where:
      name        unique operator identifier within the ADIOS object scope
      function    callable C++11 target: *e.g.* function, lambda function, member function 
      parameters  set of specific key/value parameters to apply to the returned adios2::Operator
      return      adios2::Operator
      throws      std::invalid_argument


* :cpp:`InquireOperator` retrieves a supported ADIOS2 library operator previously defined with DefineOperator

   .. code-block:: c++
   
      // signature used for compression operation 
      adios2::Operator InquireOperator(const std::string name) noexcept;
      
      //where:
      name        unique operator identifier with the ADIOS object scope
      return      adios2::Operator, if operator is not found this object bool operator is false


:ref:`IO` class
---------------

* :cpp:`InConfigFile` checks if an IO object, identified by name, is in the config file passed to ADIOS 

   .. code-block:: c++
   
      bool InConfigFile() const noexcept;
      

* :cpp:`SetEngine` sets a supported engine type, see :ref:`Supported Engines`, if this function is not called the default engine is BPFile. This can be set at runtime in the config file passed to the ADIOS owner object.

   .. code-block:: c++
   
      void SetEngine(const std::string engineType) noexcept;
      
      
* :cpp:`SetParameter` sets a parameter to fine tune up engine behavior, see :ref:`Supported Engines` for parameters supported by each Engine and defaults. If a parameter key already exists, the value will be replaced.

   .. code-block:: c++
   
      void SetParameter(const std::string key, const std::string value) noexcept;


* :cpp:`SetParameters` similar to SetParameter, but allows passing an adios2::Params (alias to :cpp:`std::map<std::string, std::string>`). This will replace any existing parameter.

   .. code-block:: c++ 
   
      void SetParameters(const adios2::Params &parameters = Params()) noexcept;
      
      // where
      parameters    full input set of parameters

* :cpp:`DefineVariable<T>` defines an adios2 Variable inside the current IO object

   .. code-block:: c++
   
      template <class T>
      Variable<T> DefineVariable(const std::string &name, const Dims &shape = Dims(), const Dims &start = Dims(), const Dims &count = Dims(), const bool constantDims = false);
      
      //where:
      name          unique variable identifier
      shape         physical dimension of the variable
      start         offsets to the variable local dimensions in the current rank
      count         variable local dimensions
      constantDims  true: shape, start, count won't change and are fixed, 
                    false: shape, start, count are expected to change
                    
* :cpp:`InquireVariable<T>` attempts to retrieve an exisiting Variable previosuly defined with DefineVariable<T> or if it's populated on the Read side by an Engine, see BeginStep


   .. code-block:: c++
   
      template <class T>
      adios2::Variable<T> InquireVariable<T>(const std::string& name);
      
      //where:
      name   unique variable identifier within the IO object
      return adios2::Variable<T> object, if the variable is not found the bool
             operator of the returned variable becomes false 


* :cpp:`DefineAttribute<T>` defines an adios2 Attribute inside the current IO object. This attribute is automatically accessible to all engines created from this IO (with Open). We can define single value or 1D array attributes.


   .. code-block:: c++
   
      // single value version
      template <class T>
      adios2::Attribute<T> DefineAttribute(const std::string &name, const T &value);
      
      // 1D array version
      template <class T>
      adios2::Attribute<T> DefineAttribute(const std::string &name, const T *data, const size_t size);
                                           
      //where:
      name   unique Attribute identifier within the IO object
      value  single value associated with the Attribute
      data   1D array associated with the Attribute
      size   data size
      return adios2::Attribute<T> attribute object  


* :cpp:`InquireAttribute<T>` attempts to retrieve an exisiting Variable previosuly defined with DefineAttribute<T> or if it's populated on the Read side by an Engine, see Engine's BeginStep.

   .. code-block:: c++
   
      template <class T>
      adios2::Variable<T> InquireAttribute<T>(const std::string& name);
      
      //where:
      name   unique variable identifier within the IO object
      return adios2::Attribute<T> object, if the variable is not found the bool operator of the returned variable becomes false


* :cpp:`AvailableVariables` inspect current available variables. Most common usage for reading mode.

   .. code-block:: c++
   
      std::map<std::string,adios2::Params> AvailableVariables() const noexcept;
      
      //where
      return std::map with key: variable name and value: adios2::Params (std::map<std::string,std::string>) 


* :cpp:`AvailableAttributes`



* :cpp:`Open` creates an Engine that starts the heavy-weight input output tasks

   .. code-block:: c++
   
      // Pass a new MPI communication version
      adios2::Engine Open(const std::string &name, const Mode mode, MPI_Comm comm);

      // Non-MPI version or reuses the communicator passed to the ADIOS object
      adios2::Engine Open(const std::string &name, const Mode mode);
   
      name    unique engine identifier, use depends on each Engine. 
              By default it's the name of a file name
      mode    supported open mode: adios2::Mode::Write, adios2::Mode::Read,
              adios2::Mode::Append (currently unsupported)
      comm    optional MPI communicator if the ADIOS object communicator is not        
              used for this Engine


:ref:`Variables` template class
-------------------------------

:ref:`Attributes` template class
--------------------------------



:ref:`Engine` class
-------------------



:ref:`Operator` class
---------------------
