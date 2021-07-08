*************
Plugin Engine
*************

The ``Plugin`` engine enables the ability to load an engine located in a separate library.
Your plugin class needs to inherit from the ``PluginEngineInterface`` class in the ``adios2/engine/plugin/PluginEngineInterface.h`` header.
Depending on the type of engine you want to implement, you'll need to override a number of methods that are inherited from the ``adios2::core::Engine`` class.
These are briefly described in the following table.
More detailed documentation can be found in ``adios2/core/Engine.h``.

========================= ===================== ===========================================================
 **Method**                **Engine Type**       **Description**
========================= ===================== ===========================================================
``BeginStep()``            Read/Write            Indicates the beginning of a step
``EndStep()``              Read/Write            Indicates the end of a step
``CurrentStep()``          Read/Write            Returns current step info
``DoClose()``              Read/Write            Close a particular transport
``Init()``                 Read/Write            Engine initialization
``InitParameters()``       Read/Write            Initialize parameters
``InitTransports()``       Read/Write            Initialize transports
``PerformPuts()``          Write                 Execute all deferred mode ``Put``
``Flush()``                Write                 Flushes data and metadata to a transport
``DoPut()``                Write                 Implementation for ``Put``
``DoPutSync()``            Write                 Implementation for ``Put`` (Sync mode)
``DoPutDeferred()``        Write                 Implementation for ``Put`` (Deferred Mode)
``PerformGets()``          Read                  Execute all deferred mode ``Get``
``DoGetSync()``            Read                  Implementation for ``Get`` (Sync mode)
``DoGetDeferred()``        Read                  Implementation for ``Get`` (Deferred Mode)
========================= ===================== ===========================================================

Examples showing how to implement an engine plugin can be found in ``examples/plugins/engine``.
An example write engine is ``ExampleWritePlugin.h``, while an example read engine is in ``ExampleReadPlugin.h``.
The writer is a simple file writing engine that creates a directory (called ``ExamplePlugin`` by default) and writes variable information to vars.txt and actual data to data.txt.
The reader example reads the files output by the writer example.

In addition to implementing the methods above, you'll need to implement ``EngineCreate()`` and ``EngineDestroy`` functions so the Plugin Engine can create/destroy the engine object.
Because of C++ name mangling, you'll need to use ``extern "C"``.
Looking at ``ExampleWritePlugin.h``, this looks like:

.. code-block:: c++

    extern "C" {

    adios2::core::engine::ExampleWritePlugin *
    EngineCreate(adios2::core::IO &io, const std::string &name,
                 const adios2::Mode mode, adios2::helper::Comm comm)
    {
        return new adios2::core::engine::ExampleWritePlugin(io, name, mode,
                                                            comm.Duplicate());
    }

    void EngineDestroy(adios2::core::engine::ExampleWritePlugin * obj)
    {
        delete obj;
    }

    }

To build your plugin, your CMake should look something like:

.. code-block:: cmake

    find_package(ADIOS2 REQUIRED)
    set(BUILD_SHARED_LIBS ON)
    add_library(PluginEngineWrite
      ExampleWritePlugin.cpp
    )
    target_link_libraries(PluginEngineWrite adios2::cxx11 adios2::core)

When using the Plugin Engine, ADIOS will check for your plugin at the path specified in the ``ADIOS2_PLUGIN_PATH`` environment variable.
If ``ADIOS2_PLUGIN_PATH`` is not set, ADIOS will warn you about it, and then look for your plugin in the current working directory.

The following steps show how to use your engine plugin in your application.
``examplePluginEngine_write.cpp`` and ``examplePluginEngine_read.cpp`` are an example of how to use the engine plugins described above.
The key steps to use your plugin are:

1. Set engine to ``Plugin``. i.e.:

.. code-block:: c++

   io.SetEngine("Plugin");

2. Set ``PluginName`` (optional) and ``PluginLibrary`` (required) parameters.
   If you don't set ``PluginName``, the Plugin Engine will give your plugin a default name of ``UserPlugin``.
   In the write example, this looks like

.. code-block:: c++

   io.SetParameters({{"PluginName", "WritePlugin"}});
   io.SetParameters({{"PluginLibrary", "PluginEngineWrite"}});

.. note::
    You don't need to add the ``lib`` prefix or the shared library ending (e.g., ``.so``, ``.dll``, etc.).
    ADIOS will add these when searching for your plugin library.

At this point you can open the engine and use it as you would any other ADIOS engine.
You also shouldn't need to make any changes to your CMake files for your application.
