********************
Inline for zero-copy
********************

The Inline engine provides in-process communication between the writer and reader, and seeks to avoid copying data buffers.

This engine is experimental, and is focused on the N -> N case: N writers share a process with N readers, and the analysis happens 'inline' without writing the data to a file or copying to another buffer. It has similar considerations to the streaming SST engine, since analysis must happen per step.

To use this engine, you can either specify it in your XML config file, with tag ``<engine type=Inline>`` or set it in your application code:

.. code-block:: c++

    adios2::IO inlineIO = adios.DeclareIO("ioName");
    inlineIO.SetEngine("Inline");
    inlineIO.SetParameters({{"writerID", "inline_write"}, {"readerID", "inline_read"}});
    adios2::Engine inlineWriter = inlineIO.Open("inline_write", adios2::Mode::Write);
    adios2::Engine inlineReader = inlineIO.Open("inline_read", adios2::Mode::Read);

Notice that unlike other engines, the reader and writer share an IO instance. Also, the ``writerID`` parameter allows the reader to connect to the writer, and ``readerID`` allows writer to connect to the reader. Both the writer and reader must be opened before either tries to call BeginStep/PerformPuts/PeformGets.

For successful operation, the writer will perform a step, then the reader will perform a step in the same process. Data is decomposed between processes, and the writer can write its portion of the data like other ADIOS engines. When the reader starts its step, the only data it has available is that written by the writer in its process. To select this data in ADIOS, use a block selection. The reader then can retrieve whatever data was written by the writer. The reader does require the use of a new ``Get()`` call that was added to the API:

.. code-block:: c++

    void Engine::Get<T>(                                       \
        Variable<T>, typename Variable<T>::Info & info, const Mode);

This version of ``Get`` is only used for the inline engine and requires passing a ``Variable<T>::Info`` object, which can be obtained from calling the reader's ``BlocksInfo()``. See the example below for details.

.. note::
 This ``Get()`` method is preliminary and may be removed in the future when the span interface on the read side becomes available.

.. note::
 The inline engine does not support Sync mode for writing. In addition, since the inline engine does not do any data copy, the writer should avoid changing the data contents before the reader has read the data.

Typical access pattern:

.. code-block:: c++

    // ... Application data generation

    inlineWriter.BeginStep();
    inlineWriter.Put(var, data); // always use deferred mode
    inlineWriter.EndStep();
    // Unlike other engines, data should not be reused at this point (since ADIOS
    // does not copy the data), though ADIOS cannot enforce this.
    // Must wait until reader is finished using the data.

    inlineReader.BeginStep();
    auto blocksInfo = inlineReader.BlocksInfo(var, step);
    for (auto& info : blocksInfo)
    {
        var.SetBlockSelection(info.BlockID);
        inlineReader.Get(var, info);
    }
    inlineReader.EndStep();

    // do application analysis -
    // use info.Data() to get the pointer for each element in blocksInfo

    // After any desired analysis is finished, writer can now reuse data pointer


Parameters:

1. **writerID**: Match the string passed to the ``IO::Open()`` call when creating the writer. The reader uses this parameter to fetch the correct writer.
2. **readerID**: Match the string passed to the ``IO::Open()`` call when creating the reader. The writer uses this parameter to fetch the correct reader.

===========  ===================== ===============================
 **Key**        **Value Type**       **Default** and Examples
===========  ===================== ===============================
 writerID         string             none, match the writer name
 readerID         string             none, match the reader name
===========  ===================== ===============================
