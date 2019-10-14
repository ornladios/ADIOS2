********************
Inline for zero-copy
********************

The Inline engine provides in-process communication between the writer and reader, and seeks to avoid copying data buffers.

This engine is experimental, and is focused on the N -> N case: N writers share a process with N readers, and the analysis happens 'inline' without writing the data to a file or copying to another buffer. It has similar considerations to the streaming SST engine, since analysis must happen per step.

To use this engine, you can either specify it in your xml config file, with tag ``<engine type=Inline>`` or set it in your application code:

.. code-block:: c++

    adios2::IO inlineIO = adios.DeclareIO("ioName");
    inlineIO.SetEngine("Inline");
    adios2::Engine inlineWriter = inlineIO.Open("inline_write", adios2::Mode::Write);
    io.SetParameters({{"writerID", "inline_write"}});
    adios2::Engine inlineReader = inlineIO.Open("inline_read", adios2::Mode::Read);

Notice that unlike other engines, the reader and writer share an IO instance. Also, the ``writerID`` parameter allows the reader to connect to the writer.

For successful operation, the writer will perform a step, then the reader will perform a step in the same process. Data is decomposed between processes, and the writer can write its portion of the data like other Adios engines. When the reader starts its step, the only data it has available is that written by the writer in its process. To select this data in Adios, use a block selection. The reader then can retrieve whatever data was written by the writer.

Please refer to the examples or tests for the typical access pattern. Note, however, that the ``Get()`` method:

.. code-block:: c++

 void Engine::Get<T>(                                       \
        Variable<T>, typename Variable<T>::Info & info, const Mode);

is preliminary, and may change in the future.

Parameters:

1. **writerID**: Match the string passed to the ``IO::Open()`` call when creating the writer. The reader uses this parameter to fetch the correct writer.

===========  ===================== ===============================
 **Key**        **Value Type**       **Default** and Examples
===========  ===================== ===============================
 writerID         string             none, match the writer name
===========  ===================== ===============================
