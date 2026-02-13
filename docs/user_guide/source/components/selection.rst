*********
Selection
*********

The ``Selection`` class provides an explicit, self-contained specification for
``Get()`` operations. It bundles all the parameters that describe what data to
read — bounding box, block ID, step range, memory layout, and accuracy — into a
single object that is passed to ``Engine::Get()``.

Motivation
----------

The traditional ADIOS2 read pattern requires mutating a ``Variable`` object
before calling ``Get()``:

.. code-block:: c++

   var.SetSelection({{0, 0}, {10, 20}});
   var.SetStepSelection({0, 5});
   engine.Get(var, data);

This works, but the selection state lives inside the ``Variable`` and can become
stale or unclear when the same variable is read with different selections in
different parts of the code. The ``Selection`` class makes each ``Get()`` call
self-documenting by specifying the selection explicitly at the call site:

.. code-block:: c++

   auto sel = adios2::Selection::BoundingBox({0, 0}, {10, 20}).WithSteps(0, 5);
   engine.Get(var, data, sel);

Creating Selections
-------------------

Selections are created using static factory methods. There are three selection
types:

.. code-block:: c++

   // All — select the entire variable (this is also the default)
   auto sel = adios2::Selection::All();

   // Bounding box (hyperslab) — specify start offsets and counts per dimension
   auto sel = adios2::Selection::BoundingBox({0, 0}, {10, 20});

   // Block selection — select an individual write block by ID
   auto sel = adios2::Selection::Block(3);

   // Using Box<Dims> convenience type
   adios2::Box<adios2::Dims> box = {{0, 0}, {10, 20}};
   auto sel = adios2::Selection::BoundingBox(box);

A default-constructed ``Selection`` is equivalent to ``Selection::All()``:

.. code-block:: c++

   adios2::Selection sel;
   engine.Get(var, data, sel);  // reads the whole variable

Fluent Style (Immutable)
------------------------

The ``With*()`` methods return a **new** ``Selection`` with the modification
applied, leaving the original unchanged. This is useful for one-shot
expressions:

.. code-block:: c++

   auto sel = adios2::Selection::BoundingBox({0, 0}, {10, 20})
                  .WithSteps(0, 5)
                  .WithMemory({0, 0}, {10, 40})
                  .WithAccuracy({0.01, 0.0, false});
   engine.Get(var, data, sel);

Mutable Style (Reuse)
---------------------

The ``Set*()`` methods modify the ``Selection`` in place and return ``*this``
for chaining. This is useful when reusing a selection across iterations:

.. code-block:: c++

   adios2::Selection sel;
   sel.SetBoundingBox({0, 0}, {10, 20});
   sel.SetAccuracy({0.01, 0.0, false});

   for (size_t step = 0; step < nsteps; ++step)
   {
       sel.SetSteps(step, 1);
       engine.Get(var, buffers[step], sel);
   }

Using Selections with Get()
---------------------------

New ``Engine::Get()`` overloads accept a ``Selection`` parameter:

.. code-block:: c++

   // Raw pointer version
   engine.Get(var, data_ptr, sel, adios2::Mode::Sync);

   // std::vector version (auto-resized)
   std::vector<double> data;
   engine.Get(var, data, sel);

Buffer Pre-allocation with SelectionSize()
------------------------------------------

``Variable::SelectionSize()`` now accepts a ``Selection`` to compute the
required buffer size without modifying the variable:

.. code-block:: c++

   auto sel = adios2::Selection::BoundingBox({0, 0}, {10, 20});
   size_t nelems = var.SelectionSize(sel);
   double *buf = new double[nelems];
   engine.Get(var, buf, sel);

Available Selection Parameters
------------------------------

.. list-table::
   :header-rows: 1

   * - Parameter
     - Factory / Setter
     - Non-mutating modifier
     - Description
   * - All
     - ``All()``
     - —
     - Select the entire variable (default)
   * - Bounding Box
     - ``BoundingBox(start, count)`` / ``SetBoundingBox()``
     - —
     - Hyperslab selection with start offsets and counts
   * - Block
     - ``Block(blockID)`` / ``SetBlock()``
     - —
     - Select an individual write block by ID
   * - Steps
     - ``SetSteps(start, count)``
     - ``WithSteps()``
     - Step range to read (default: current step, count 1)
   * - Memory
     - ``SetMemory(start, count)``
     - ``WithMemory()``
     - Memory layout specification for the destination buffer
   * - Accuracy
     - ``SetAccuracy(error, norm, relative)``
     - ``WithAccuracy()``
     - Lossy accuracy requirements

Other methods:

- ``Clear()`` — reset the selection to default state (All)
- ``ClearMemory()`` — remove memory layout specification
- ``ToString()`` — human-readable string representation for debugging

.. code-block:: c++

   auto sel = adios2::Selection::BoundingBox({0, 0}, {10, 20}).WithSteps(0, 5);
   std::cout << sel.ToString() << std::endl;
   // Output: Selection(BoundingBox start={0, 0} count={10, 20}, steps=[0, 5])

Engine Support
--------------

Selection-based ``Get()`` is currently supported by the **BP5** and **SST**
(with BP5 marshalling) engines. Other engines will throw a runtime error if a
``Selection`` is passed to ``Get()``.

Compatibility
-------------

The existing ``Variable::SetSelection()``, ``SetBlockSelection()``,
``SetStepSelection()``, and ``SetMemorySelection()`` APIs remain available and
are not deprecated. The ``Selection`` class is a convenience alternative that
makes the selection explicit at the ``Get()`` call site rather than relying on
state previously set on the ``Variable``.
