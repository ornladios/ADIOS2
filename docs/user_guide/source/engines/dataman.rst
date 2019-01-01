***************
DataMan for WAN
***************

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

The DataMan engine does not accept any parameters. However, users are allowed to specify the following transport parameters:

1. **Library**: the underlying network / socket library used for data transfer.

2. **IPAddress**: the IP address of the host where the writer application runs.

3. **Port**: the port on the writer host that will be used for data transfers.

4. **Timeout**: the timeout in seconds to wait for every send / receive.

============= ================= ================================================
 **Key**       **Value Format**  **Default** and Examples
============= ================= ================================================
 Library           string        **ZMQ**
 IPAddress         string        **127.0.0.1**, 22.195.18.29
 Port              integer       **12306**, 22000, 33000
 Timeout           integer       **5**, 10, 30
============= ================= ================================================


