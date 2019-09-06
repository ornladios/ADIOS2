***
BP4 
***

The BP4 Engine writes and reads files in ADIOS2 native binary-pack (bp version 4) format. 
This is a new format for ADIOS 2.x which improves on the metadata operations of the older BP3 format. 
Compared to the older format, BP4 provides two main advantages:

  * Fast and safe **appending** of multiple output steps into the same file. Better performance than writing new files each step. 
    Existing steps cannot be corrupted by appending new steps. 
  * **Streaming** through files (i.e. online processing). Consumer apps can read existing steps while the Producer is still writing new steps.
    Reader's loop can block (with timeout) and wait for new steps to arrive. Same reader code can read the entire data in post or in situ.
    No restrictions on the Producer.  
    
BP4 files have the following structure given a "name" string passed as the first argument of ``IO::Open``:

.. code-block:: c++

   io.SetEngine("BP4");
   adios2::Engine bpFile = io.Open("name", adios2::Mode::Write);

will generate:

.. code-block:: bash

   % BP4 datasets are always a directory
   name.bp/

   % data and metadata files
   name.bp/
           data.0
           data.1
           ...
           data.M
           md.0
           md.idx

.. note::

   BP4 file names are compatible with the Unix (``/``) and Windows (``\\``) file system naming convention for directories and files.


This engine allows the user to fine tune the buffering operations through the following optional parameters:

1. **Profile**: turns ON/OFF profiling information right after a run

2. **ProfileUnits**: set profile units according to the required measurement scale for intensive operations

3. **Threads**: number of threads provided from the application for buffering, use this for very large variables in data size

4. **InitialBufferSize**: initial memory provided for buffering (minimum is 16Kb)

5. **BufferGrowthFactor**: exponential growth factor for initial buffer > 1, default = 1.05.

6. **MaxBufferSize**: maximum allowable buffer size (must be larger than 16Kb). If too large adios2 will throw an exception.

7. **FlushStepsCount**: users can select how often to produce the more expensive collective metadata file in terms of steps: default is 1. Increase to reduce adios2 collective operations footprint, with the trade-off of reducing checkpoint frequency. Buffer size will increase until first steps count if ``MaxBufferSize`` is not set.

8. **SubStreams**: (MPI-only) users can select how many sub-streams (``M`` sub-files) are produced during a run, ranges between 1 and the number of mpi processes from ``MPI_Size`` (``N``), adios2 will internally aggregate data buffers (``N-to-M``) to output the required number of sub-files. If Substream is out of bounds it will pick either 1 (``SubStreams`` < ``1 -> N-to-1``) or ``N`` ((``SubStreams`` > ``N -> N-to-N``) and ADIOS2 will issue a WARNING message. Use for performance tuning.

9. **OpenTimeoutSecs**: (Streaming mode) Reader may want to wait for the creation of the file in ``io.Open()``. By default the Open() function returns with an error if file is not found.

10. **BeginStepPollingFrequencySecs**: (Streaming mode) Reader can set how frequently to check the file (and file system) for new steps. Default is 1 seconds which may be stressful for the file system and unnecessary for the application.

11. **StatsLevel**: Turn on/off calculating statistics for every variable (Min/Max). Default is On. It has some cost to generate this metadata so it can be turned off if there is no need for this information.

12. **StatsBlockSize**: Calculate Min/Max for a given size of each process output. Default is one Min/Max per writer. More fine-grained min/max can be useful for querying the data. 

13. **Node-Local**: For distributed file system. Every writer process must make sure the .bp/ directory is created on the local file system. Required for using local disk/SSD/NVMe in a cluster.  

============================== ===================== ===========================================================
 **Key**                       **Value Format**      **Default** and Examples
============================== ===================== ===========================================================
 Profile                        string On/Off         **On**, Off
 ProfileUnits                   string                **Microseconds**, Milliseconds, Seconds, Minutes, Hours
 Threads                        integer > 1           **1**, 2, 3, 4, 16, 32, 64
 InitialBufferSize              float+units >= 16Kb   **16Kb**, 10Mb, 0.5Gb
 MaxBufferSize                  float+units >= 16Kb   **at EndStep**, 10Mb, 0.5Gb
 BufferGrowthFactor             float > 1             **1.05**, 1.01, 1.5, 2
 FlushStepsCount                integer > 1           **1**, 5, 1000, 50000
 SubStreams                     integer >= 1          **MPI_Size (N-to-N)**, ``MPI_Size``/2, ... , 2, (N-to-1) 1
 OpenTimeoutSecs                float                 **0**, ``10.0``, ``5``
 BeginStepPollingFrequencySecs  float                 **1**, ``10.0`` 
 StatsLevel                     integer, 0 or 1       **1**, ``0``
 StatsBlockSize                 integer > 0           **a very big number**, ``1073741824`` for blocks with 1M elements
 Node-Local                     string On/Off         **Off**, On
============================== ===================== ===========================================================


Only file transport types are supported. Optional parameters for ``IO::AddTransport`` or in runtime config file transport field:

**Transport type: File**

============= ================= ================================================
 **Key**       **Value Format**  **Default** and Examples
============= ================= ================================================
 Library           string        **POSIX** (UNIX), **FStream** (Windows), stdio
============= ================= ================================================


