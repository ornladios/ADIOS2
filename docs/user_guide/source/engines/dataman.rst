******************************************
DataMan for Wide Area Network Data Staging
******************************************

The DataMan engine is designed for data transfers over the wide area network. To use this engine, you can either specify it in your xml config file, with tag ``<engine type=DataMan>`` or set it in your application code:

.. code-block:: c++

 adios2::IO datamanIO = adios.DeclareIO("ioName");
 datamanIO.SetEngine("DataMan");
 adios2::Engine datamanReader = datamanIO.Open(filename, adios2::Mode::Write);

On the reader side you need to do instead:

.. code-block:: c++

 adios2::IO datamanIO = adios.DeclareIO("ioName");
 datamanIO.SetEngine("DataMan");
 adios2::Engine datamanReader = datamanIO.Open(filename, adios2::Mode::Read);

.. note::
 The DataMan engine currently does not support data staging within a cluster.

The DataMan engine takes the following parameters:

1. ``IPAddress``: No default value. The IP address of the host where the writer application runs.
   This parameter is compulsory in wide area network data staging.

2. ``Port``: Default **50001**. The port number on the writer host that will be used for data transfers.

3. ``Timeout``: Default **5**. Timeout in seconds to wait for every send / receive operation.
   Packages not sent or received within this time are considered lost.

4. ``AlwaysProvideLatestTimestep``: Default **TRUE**.
This is a boolean parameter that affects what
of the available timesteps will be provided to the reader engine.  If
AlwaysProvideLatestTimestep is **TRUE**, then if there are multiple
timesteps available to the reader, older timesteps will be skipped and
the reader will see only the newest available upon BeginStep.
This value is interpreted by only by the DataMan Reader engine.
If AlwaysProvideLatestTimestep is **FALSE**, then the reader engine
will be provided with the oldest step that has not been processed.

5. ``OneToOneMode``: Default **FALSE**. It is recommended that this parameter is set to TRUE when
   there is only one writer process and only one reader process. This will explicitly tell both the
   writer engine and the reader engine that it only needs to connect to a single writer or reader,
   and thus saves the handshake overhead.

=============================== ================== ================================================
 **Key**                         **Value Format**   **Default** and Examples
=============================== ================== ================================================
 IPAddress                       string             **N/A**, 22.195.18.29
 Port                            integer            **50001**, 22000, 33000
 Timeout                         integer            **5**, 10, 30
 AlwaysProvideLatestTimestep     boolean            **TRUE**, false
 OneToOneMode                    boolean            **FALSE**, true
=============================== ================== ================================================


