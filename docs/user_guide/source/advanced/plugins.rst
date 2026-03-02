#########
 Plugins
#########

ADIOS now has the ability for users to load their own engines and operators through the plugin interface.
The basic steps for doing this are:

1. Write your plugin class, which needs to inherit from the appropriate ``Plugin*Interface`` class.
2. Build as a shared library and add the path to your shared library to the ``ADIOS2_PLUGIN_PATH`` environment variable.
3. Start using your plugin in your application.

These steps are discussed in further detail below.

*************************
Writing Your Plugin Class
*************************


Engine Plugin
-------------

Your engine plugin class needs to inherit from the ``PluginEngineInterface`` class in the ``adios2/plugin/PluginEngineInterface.h`` header.
The plugin interface is a standalone class that uses the public C++ API types (``adios2::IO``, ``adios2::Variable<T>``) rather than internal core types.
All virtual methods except ``BeginStep()``, ``EndStep()``, and ``CurrentStep()`` have empty default implementations, so you only need to override the methods relevant to your engine.

The following table describes the available methods.

**Required methods** (pure virtual, must be overridden):

================================= ===================== ===========================================================
 **Method**                        **Engine Type**       **Description**
================================= ===================== ===========================================================
``BeginStep()``                    Read/Write            Indicates the beginning of a step
``EndStep()``                      Read/Write            Indicates the end of a step
``CurrentStep()``                  Read/Write            Returns current step
================================= ===================== ===========================================================

**Core I/O methods** (override the types your engine supports):

================================= ===================== ===========================================================
 **Method**                        **Engine Type**       **Description**
================================= ===================== ===========================================================
``DoPutSync()``                    Write                 Implementation for ``Put`` (Sync mode)
``DoPutDeferred()``                Write                 Implementation for ``Put`` (Deferred mode)
``DoGetSync()``                    Read                  Implementation for ``Get`` (Sync mode)
``DoGetDeferred()``                Read                  Implementation for ``Get`` (Deferred mode)
``PerformPuts()``                  Write                 Execute all deferred mode ``Put``
``PerformGets()``                  Read                  Execute all deferred mode ``Get``
``DoClose()``                      Read/Write            Close the engine
``Init()``                         Read/Write            Called after construction for initialization
================================= ===================== ===========================================================

**Advanced methods** (optional, override for additional capabilities):

================================= ===================== ===========================================================
 **Method**                        **Engine Type**       **Description**
================================= ===================== ===========================================================
``DoGetSync(var, data, sel)``      Read                  ``Get`` with a ``Selection`` for sub-array reads
``DoGetDeferred(var, data, sel)``  Read                  Deferred ``Get`` with a ``Selection``
``DoPut(var, span, init, val)``    Write                 Span-based ``Put`` for zero-copy writes
``Steps()``                        Read                  Returns the total number of available steps
``MinBlocksInfo(name, step)``      Read                  Returns block decomposition info for a variable
================================= ===================== ===========================================================

Examples showing how to implement an engine plugin can be found in ``examples/plugins/engine``.
An example write engine is ``ExampleWritePlugin.h``, while an example read engine is in ``ExampleReadPlugin.h``.
The writer is a simple file writing engine that creates a directory (called ``ExamplePlugin`` by default) and writes variable information to vars.txt and actual data to data.txt.
The reader example reads the files output by the writer example.

In addition to implementing the methods above, you'll need to implement ``EngineCreate()`` and ``EngineDestroy()`` functions so ADIOS can create/destroy the engine object.
Because of C++ name mangling, you'll need to use ``extern "C"``.
Looking at ``ExampleWritePlugin.h``, this looks like:

.. code-block:: c++

    extern "C" {

    adios2::plugin::ExampleWritePlugin *
    EngineCreate(adios2::IO io, const std::string &name,
                 const adios2::Mode mode)
    {
        return new adios2::plugin::ExampleWritePlugin(io, name, mode);
    }

    void EngineDestroy(adios2::plugin::ExampleWritePlugin * obj)
    {
        delete obj;
    }

    }

Operator Plugin
---------------

Your operator plugin class needs to inherit from the ``PluginOperatorInterface`` class in the ``adios2/plugin/PluginOperatorInterface.h`` header.
The operator plugin interface is a standalone class that does not depend on any ADIOS internal types.
You must override the three pure virtual methods listed below. Additional optional methods are available for more advanced use.

**Required methods** (pure virtual, must be overridden):

================================= ===========================================================
 **Method**                        **Description**
================================= ===========================================================
``Operate()``                      Performs the operation, e.g., compress data
``InverseOperate()``               Performs the inverse operation, e.g., decompress data
``IsDataTypeValid()``              Checks that a given data type can be processed
================================= ===========================================================

**Optional methods** (override for additional capabilities):

================================= ===========================================================
 **Method**                        **Description**
================================= ===========================================================
``GetEstimatedSize()``             Upper bound estimate of transformed data size
``AddExtraParameters()``           Receive per-invocation parameters
================================= ===========================================================

An example showing how to implement an operator plugin can be found at ``examples/plugins/operator``.
This operator uses `libsodium <https://doc.libsodium.org/>`_ for encrypting and decrypting data.

In addition to implementing the methods above, you'll need to implement ``OperatorCreate()`` and ``OperatorDestroy()`` functions so ADIOS can create/destroy the operator object.
Because of C++ name mangling, you'll need to use ``extern "C"``.
Looking at ``EncryptionOperator``, this looks like:

.. code-block:: c++

    extern "C" {

    adios2::plugin::EncryptionOperator *
    OperatorCreate(const adios2::Params &parameters)
    {
        return new adios2::plugin::EncryptionOperator(parameters);
    }

    void OperatorDestroy(adios2::plugin::EncryptionOperator * obj)
    {
        delete obj;
    }

    }

********************
Build Shared Library
********************

To build your plugin, your CMake should look something like the following (using the plugin engine example described above):

.. code-block:: cmake

    find_package(ADIOS2 REQUIRED)
    set(BUILD_SHARED_LIBS ON)
    add_library(PluginEngineWrite
      ExampleWritePlugin.cpp
    )
    target_link_libraries(PluginEngineWrite adios2::cxx)

When using the Plugin Engine, ADIOS will check for your plugin at the path specified in the ``ADIOS2_PLUGIN_PATH`` environment variable.
If ``ADIOS2_PLUGIN_PATH`` is not set, and a path is not specified when loading your plugin (see below steps for using a plugin in your application), then the usual ``dlopen`` search is performed (see the `dlopen man page <https://man7.org/linux/man-pages/man3/dlopen.3.html>`_).

.. note::
    The ``ADIOS2_PLUGIN_PATH`` environment variable can contain multiple paths, which must be separated with a ``:``.


When building on Windows, you will likely need to explicitly export the Create and Destroy symbols for your plugin, as symbols are invisible by default on Windows.
To do this in a portable way across platforms, you can add something similar to the following lines to your CMakeLists.txt:

.. code-block:: cmake

    include(GenerateExportHeader)
    generate_export_header(PluginEngineWrite BASE_NAME plugin_engine_write)
    target_include_directories(PluginEngineWrite PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include>)


Then in your plugin header, you'll need to ``#include "plugin_engine_write_export.h"``. Then edit your function definitions as follows:

.. code-block:: c++

    extern "C" {

    PLUGIN_ENGINE_WRITE_EXPORT adios2::plugin::ExampleWritePlugin *
        EngineCreate(adios2::IO io, const std::string &name,
                     const adios2::Mode mode);

    PLUGIN_ENGINE_WRITE_EXPORT void
        EngineDestroy(adios2::plugin::ExampleWritePlugin * obj);

    }


***********************************
Using Your Plugin in an Application
***********************************

For both types of plugins, loading the plugin is done by setting the ``PluginName`` and ``PluginLibrary`` parameters in an  ``adios2::Params`` object or ``<parameter>`` XML tag.

Engine Plugins
--------------

For engine plugins, this looks like:

.. code-block:: c++

    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("writer");
    io.SetEngine("Plugin");
    adios2::Params params;
    params["PluginName"] = "WritePlugin";
    params["PluginLibrary"] = "PluginEngineWrite";
    // If the engine plugin has any other parameters, these can be added to
    // the same params object and they will be forwarded to the engine
    io.SetParameters(params);

Where "WritePlugin" is the name that ADIOS will use to keep track of the plugin, and "PluginEngineWrite" is the shared library name.
At this point you can open the engine and use it as you would any other ADIOS engine.
You also shouldn't need to make any changes to your CMake files for your application.

The second option is using an ADIOS XML config file. If you'd like to load your plugins through an XML config file, the following shows an example XML when using Engine Plugins:

.. code-block:: xml

    <adios-config>
        <io name="writer">
            <engine type="Plugin">
                <parameter key="PluginName" value="WritePlugin" />
                <parameter key="PluginLibrary" value="PluginEngineWrite" />
                <!-- any parameters needed for your plugin can be added here in the parameter tag -->
            </engine>
        </io>
        <io name="reader">
            <engine type="Plugin">
                <parameter key="PluginName" value="ReadPlugin" />
                <parameter key="PluginLibrary" value="PluginEngineRead" />
                <!-- any parameters needed for your plugin can be added here in the parameter tag -->
            </engine>
        </io>
    </adios-config>

The examples ``examples/plugins/engine/examplePluginEngine_write.cpp`` and ``examples/plugins/engine/examplePluginEngine_read.cpp`` are an example of how to use the engine plugins described above.


Operator Plugins
----------------

For operator plugins, the code to use your plugin looks like:

.. code-block:: c++

    // for an adios2::Variable<T> var
    adios2::Params params;
    params["PluginName"] = "MyOperator";
    params["PluginLibrary"] = "EncryptionOperator";
    // example param required for the EncryptionOperator
    params["SecretKeyFile"] = "test-key";
    var.AddOperation("plugin", params);

If you'd like to load your operator plugin through an XML config file, the following shows an example:

.. code-block:: xml

    <adios-config>
        <io name="writer">
            <variable name="data">
                <operation type="plugin">
                    <parameter key="PluginName" value="OperatorPlugin"/>
                    <parameter key="PluginLibrary" value="EncryptionOperator" />
                    <parameter key="SecretKeyFile" value="test-key" />
                </operation>
            </variable>
            <engine type="BP5">
            </engine>
        </io>
    </adios-config>


The examples ``examples/plugins/operator/examplePluginOperator_write.cpp`` and ``examples/plugins/engine/examplePluginOperator_read.cpp`` show an example of how to use the ``EncryptionOperator`` plugin described above.

.. note::
    You don't need to add the ``lib`` prefix or the shared library ending (e.g., ``.so``, ``.dll``, etc.) when 
    setting ``PluginLibrary``.
    ADIOS will add these when searching for your plugin library.
    If you do add the prefix/suffix, ADIOS will still be able to find your plugin.
    It's also possible to put the full path to the shared library here, instead of using ``ADIOS2_PLUGIN_PATH``.
