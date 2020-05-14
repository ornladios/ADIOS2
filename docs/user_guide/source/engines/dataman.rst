******************************************
DataMan for Wide Area Network Data Staging
******************************************

The DataMan engine is designed for data staging over the wide area network.
It is supposed to be used in cases where a few writers send data to a few readers
over long distance.

DataMan does NOT guarantee that readers receive EVERY data step
from writers. The idea behind this is that for experimental data, which is the target
use case of this engine, the data rate of writers should not be slowed down
by readers. If readers cannot keep up with the experiment, the experiment should still
continue, and the readers should read the latest data steps. The design also helps
improving performance because it saves the communication time for checking step completeness,
which usually means ~100 milliseconds every step for transoceanic connections.

For wide area data staging applications that require readers to receive EVERY data step,
the SST engine is recommended.

The DataMan engine takes the following parameters:

1. ``IPAddress``: No default value. The IP address of the host where the writer application runs.
   This parameter is compulsory in wide area network data staging.

2. ``Port``: Default **50001**. The port number on the writer host that will be used for data transfers.

3. ``Timeout``: Default **5**. Timeout in seconds to wait for every send / receive operation.
   Packages not sent or received within this time are considered lost.

4. ``RendezvousReaderCount``: Default **1**. This integer value specifies the number of readers for which the writer should wait before the writer-side Open() returns.
   By default, an early-starting writer will wait for the reader to start, or vice versa.
   A number >1 will cause the writer to wait for more readers, and a value of 0 will allow the writer to proceed without any readers present.
   This value is interpreted by DataMan Writer engines only.

5. ``DoubleBuffer``: Default **true** for reader, **false** for writer. Whether to use double buffer for caching send and receive operations.
   Enabling double buffer will cause extra overhead for managing threads and buffer queues, but will improve the continuity of data steps for the reader, for the pub/sub mode.
   Advice for generic uses cases is to keep the default values, true for reader and false for writer.

6. ``TransportMode``: Default **fast**. The fast mode is optimized for latency-critical applications.
   It enforces readers to only receive the latest step.
   Therefore, in cases where writers are faster than readers, readers will skip some data steps.
   The reliable mode ensures that all steps are received by readers, by sacrificing performance compared to the fast mode.

=============================== ================== ================================================
 **Key**                         **Value Format**   **Default** and Examples
=============================== ================== ================================================
 IPAddress                       string             **N/A**, 22.195.18.29
 Port                            integer            **50001**, 22000, 33000
 Timeout                         integer            **5**, 10, 30
 RendezvousReaderCount           integer            **1**, 0, 3
 DoubleBuffer                    bool               **true** for reader, **false** for writer
 TransportMode                   string             **fast**, reliable
=============================== ================== ================================================


