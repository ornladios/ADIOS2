**************
CompressorSZ3
**************

The ``CompressorSZ3`` Operator is a compressor that uses lossy error-bounded
compression to achieve high compression ratios for scientific floating-point data.

SZ3 is a modular error-bounded lossy compression framework for scientific datasets.
It provides various compression modes and error control mechanisms, making it
suitable for a wide range of scientific applications that require both high
compression ratios and controlled data accuracy.

ADIOS2 provides a ``CompressorSZ3`` operator that lets you compress and
decompress variables. Below there is an example of how to invoke the
``CompressorSZ3`` operator:

.. code-block:: c++

    adios2::IO io = adios.DeclareIO("Output");
    auto sz3Op = adios.DefineOperator("SZ3Compressor", "sz3");

    auto var_r32 = io.DefineVariable<float>("r32", shape, start, count);
    var_r32.AddOperation(sz3Op, {{"accuracy", "0.001"}});

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CompressorSZ3 Specific parameters
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``CompressorSZ3`` operator accepts the following operator specific
parameters:

+--------------------+--------------------------------------------------------------+
| ``CompressorSZ3`` available parameters                                          |
+====================+==============================================================+
| ``accuracy``       | Alias for ``absolute``, fixed absolute error tolerance       |
+--------------------+--------------------------------------------------------------+
| ``absolute``       | Alias for ``abs``, fixed absolute error tolerance            |
+--------------------+--------------------------------------------------------------+
| ``abs``            | Fixed absolute error tolerance                               |
+--------------------+--------------------------------------------------------------+
| ``abserrbound``    | Fixed absolute error tolerance                               |
+--------------------+--------------------------------------------------------------+
| ``relative``       | Alias for ``rel``, relative error tolerance                  |
+--------------------+--------------------------------------------------------------+
| ``rel``            | Relative error tolerance                                     |
+--------------------+--------------------------------------------------------------+
| ``relerrbound``    | Relative error tolerance                                     |
+--------------------+--------------------------------------------------------------+
| ``psnr``           | Peak signal-to-noise ratio error bound                       |
+--------------------+--------------------------------------------------------------+
| ``psnrerrbound``   | Peak signal-to-noise ratio error bound                       |
+--------------------+--------------------------------------------------------------+
| ``norm``           | Alias for ``l2norm``, L2 norm error bound                    |
+--------------------+--------------------------------------------------------------+
| ``l2norm``         | L2 norm error bound                                          |
+--------------------+--------------------------------------------------------------+
| ``l2normerrbound`` | L2 norm error bound                                          |
+--------------------+--------------------------------------------------------------+
| ``mode``           | Error bound mode: ``ABS``, ``REL``, ``PSNR``, ``L2NORM``,   |
|                    | ``ABS_AND_REL``, ``ABS_OR_REL``                              |
+--------------------+--------------------------------------------------------------+
| ``errorboundmode`` | Error bound mode (same options as ``mode``)                  |
+--------------------+--------------------------------------------------------------+

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CompressorSZ3 Supported Data Types
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``CompressorSZ3`` supports the following data types:

- ``float`` - 32-bit floating point
- ``double`` - 64-bit floating point
- ``std::complex<float>`` - Complex 32-bit floating point
- ``std::complex<double>`` - Complex 64-bit floating point

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CompressorSZ3 Dimension Support
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``CompressorSZ3`` supports arrays with 1 to 4 dimensions. The SZ3 library
currently has a maximum dimension limit of 4.

~~~~~~~~~~~~~~~~~~~~~~~~~
CompressorSZ3 Error Modes
~~~~~~~~~~~~~~~~~~~~~~~~~

The ``CompressorSZ3`` operator supports multiple error control modes:

- **Absolute Error (ABS)**: Guarantees that the absolute difference between the
  original and decompressed data is within the specified bound.

- **Relative Error (REL)**: Guarantees that the relative difference between the
  original and decompressed data is within the specified bound.

- **PSNR Error**: Controls compression based on peak signal-to-noise ratio.

- **L2 Norm Error**: Controls compression based on the L2 norm of the error.

- **ABS_AND_REL**: Applies both absolute and relative error bounds simultaneously.

- **ABS_OR_REL**: Uses whichever error bound is less restrictive.

.. note::

   When using ``accuracy`` parameter without specifying a ``mode``, the
   default error bound mode is **ABS** (absolute error).
