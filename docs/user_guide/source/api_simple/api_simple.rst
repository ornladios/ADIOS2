#############################
Simple Language Bindings APIs
#############################

The current simple APIs are designed for simple and direct tasks. Unlike the :ref:`Full Language Bindings APIs` the only require a single object/handler that is closely related to the native language IO bindings for formatted text IO **e.g.** C++ fstream, C FILE*, Fortran and Python file IO. Thus providing a nearly-flat learning curve. 

The main function calls are: ``open`` (or constructor in C++), ``write``, ``read`` and ``close`` (or destructor in C++), with the corresponding language native syntax for advancing the step in write mode (**e.g.** C++ ``adios2::endl``, Fortran ``adios2_advance_yes``, Python ``endl=True``), and for a "step-by-step" streaming basis in read mode (**e.g.** C++ ``adios2::getstep``, Fortran ``adios2_advance_yes``, Python ``for fstep in fh``).

Typical scenarios for using the simple APIs are:

* Reading a file to perform data analysis with libraries (matplotlib, scipy, etc.)
* Interactive: few calls make interactive usage easier. 
* Saving data to files is small or personal projects
* Notebooks: **e.g.** Jupyter


.. tip::
   
   Use the simple APIs for very simple tasks in which performance is not a critical aspect:

.. warning::

   The simplified APIs are not meant to be used inside applications in which the end user has no access to the IO layer and/or when performance and full functionality are critical. Use the :ref:`Full Language Bindings APIs` instead

.. note::

   The simpified APIs are based on language native file IO interface. Hence write and read calls are always synchronized and variables are ready to use


Currently ADIOS2 support bindings for the following languages and their minimum standards:

+----------+----------+---------------------+---------------+
| Language | Standard | Interface           | Based on      |
+----------+----------+---------------------+---------------+
| C++      | 11/newer | #include adios2.h   | fstream       |
+----------+----------+---------------------+---------------+
| C        | 99       | #include adios2_c.h | stdio FILE*   |
+----------+----------+---------------------+---------------+
| Fortran  | 90       | use adios2          | Fortran/stdio |
+----------+----------+---------------------+---------------+
| Python   | 2.7/3    | import adios2       | Python IO     |
+----------+----------+---------------------+---------------+
| Matlab   |          |                     |               |
+----------+----------+---------------------+---------------+

The following sections provide a summary of the API calls on each language and links to Write and Read examples to put it all together.

.. include:: cxx11.rst
.. include:: c.rst
.. include:: fortran.rst
.. include:: python.rst
.. include:: matlab.rst

