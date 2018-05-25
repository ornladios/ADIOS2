******
BPFile
******

The BPFile Engine writes and reads files in ADIOS2 native binary-pack (bp) format. BP files are backwards compatible with ADIOS1.x and have the following structure given a "name" string passed as the first argument of ``IO::Open``:

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
   
   BPFile names are compatible with the Unix (/) and Windows (\\) file system naming convention for directories and files.

This engine allows the user to fine tune the buffering operations through the following optional parameters: 

1. **Threads**: number of threads provided from the application for buffering, use this for very large variables

2. **InitialBufferSize**: initial memory provided for buffering (minimum is 16Kb)

3. **BufferGrowthFactor**: exponential growth factor for initial buffer > 1, default = 1.05.

4. **MaxBufferSize**: maximum allowable buffer size (must be larger than 16Kb). If to large adios2 will throw an exception.

5. **FlushStepsCount**: user can select how often to produce the more expensive collective metadata file in terms of steps: default is 1. Increase to reduce adios2 collective operations footprint, with the trade-off of reducing checkpoint frequency. Buffer size will increase until first steps count if MaxBufferSize is not set.

=================== ===================== ==============================
 **Key**             **Value Format**      **Default** and Examples 
=================== ===================== ==============================
 Threads             integer > 1           **1**, 2, 3, 4, 16, 32, 64 
 InitialBufferSize   float+units >= 16Kb   **16Kb**, 10Mb, 0.5Gb 
 MaxBufferSize       float+units >= 16Kb   **at EndStep**, 10Mb, 0.5Gb   
 BufferGrowthFactor  float > 1             **1.05**, 1.01, 1.5, 2 
 FlushStepsCount     integer > 1           **1** 5, 1000, 50000 
=================== ===================== ==============================


Only file transport types are supported. Optional parameters for `IO::AddTransport` or in runtime config file transport field: 

**Transport type: File**

============= ================= ================================================
 **Key**       **Value Format**  **Default** and Examples 
============= ================= ================================================
 Library           string        **POSIX** (UNIX), **FStream** (Windows), stdio  
============= ================= ================================================

   