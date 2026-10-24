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

Selections are created using static factory methods. A selection has two
independent aspects:

- **Spatial selection** — ``All`` (the entire variable) or ``BoundingBox``
  (a hyperslab with start offsets and counts).
- **Block selection** — optionally targets an individual write block by ID.

A block ID combines with a bounding box only on a **local array**, where the
box is taken relative to the selected block. On a global array the two select
alternative regions and the pair is refused; use a global bounding box, which
can already address any part of any block.

.. code-block:: c++

   // All — select the entire variable (this is also the default)
   auto sel = adios2::Selection::All();

   // Bounding box (hyperslab) — specify start offsets and counts per dimension
   auto sel = adios2::Selection::BoundingBox({0, 0}, {10, 20});

   // Block selection — read an entire write block
   // (equivalent to All().WithBlock(3))
   auto sel = adios2::Selection::Block(3);

   // Block + bounding box — a sub-region within a block, local arrays only
   // (on a global array this throws; select the region globally instead)
   auto sel = adios2::Selection::BoundingBox({0}, {10}).WithBlock(3);

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
                  .WithBlock(2)
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
     - ``WithBlock()``
     - Select an individual write block by ID (combines with a bounding box on
       a local array only, where the box is block-relative)
   * - Steps
     - ``SetSteps(start, count)``
     - ``WithSteps()``
     - Step range to read (default: current step, count 1). A multi-step read
       returns one rectangle, so the geometry must be the same at every step of
       the range; see below
   * - Memory
     - ``SetMemory(start, count)``
     - ``WithMemory()``
     - Memory layout specification for the destination buffer
   * - Accuracy
     - ``SetAccuracy(error, norm, relative)``
     - ``WithAccuracy()``
     - Lossy accuracy requirements

Other methods:

- ``Clear()`` — reset the selection to default state (All, no block)
- ``ClearBlock()`` — remove block selection
- ``ClearMemory()`` — remove memory layout specification
- ``ToString()`` — human-readable string representation for debugging

.. code-block:: c++

   auto sel = adios2::Selection::BoundingBox({0, 0}, {10, 20}).WithSteps(0, 5);
   std::cout << sel.ToString() << std::endl;
   // Output: Selection(BoundingBox start={0, 0} count={10, 20}, steps=[0, 5])

Reading more than one step at a time
------------------------------------

A multi-step read returns ``stepCount`` copies of one geometry, laid out back to
back and strided by the per-step size. That only has an answer when the geometry
is the same at every step of the range, so a range over which the global
dimensions change, or over which the selected block changes size, is refused on
every engine.
Read those a step at a time, asking for each step's shape as you go. A step
selection belongs to ``Mode::ReadRandomAccess``; in ``Mode::Read`` the engine
hands you one step at a time already, so you read inside the
``BeginStep``/``EndStep`` pair and do not set one:

.. code-block:: c++

   // Mode::ReadRandomAccess
   for (size_t step = 0; step < var.Steps(); ++step)
   {
       var.SetStepSelection({step, 1});
       data.resize(var.SelectionSize());   // this step's extent
       engine.Get(var, data.data(), adios2::Mode::Sync);
   }

``var.Shape(step)`` gives that step's dimensions if you need them rather than
just the element count.

Engine Support
--------------

Selection-based ``Get()`` is currently supported by the **BP5** and **SST**
(with BP5 marshalling) engines. Other engines will throw a runtime error if a
``Selection`` is passed to ``Get()``.

Thread-Safe Concurrent Reads with GetContext
--------------------------------------------

The Selection-based ``Get()`` shown above shares per-engine queue state with
every other ``Get()`` call on the same engine, so concurrent calls from
multiple threads are not safe. A second overload takes an explicit
``GetContext`` so each thread (or each independent reader) can queue and drain
``Get()`` requests on its own:

.. code-block:: c++

   auto ctx = engine.NewGetContext();
   if (!ctx) {
       // Engine does not support concurrent contexts; fall back to the
       // single-threaded Selection-based Get above.
   }

   auto sel = adios2::Selection::BoundingBox({0, 0}, {10, 20}).WithSteps(0, 1);
   engine.Get(*ctx, var, data, sel);
   engine.PerformGets(*ctx);

Each ``GetContext`` owns its own queue of pending requests, so
``PerformGets(ctxA)`` drains only ``ctxA`` and leaves any requests queued on
``ctxB`` untouched. A context is reusable after ``PerformGets``; the queue is
cleared on the success path. Calls in ctx-form are always deferred — there is
no ``Mode::Sync`` variant.

``NewGetContext()`` is a feature probe: it returns ``nullptr`` if the engine
does not support concurrent contexts. Currently only the **BP5** reader
supports them, and only when:

- the engine was opened in ``Mode::ReadRandomAccess`` (no step lifecycle to
  race on), and
- the underlying file transport is reentrant for ``Read`` (the POSIX
  transport is; ``fstream`` and others are not).

The legacy ``Selection``-less ``Get()``/``PerformGets()`` API and the
single-threaded Selection-based ``Get()`` are unchanged; the ctx-form is an
additional API for callers that need concurrency.

Compatibility
-------------

The existing ``Variable::SetSelection()``, ``SetBlockSelection()``,
``SetStepSelection()``, and ``SetMemorySelection()`` APIs remain available and
are not deprecated. The ``Selection`` class is a convenience alternative that
makes the selection explicit at the ``Get()`` call site rather than relying on
state previously set on the ``Variable``.
