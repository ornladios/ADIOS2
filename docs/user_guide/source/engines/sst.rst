**********************************
SST Sustainable Staging Transport
**********************************

In ADIOS2, the Sustainable Staging Transport (SST) is an engine that allows
direct connection of data producers and consumers via the ADIOS2 write/read
APIs.  This is a classic streaming data architecture where the data passed
to ADIOS on the write side (via PutDeferred(), PutSync() and similar calls)
is make directly available to a reader (via GetDeferred(), GetSync() and
similar calls).

SST is designed for use in HPC environment and can take advantage of RDMA
network interconnects to speed the transfer of data between communicating
HPC applications, however it is also capable of operating in a Wide Area
Networking environment over standard sockets.  SST supports full MxN data
distribution, where the number of reader ranks can differ from the number of
writer ranks.  SST also allows multiple reader cohorts to get access to a writers
data simultaneously.

To use this engine, you can either specify it in your xml config file, with
tag ``<engine type=SST>`` or, set it in client code. For example, here is
how to create an SST reader: 

.. code-block:: c++

 adios2::IO sstIO = adios.DeclareIO("SomeName");
 sstIO.SetEngine("SST");	
 adios2::Engine sstReader = sstIO.Open(filename, adios2::Mode::Read);	

and a sample code for SST writer is:

.. code-block:: c++

 adios2::IO sstIO = adios.DeclareIO("SomeName");
 sstIO.SetEngine("SST");	
 adios2::Engine sstWriter = sstIO.Open(filename, adios2::Mode::Write);	

The general goal of ADIOS2 is to ease the conversion of a file-based
application to instead use a non-file streaming interconnect, for example,
data producers such as computational physics codes and consumers such as
analysis applications.  However, there are some uses of ADIOS2 APIs that
work perfectly well with the ADIOS2 file engines, but which will not work or
will perform badly.  For example, SST is based upon the *"step"* concept and
ADIOS2 applications that use SST must call BeginStep() and EndStep().  On
the writer side, the Put() calls between BeginStep and EndStep are the unit
of communication and represent the data that will be available between the
corresponding Begin/EndStep calls on the reader.  

Also, it is recommended that SST-based applications not use the ADIOS2
GetSync() method unless there is only one data item to be read per step.
This is because SST implements MxN data transfer (and avoids having to
deliver all data to every reader), by queueing data on the writer ranks
until it is known which reader rank requires it.  Normally this data fetch
stage is initiated by PerformGets() or EndStep(), both of which fulfill any
pending GetDeferred() operations.  However, unlike GetDeferred(), the
semantics GetSync() require the requested data to be fetched from the
writers before the call can return.   If there are multiple calls to
GetSync() per step, each one may require a communication with many writers,
something that would have only had to happen once if GetDeferred() were used
instead.  Thus the use of GetSync() is likely to incur a substantial
performance penalty.

On the writer side, depending upon the chosen data marshaling option there
may be some (relatively small) performance differences between PutSync() and
PutDeferred(), but they are unlikely to be as substantial as between
GetSync() and GetDeferred().

Note that SST readers and writers do not necessarily move in lockstep, but
depending upon the queue length parameters and queueing policies specified,
differing reader and writer speeds may cause one or the other side to wait
for data to be produced or consumed, or data may be dropped if allowed by
the queueing policy.  However, steps themselves are atomic and no step will
be partially dropped, delivered to a subset of ranks, or otherwise divided.

The SST engine allows the user to customize the streaming operations through
the following optional parameters:

1. **RendezvousReaderCount**: Default **1**.  This integer value specifies
the number of readers for which the writer should wait before the
writer-side Open() returns.   The default of 1 implements an ADIOS1/flexpath
style "rendezvous", in which an early-starting reader will wait for the
writer to start, or vice versa.  A number >1 will cause the writer to wait
for more readers and a value of 0 will allow the writer to proceed without
any readers present.  This value is interpreted by SST Writer engines only.

2. **RegistrationMethod**:  Default **"File"**.  By default, SST reader and
writer engines communicate network contact information via files in a shared
filesystem.  Specifically, the "filename" parameter in the Open() call is
interpreted as a path which the writer uses as the name of a file to which
contact information is written, and from which a reader will attempt to read
contact information.  As with other file-based engines, file creation and
access is subject to the usual considerations (directory components are
interpreted, but must exist and be traversable, writer must be able to
create the file and the reader must be able to read it).  Generally the file
so created will exist only for as long as the writer keeps the stream
Open(), but abnormal process termination may leave "stale" files in those
locations.  These stray ".sst" files should be deleted to avoid confusing
future readers.   SST also offers a **"Screen"** registration method in which
writers and readers send their contact information to, and read it from,
stdout and stdin respectively.  The "screen" registration method doesn't
support batch mode operations in any way, but may be useful when manually
starting jobs on machines in a WAN environment that don't share a
filesystem. A future release of SST will also support a **"Cloud"**
registration method where contact information is registered to and retrieved
from a network-based third-party server so that both the shared filesystem
and interactivity can be avoided. This value is interpreted by both SST
Writer and Reader engines.

3. **QueueLimit**:  Default **0**.  This integer value specifies the number
of steps which the writer will allow to be queued before taking specific
action (such as discarding data or waiting for readers to consume the
data).  The default value of 0 is interpreted as no limit.  This value is
interpreted by SST Writer engines only. 

4. **QueueFullPolicy**: Default **"Block"**.  This value controls what
policy is invoked if a non-zero **QueueLimit** has been specified and new
data would cause the queue limit to be reached.  Essentially, the
**"Block"** option ensures data will not be discarded and if the queue fills
up the writer will block on **EndStep** until the data has been read.  If
there are no active readers, **EndStep** will block until at least one
arrives.  If there is one active reader, **EndStep** will block until data
has been consumed off the front of the queue to make room for newly arriving
data.  If there is more than one active reader, it is only removed from the
queue when it has been read by all readers, so the slowest reader will
dictate progress.  Besides **"Block"**, the other acceptable value for
**QueueFullPolicy** is **"Discard"**.  When **"Discard"** is specified, and
an **EndStep** operation would add more than the allowed number of steps to
the queue, some step is discarded.  If there are no current readers
connected to the stream, the *oldest* data in the queue is discarded.  If
there are current readers, then the *newest* data (I.E. the just-created
step) is discarded.  (The differential treatment is because SST sends
metadata for each step to the readers as soon as the step is accepted and
cannot reliably prevent that use of that data without a costly all-to-all
synchronization operation.  Discarding the *newest* data instead is less
satisfying, but has a similar long-term effect upon the set of steps
delivered to the readers.)  This value is interpreted by SST Writer engines
only.

5. **DataTransport**:  Default **"RDMA"**.   This string value specifies the
underlying network communication mechanism to use for exchanging data in
SST.  Current allowed values are **"RDMA"** and **"WAN"**.  (**ib** and
**fabric** are accepted as equivalent to **RDMA** and **evpath** is
equivalent to **WAN**.)  Generally both the reader and writer should be
using the same network transport, and the network transport chosen may be
dictated by the situation.  For example, the RDMA transport generally
operates only between applications running on the same high-performance
interconnect (e.g. on the same HPC machine).  If communication is desired
between applications running on different interconnects, the Wide Area
Network (WAN) option should be chosen.  This value is interpreted by both
SST Writer and Reader engines.

====================   ===================== =========================================================
 **Key**                **Value Format**      **Default** and Examples 
====================   ===================== =========================================================
 RendezvousReaderCount integer		     **1**
 RegistrationMethod    string                **File**, Screen
 QueueLimit            integer		     **0** (no queue limits)
 QueueFullPolicy       string	             **Block**, Discard
 DataTransport	       string		     **default varies by platform**, RDMA, WAN
====================   ===================== =========================================================
