***************
CompressorMGARD
***************

The ``CompressorMGARD`` Operator is a compressor that uses lossy
error-bounded compression to achieve high compression ratios for scientific
floating-point data.

MGARD (MultiGrid Adaptive Reduction of Data) is a multilevel compression
framework based on a hierarchical (multigrid) decomposition of the input.
It supports a rigorous error bound in either an absolute or relative sense
and lets the user tune how that error is measured through a smoothness
parameter.

ADIOS2 provides a ``CompressorMGARD`` operator that lets you compress and
decompress variables. Below there is an example of how to invoke the
``CompressorMGARD`` operator:

.. code-block:: c++

    adios2::IO io = adios.DeclareIO("Output");
    auto mgardOp = adios.DefineOperator("MGARDCompressor", adios2::ops::LossyMGARD);

    auto var_r32 = io.DefineVariable<float>("r32", shape, start, count);
    var_r32.AddOperation(mgardOp, {{adios2::ops::mgard::key::tolerance, "0.001"},
                                   {adios2::ops::mgard::key::s, "inf"}});

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CompressorMGARD Specific parameters
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``CompressorMGARD`` operator accepts the following operator specific
parameters:

+------------------+-----------------------------------------------------------------+
| ``CompressorMGARD`` available parameters                                           |
+==================+=================================================================+
| ``tolerance``    | Error tolerance (mandatory). Interpreted according to ``mode``. |
+------------------+-----------------------------------------------------------------+
| ``accuracy``     | Alias for ``tolerance``.                                        |
+------------------+-----------------------------------------------------------------+
| ``mode``         | Error bound mode: ``ABS`` or ``REL`` (default ``REL``).         |
+------------------+-----------------------------------------------------------------+
| ``s``            | Smoothness / norm parameter (default ``0``). Use ``inf`` for    |
|                  | the L-infinity (max) norm, ``0`` for the L2 norm, or any finite |
|                  | real for the corresponding Sobolev norm.                        |
+------------------+-----------------------------------------------------------------+
| ``threshold``    | Minimum input size in bytes below which compression is skipped  |
|                  | and the block is stored uncompressed (default ``100000``).      |
+------------------+-----------------------------------------------------------------+
| ``lossless_type``| Lossless stage applied after MGARD's quantization. One of:      |
|                  | ``huffman``, ``huffman_lz4``, ``huffman_zstd``, ``cpu_lossless``|
|                  | (default ``huffman_zstd``).                                     |
+------------------+-----------------------------------------------------------------+

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CompressorMGARD Supported Data Types
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``CompressorMGARD`` supports the following data types:

- ``float`` - 32-bit floating point
- ``double`` - 64-bit floating point
- ``std::complex<float>`` - complex 32-bit floating point
- ``std::complex<double>`` - complex 64-bit floating point

For complex inputs, the operator presents the data to MGARD as a
real-valued array of the underlying scalar type (``float`` or ``double``)
with the trailing dimension doubled. MGARD therefore operates on the
real and imaginary parts interleaved along that axis, which may affect
compression quality compared to a purely real-valued input of the same
shape.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CompressorMGARD Dimension Support
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``CompressorMGARD`` supports arrays with 1 to 5 dimensions. ADIOS2 pads
1D and 2D blocks up to 3 dimensions before handing them to MGARD-x.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CompressorMGARD Error Bound Semantics
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The error bound is specified by two orthogonal parameters: ``mode``
selects how ``tolerance`` is interpreted, and ``s`` selects the norm in
which the error is measured.

**``mode``** controls the scale of the bound:

- **``ABS``**: ``tolerance`` is an absolute value. The error in the
  selected norm is bounded by ``tolerance``.

- **``REL``** *(default)*: ``tolerance`` is relative to the magnitude of
  the input block. The guaranteed error is ``tolerance * ||input||``.

**``s``** selects the norm used to measure the error:

- ``s=inf`` (L-infinity) bounds the worst-case pointwise error.
- ``s=0`` (L2, default) bounds the aggregate error in the least-squares
  sense.
- Finite real values select the corresponding Sobolev norms, which
  balance pointwise accuracy against smoothness.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CompressorMGARD GPU Interaction
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ADIOS2 MGARD operator passes the input buffer directly to the
underlying MGARD-x library without staging it through host memory. Whether
a GPU device pointer can be used as input therefore depends on the
MGARD-x build that ADIOS2 is linked against:

- If MGARD-x is built with a GPU backend (CUDA, HIP, or SYCL), it
  consumes a matching device pointer in place.

- If MGARD-x is built SERIAL-only, the input must reside in host memory.

The output buffer that ADIOS2 hands to the operator is host-resident, so
the decompressed result is always returned to host memory regardless of
the MGARD-x backend.
