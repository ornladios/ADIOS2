***************
Python bindings
***************

.. note::

   Product Application Developers targeting finer-level control for their IO tasks for optimization should use the current full APIs. If you want to use ADIOS2 in simple use cases (*e.g.* reading a file for analysis, interactive Python, or saving some data for a small project) please refer to the :ref:`High-Level APIs` for a flat learning curve.

The full Python APIs follow very closely the full C++11 API interface.

ADIOS class
--------------
.. autoclass:: adios2.bindings.adios2_bindings::ADIOS
    :members:

IO class
--------------
.. autoclass:: adios2.bindings.adios2_bindings::IO
    :members:

Variable class
--------------
.. autoclass:: adios2.bindings.adios2_bindings::Variable
    :members:

Attribute class
---------------
.. autoclass:: adios2.bindings.adios2_bindings::Attribute
    :members:

Engine class
--------------
.. autoclass:: adios2.bindings.adios2_bindings::Engine
    :members:

Operator class
--------------
.. autoclass:: adios2.bindings.adios2_bindings::Operator
    :members:
