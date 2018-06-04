************************
C++11 and C++98 bindings
************************

C++11 and C++98 public API bindings are closely related. They both use the same oriented-oriented and template structure, names, private implementation, `Pimpl <https://isocpp.org/blog/2018/01/the-pimpl-pattern-what-you-should-know-bartlomiej-filipek>`_ , thus they are object and member function based (no pointer, references, member variables, etc.). 

They differ in their usage due to the newly added C++11 features: `std::function` for callback functions, list initializers, and strongly typed enum classes. The C++11 uses the `adios2 namespace`, while the C++98 requires the use of the `adios2::cxx98` namespace.

.. caution::

   DO NOT use the clause ``using namespace adios2`` or ``using namespace adios2::cxx98`` in your code. It might create name conflicts.


.. tip::

   Prefer the C++11 bindings to take advantage of added functionality and to avoid application binary interface (ABI) incompatabilities between C++98 and the native C++11 library. Use C++98 if, and only if, it's your only choice.
   

ADIOS2 objects
--------------

ADIOS2 C++ bindings objects are mapped 1-to-1 to the ADIOS components described in the :ref:`Application Programmer Interface` section. Only the ``adios2::ADIOS`` object is "heavy", all other components are light-weigth objects refering to a component inside the "factory" ``adios2::ADIOS`` object.`
 
.. code-block:: c++
   
   c++11                 c++98
   adios2::ADIOS         adios2::cxx98::ADIOS  
   adios2::IO            adios2::cxx98::IO
   adios2::Variable<T>   adios2::cxx98::Variable<T>
   adios2::Attribute<T>  adios2::cxx98::Attribute<T>
   adios2::Engine        adios2::cxx98::Engine
   adios2::Operator      


The following section provides a summary of the available functionality for each class. Users can generate their own API reference documentation via doxygen, see ADIOS2/docs/ReadMe.md.

:ref:`ADIOS` class
------------------

:ref:`IO` class
---------------

:ref:`Variables` template class
-------------------------------

:ref:`Attributes` template class
--------------------------------

:ref:`Engine` class
-------------------

:ref:`Operator` class
---------------------

   
   
