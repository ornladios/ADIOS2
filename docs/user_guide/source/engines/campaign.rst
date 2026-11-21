***************
Campaign Reader
***************

The Campaign Reader Engine is a specialty engine made for reading Campaign files (.aca) created with `hpc-campaign <https://hpc-campaign.readthedocs.io>`_  tools. See an `example <../advanced/campaign_management.html>`_.

It is an engine, that uses other engine (BP5, HDF5, Timeseries) to open ADIOS/HDF5 datasets added to the campaign file. It directly supports reading images and text files added to the campaign file. 

1. **hostname**: Use this name as local hostname. If the host of a replica of a dataset in the campaign file matches this hostname, it will be regarded as locally readable replica (i.e., its path must be a local path). The default hostname is read from *~/.config/hpc-campaign/config.yaml*.

#. **campaignstorepath**: Use this base path to find campaign files using their relative path/name. Having all campaign files organized under a common base path allows for finding them using a simple relative path under that base path. The default value is read from *~/.config/hpc-campaign/config.yaml*.

#. **cachepath**: Local disk path where extracted pieces from the campaign file are stored, as well as data downloaded by ADIOS when reading remote data. The default value is read from *~/.config/hpc-campaign/config.yaml*.

#. **include-dataset**: As a campaign file contains more and more datasets the open time will increase linearly since ADIOS has to create a reading engine for each BP/HDF5 file, to open that dataset and process it's metadata. If one is only interested in specific datasets in the campaign, a semicolon-separated (;) list of regular expressions can be provided to only open datasets that match one of those expressions. Note that these are NOT paths with wildcards but regular expressions, so ``?`` and ``*`` is not going to work as you expect but ``.`` and ``.*`` will.

#. **exclude-dataset**: Similar to the previous option, datasets can be excluded from processing by providing a semicolon-separated list of regular expressions. 

#. **verbose**: print debug information on what is happening when reading a campaign file. 
   
=============================== ===================== ===========================================================
 **Key**                        **Value Format**      **Default** and Examples
=============================== ===================== ===========================================================
 hostname                        string               **hostname** from *~/.config/hpc-campaign/config.yaml*
 campaignstorepath               string               **campaignstorepath** from *~/.config/hpc-campaign/config.yaml*
 cachepath                       string               **cachepath** from *~/.config/hpc-campaign/config.yaml*
 include-dataset                 string               "include-dataset=.*/images;.*/data"
 exclude-dataset                 string               "exclude-dataset=logs/.*"
 verbose                         integer              **0**, 0..5
=============================== ===================== ===========================================================

Campaign Index Reader
=====================

The ``CampaignIndex`` engine opens SQLite campaign indexes (``.acx``) created
by hpc-campaign. The file extension selects this engine automatically, or an
application can select it with ``IO::SetEngine("CampaignIndex")``. It requires
the same Campaign build option as the Campaign Reader.

Open creates all indexed variables and attributes directly from the index,
without opening any campaign archive or dataset. Names have the form
``archive/dataset/entry``, including the archive's stored path and extension.
Attribute names retain their stored spelling, including leading slashes.
The SQLite connection is read-only and is released when Open finishes.

Metadata loading uses one SQLite streamer and a fixed pool of workers. Workers
decode shapes, scalar summaries, and attribute JSON and construct private ADIOS
definition batches. The calling thread merges completed batches in stream order;
workers never access the SQLite connection or modify the destination IO maps.
Work is split by entry count, so even one archive containing one large BP file
can use all workers. At most one batch per worker plus the streamer's current
batch is in flight, avoiding a second complete decoded copy of the index.

The following engine parameters control metadata loading (case insensitive):

* ``MetadataThreads``: number of definition workers, defaulting to the smaller
  of 4 and the detected hardware concurrency (at least 1). Setting it to ``1``
  processes batches on the calling thread without starting worker threads.
* ``MetadataBatchSize``: maximum number of variables and attributes in one
  batch, default ``4096``. Both parameters must be positive integers.

Batches amortize scheduling and merge overhead across many entries. Smaller
batches balance uneven metadata costs more closely; larger batches reduce
handoffs but retain more temporary metadata. Dataset/archive boundaries do not
limit parallelism. Duplicate indexed definition names are rejected. A failed
Open joins all workers before propagating the error; definitions already merged
into the IO remain installed, so use a fresh IO when retrying a failed load.

Shapes and min/max are index summaries; the index does not contain block
decomposition, per-step shapes, or per-step extrema. Block inquiries for ADIOS
and HDF5 variables open the source dataset lazily and return its block metadata.
This includes ``Start()``, ``Count()``, and ``SelectionSize()`` after a block
selection, as well as the block-info APIs. If the source cannot be opened, these
inquiries report an error. Ordinary global-array listings (``bpls -l``) use the
index summaries; ``bpls -lD`` retrieves the original blocks. Step numbers are
interpreted as contiguous, starting at zero, including across timeseries members.

Get supports ADIOS and HDF5 variables, image/text bytes, and parsed scalar
values cached in the index. Parsed TEXT fields containing JSON objects or arrays
are exposed as scalar strings containing the serialized JSON.
Dataset reads resolve replicas from the corresponding
campaign archive and open the underlying BP or HDF5 dataset lazily. The archive
is read as SQLite, never opened through an ADIOS engine. Opened dataset engines
are reused for subsequent reads and released by Close. Spatial, block, memory,
and step selections are forwarded to the dataset engine. Deferred requests
retain their selections and complete at PerformGets, EndStep, or Close.
For SSH access, BP4 members use the remote file transport to read payload
subfiles; embedded metadata alone does not make those payloads local.

Indexes with ``timeseries``, ``timeseries_datasets``, and
``timeseries_variable_steps`` tables resolve series membership and per-variable
step ranges directly from the index. A logical series dataset may have a negative
``dsid``; member datasets contain the physical ACA dataset IDs. Each variable's
``step_start`` and ``steps`` ranges must cover its indexed steps contiguously from
zero. These ranges select the member and its local variable steps without opening
preceding members. Missing variables in other members do not introduce gaps.
Malformed or incomplete mappings are rejected at Open. The ACA is still needed
to resolve replicas when reading data or block metadata.

Older indexes without these tables resolve timeseries names against the archive's
timeseries table and concatenate member variable steps in ``tsorder`` order.
Both formats support selections spanning member datasets, random access, and
step-by-step reading.

For example, to measure the complete listing while excluding terminal rendering::

    time bpls -l /path/to/kstar.acx > /dev/null

All metadata is materialized in memory, so very large indexes require memory
proportional to the number of variables and attributes.
