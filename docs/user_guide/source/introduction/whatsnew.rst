======================
What's new in master?
======================

C API Improvements
------------------

New functions for safely retrieving string data of unknown length using a
two-call pattern (first call with NULL buffer to query length, then call
again with allocated buffer to retrieve data):

- ``adios2_get_string()`` - for string variables
- ``adios2_attribute_string_data()`` - for single-value string attributes
- ``adios2_attribute_string_data_array()`` - for string array attributes

This addresses a gap in the C API where string lengths could not be queried
before reading.

Selection API for Get() Operations
-----------------------------------

A new ``Selection`` class provides an explicit, self-contained way to specify
what data to read in ``Get()`` calls. Instead of mutating the ``Variable``
object with ``SetSelection()``, ``SetBlockSelection()``, ``SetStepSelection()``,
etc., all selection parameters are bundled into a ``Selection`` object that is
passed directly to ``Get()``. This avoids relying on potentially stale variable
state and makes the intent of each ``Get()`` call clear at the call site.

A selection has two independent aspects: a spatial selection —
``Selection::All()`` (the default — reads the entire variable) or
``Selection::BoundingBox(start, count)`` (hyperslab) — and an optional block
selection via ``Selection::Block(blockID)`` or ``.WithBlock(blockID)``. These
are orthogonal: a block ID can be combined with a bounding box to read a
sub-region of a specific write block. Additional parameters (steps, memory
layout, accuracy) can be added via fluent ``With*()`` methods or mutable
``Set*()`` methods. A ``ToString()`` method is provided for debugging.

.. code-block:: c++

   // Select everything
   engine.Get(var, data, adios2::Selection::All());

   // Fluent style — bounding box with step range
   auto sel = adios2::Selection::BoundingBox({0, 0}, {10, 20}).WithSteps(0, 5);
   engine.Get(var, data, sel);

   // Mutable style — reuse across iterations
   adios2::Selection sel;
   sel.SetBoundingBox({0, 0}, {10, 20});
   for (size_t step = 0; step < nsteps; ++step)
   {
       sel.SetSteps(step, 1);
       engine.Get(var, buffers[step], sel);
   }

   // Debug output
   std::cout << sel.ToString() << std::endl;

Existing ``SetSelection()``-based APIs remain available and are not deprecated.
See :ref:`Selection` for full documentation.

Plugin Interface v2 (Breaking Change)
--------------------------------------

Both the engine and operator plugin interfaces have been redesigned as
standalone classes that no longer inherit from internal core types
(``core::Engine`` and ``core::Operator``). This is a **breaking change** for
existing plugins written against the v1 interface (ADIOS2 2.11 and earlier).

**What changed:**

- **Engine plugins** now inherit from ``PluginEngineInterface`` (in
  ``adios2/plugin/PluginEngineInterface.h``) which uses public C++ API types
  (``adios2::IO``, ``adios2::Variable<T>``) instead of internal core types.
  The ``EngineCreate()`` signature now takes ``adios2::IO`` and ``adios2::Mode``
  instead of ``core::IO &`` and ``helper::Comm``.
- **Operator plugins** now inherit from ``PluginOperatorInterface`` (in
  ``adios2/plugin/PluginOperatorInterface.h``) which is fully self-contained
  with no dependencies on ADIOS internals.
- Plugin shared libraries now only need to link against ``adios2::cxx``
  (not ``adios2::core``).

**Migrating from v1:** Update your plugin class to inherit from the new
interface, replace any ``core::IO`` / ``core::Variable<T>`` usage with the
public ``adios2::IO`` / ``adios2::Variable<T>`` types, and update the
``EngineCreate()`` / ``EngineDestroy()`` signatures accordingly. See the
examples in ``examples/plugins/`` for reference implementations.

**New engine plugin capabilities:** The v2 engine interface has also been
extended with optional methods for more advanced functionality:

- **Selection-based Get**: ``DoGetSync()`` and ``DoGetDeferred()`` overloads
  that accept a ``Selection`` parameter, enabling sub-array and block reads.
- **Span-based Put**: ``DoPut()`` with a ``Span`` parameter for zero-copy
  writes of primitive types.
- **Step queries**: ``Steps()`` to report the total number of available steps.
- **Block introspection**: ``MinBlocksInfo()`` to return block decomposition
  metadata for a given variable and step.

All new methods have empty default implementations, so they are optional.
See :ref:`Plugins` for full documentation.

Cross-Endian Interoperability
-----------------------------

The ``ADIOS2_USE_Endian_Reverse`` CMake option has been removed. Cross-endian
file interoperability (reading files written on a machine with different byte
order) is now always enabled. This means files written on big-endian systems
can be read on little-endian systems and vice versa without any special build
configuration.


===================
What's new in 2.11?
===================

This is a major release with new features and lots of bug fixes. The main
highlights include enhanced derived variables, remote data access, GPU
improvements, campaign management overhaul, and advanced compression operators.

Summary
=======

Derived Variables
-----------------

Support for reader-side computed variables with expression evaluation. This
release adds comprehensive functionality including new mathematical operations
(trigonometric functions, multiplication, division, power) and the
ability to apply derived variables to aggregated arrays.

Remote Data Access Infrastructure
----------------------------------

Implementation of XRootD and remote server capabilities enabling distributed
file access. This includes support for remote streaming, metadata caching, and
optimized query handling across networked systems.

GPU and Memory Space Improvements
----------------------------------

Enhanced GPU backend support with memory selection adjustments, Kokkos
integration improvements, and bindings for cuPy pointers and torch tensors.
Addresses layout mismatches between user code and device memory.

Campaign Management System Overhaul
------------------------------------

Significant updates to campaign tracking using SQLite databases replacing JSON
files, configuration file support, and attribute handling. Includes multi-host
support and improved metadata organization.

Compression and Data Format Advances
-------------------------------------

New operators including BigWhoop compression and Complex MGARD support. ZFP now
supports 4D arrays, and improved MGARD efficiency through zstd compression of
lossless data portions.

OpenSSF Best Practices
-----------------------

ADIOS2 now adopts OpenSSF (Open Source Security Foundation) best practices
including enhanced security measures, automated vulnerability scanning, and
adherence to industry-standard development practices.


===================
What's new in 2.10?
===================

This is a major release with new features and lots of bug fixes. The main new feature is the new Python API. 

Python
------
Before, ADIOS had two separate APIs for Python. The low-level ("Full") API was written with Pybind11 and directly mimicked the C++ API. The high-level API was another, smaller, and more pythonesque API that allowed for easier scripting with Python. The main problems with these two were that they were independent, and that the high-level API was not complete. Once a developer needed a feature only available in the full API, they had to start from scratch writing a script with the full API. 

In 2.10, there is officially one Python API, written in Python, which in turn uses the old Pybind11 classes. The new API combines the high-level features of the old high-level API -- hopefully in a more consistent and likeable way, -- and the full feature set of the low-level bindings. 

.. note::

   Old scripts that used the full API can still run without almost any modification, just change the import line from ``import adios2`` to ``import adios2.bindings as adios2``

   Old scripts that used the high-level API must be modified to make them work with the new API, see :ref:`Transition from old API to new API`


See :ref:`Python API`


New/updated features
--------------------

 - BP5 is supported on Windows now 
 - SST and DataMan staging engines are GPU-Aware now
 - SYCL support added for Intel GPUs (besides CUDA and HIP for NVidia and AMD GPUs)
 - the SST/libfabric data transport now works on Frontier (besides the MPI data transport)


Packaging
----------

  - adios2 package is now on `PyPi <https://pypi.org/project/adios2/>`_


==================
What's new in 2.9?
==================

Summary
=======

This is a major release with new features and lots of bug fixes.

General
-------

- GPU-Aware I/O enabled by using Kokkos. Device pointers can be passed to Put()/Get() calls directly. Kokkos 3.7.x required for this release. Works with CUDA, HIP and Kokkos applications.  https://adios2.readthedocs.io/en/latest/advanced/gpu_aware.html#gpu-aware-i-o
- GPU-compression. MGARD and ZFP operators can compress data on GPU if they are built for GPU. MGARD operator can be fed with host/device pointers and will move data automaticaly. ZFP operator requires matching data and compressor location.
- Joined Array concept (besides Global Array and Local Array), which lets writers dump Local Arrays (no offsets no global shape) that are put together into a Global Array by the reader. One dimension of the arrays is selected for this join operation, while other dimensions must be the same for all writers. https://adios2.readthedocs.io/en/latest/components/components.html?highlight=Joined#shapes 

File I/O
--------

- Default File engine is now BP5. If for some reason this causes problems, manually specify using "BP4" for your application.
- BP5 engine supports multithreaded reading to accelerate read performance for low-core counts.
- BP5 Two level metadata aggregation and reduction reduced memory impact of collecting metadata and therefore is more scalable in terms of numbers of variables and writers than BP4.
- Uses Blosc-2 instead of Blosc for lossless compression. The new compression operator is backward compatible with old files compressed with blosc. The name of the operator remains "blosc".

Staging
-------

- UCX dataplane added for SST staging engine to support networks under the UCX consortium
- MPI dataplane added for  SST staging engine. It relies on MPI intercommunicators to connect multiple independent MPI applications for staging purposes. Applications must enable multithreaded MPI for this dataplane.

Experimental features
---------------------

- Preliminary support for data structs. A struct can have single variables of basic types, and 1D fixed size arrays of basic types. Supported by BP5, SST and SSC engines. 
