****
BP3 
****

The BP3 Engine writes and reads files in ADIOS2 native binary-pack (bp) format. BP files are backwards compatible with ADIOS1.x and have the following structure given a "name" string passed as the first argument of ``IO::Open``:

.. code-block:: c++

   adios2::Engine bpFile = io.Open("name", adios2::Mode::Write);

will generate:

.. code-block:: bash

   % collective metadata file
   name.bp

   % data directory and files
   name.bp.dir/
               name.bp.0
               name.bp.1
               ...
               name.bp.M

.. note::

   BP3 file names are compatible with the Unix (``/``) and Windows (``\\``) file system naming convention for directories and files.

.. caution::

   The default BP3 engine will check if the ``.bp`` is the extension of the first argument of ``IO::Open`` and will add ``.bp`` and ``.bp.dir`` if not.

This engine allows the user to fine tune the buffering operations through the following optional parameters:

1. **Profile**: turns ON/OFF profiling information right after a run

2. **ProfileUnits**: set profile units according to the required measurement scale for intensive operations

3. **CollectiveMetadata**: turns ON/OFF forming collective metadata during run (used by large scale HPC applications)

4. **Threads**: number of threads provided from the application for buffering, use this for very large variables in data size

5. **InitialBufferSize**: initial memory provided for buffering (minimum is 16Kb)

6. **BufferGrowthFactor**: exponential growth factor for initial buffer > 1, default = 1.05.

7. **MaxBufferSize**: maximum allowable buffer size (must be larger than 16Kb). If too large adios2 will throw an exception.

8. **FlushStepsCount**: users can select how often to produce the more expensive collective metadata file in terms of steps: default is 1. Increase to reduce adios2 collective operations footprint, with the trade-off of reducing checkpoint frequency. Buffer size will increase until first steps count if ``MaxBufferSize`` is not set.

9. **SubStreams**: (MPI-only) users can select how many sub-streams (``M`` sub-files) are produced during a run, ranges between 1 and the number of mpi processes from ``MPI_Size`` (``N``), adios2 will internally aggregate data buffers (``N-to-M``) to output the required number of sub-files. If Substream is out of bounds it will pick either 1 (``SubStreams`` < ``1 -> N-to-1``) or ``N`` ((``SubStreams`` > ``N -> N-to-N``) and ADIOS2 will issue a WARNING message. Use for performance tuning.

==================== ===================== ===========================================================
 **Key**              **Value Format**      **Default** and Examples
==================== ===================== ===========================================================
 Profile              string On/Off         **On**, Off
 ProfileUnits         string                **Microseconds**, Milliseconds, Seconds, Minutes, Hours
 CollectiveMetadata   string On/Off         **On**, Off
 Threads              integer > 1           **1**, 2, 3, 4, 16, 32, 64
 InitialBufferSize    float+units >= 16Kb   **16Kb**, 10Mb, 0.5Gb
 MaxBufferSize        float+units >= 16Kb   **at EndStep**, 10Mb, 0.5Gb
 BufferGrowthFactor   float > 1             **1.05**, 1.01, 1.5, 2
 FlushStepsCount      integer > 1           **1**, 5, 1000, 50000
 SubStreams           integer >= 1          **MPI_Size (N-to-N)**, ``MPI_Size``/2, ... , 2, (N-to-1) 1
==================== ===================== ===========================================================


Only file transport types are supported. Optional parameters for ``IO::AddTransport`` or in runtime config file transport field:

**Transport type: File**

============= ================= ================================================
 **Key**       **Value Format**  **Default** and Examples
============= ================= ================================================
 Library           string        **POSIX** (UNIX), **FStream** (Windows), stdio
============= ================= ================================================


