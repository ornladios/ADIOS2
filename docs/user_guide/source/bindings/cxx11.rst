************************
C++11 and C++98 bindings
************************

.. role:: cpp(code)
   :language: c++
   :class: highlight
   
.. caution::

   DO NOT use the clause ``using namespace adios2`` or ``using namespace adios2::cxx98`` in your code. It might create name conflicts.
   Always use adios2:: explicitly, *e.g.* adios2::ADIOS, adios2::IO.


.. tip::

   Prefer the C++11 bindings to take advantage of added functionality and to avoid application binary interface (ABI) incompatabilities between C++98 and the native C++11 library. Use C++98 if, and only if, it's your only choice.
   

ADIOS2 components classes
-------------------------

ADIOS2 C++ bindings objects are mapped 1-to-1 to the ADIOS components described in the :ref:`Components Overview` section. Only the ``adios2::ADIOS`` object is "owned" by the developer's program using adios2, all other components are light-weigth objects refering to a component that lives inside the ``adios2::ADIOS`` "factory" object.`
 
.. code-block:: c++
   
   c++11                 
   adios2::ADIOS           
   adios2::IO            
   adios2::Variable<T>   
   adios2::Attribute<T>  
   adios2::Engine
   adios2::Operator

The following section provides a summary of the available functionality for each class.

:ref:`ADIOS` class
------------------

.. doxygenclass:: adios2::ADIOS
   :project: CXX11
   :path: ../../bindings/CXX11/cxx11/
   :members:
   
:ref:`Variable` <T> class
-------------------------

.. doxygenclass:: adios2::Variable
   :project: CXX11
   :path: ../../bindings/CXX11/cxx11/
   :members:


:ref:`Attribute` <T> class
--------------------------

.. doxygenclass:: adios2::Attribute
   :project: CXX11
   :path: ../../bindings/CXX11/cxx11/
   :members:

:ref:`Engine` class
-------------------

.. doxygenclass:: adios2::Engine
   :project: CXX11
   :path: ../../bindings/CXX11/cxx11/
   :members:

:ref:`Operator` class
---------------------

.. doxygenclass:: adios2::Operator
   :project: CXX11
   :path: ../../bindings/CXX11/cxx11/
   :members:
