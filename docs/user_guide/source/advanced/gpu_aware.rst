#################
 GPU-aware I/O
#################

The ``Put`` and ``Get`` functions in the BP4 and BP5 engines can receive user buffers allocated on the host or the device in both Sync and Deferred modes.

.. note::
Currently only CUDA allocated buffers are supported for device data.

If ADIOS2 is build without CUDA support, only buffers allocated on the host are supported. If ADIOS2 is build with CUDA support, by default, the library will automatically detect where does the buffer memory physically resides.

Users can also provide information about where the buffer was allocated by using the ``SetMemorySpace`` function within each variable.

     .. code-block:: c++

        enum class MemorySpace
        {
            Detect, ///< Detect the memory space automatically
            Host,   ///< Host memory space (default)
            CUDA    ///< CUDA memory spaces
        };


Building with CUDA enabled
------------

If there is no CUDA toolkit installed, cmake will turn CUDA off automatically. ADIOS2 default behavior for ``ADIOS2_USE_CUDA`` is to enable CUDA if it can find a CUDA toolkit on the system. In case the system has a CUDA toolkit installed, but it is desired to build ADIOS2 without CUDA enabled ``-DADIOS2_USE_CUDA=OFF`` must be used.

When building ADIOS2 with CUDA enabled, the ``CMAKE_CUDA_ARCHITECTURES`` is set by default to 70 to match the NVIDIA Volta V100. For any other architecture, the user is responsible with setting the correct ``CMAKE_CUDA_ARCHITECTURES``.


Using CUDA buffers
------------

The following is a simple example of writing data to storage directly from a GPU buffer allocated with CUDA.

     .. code-block:: c++

    float *gpuSimData;
    cudaMalloc(&gpuSimData, N * sizeof(float));
    cudaMemset(gpuSimData, 0, N);
    auto data = io.DefineVariable<float>("data", shape, start, count);

    io.SetEngine("BP5"); // or BPFile
    adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);
    // Simulation steps
    for (size_t step = 0; step < nSteps; ++step)
    {
        bpWriter.BeginStep();
        bpWriter.Put(data, gpuSimData, adios2::Mode::Deferred); // or Sync
        bpWriter.EndStep();
    }


The API is unchanged compared to using Host buffers for both the read and write logic.

If the ``SetMemorySpace`` function is used, the ADIOS2 library will not detect automatically where the buffer was allocated and will use the information provided by the user for all subsequent Puts or Gets. Example:

     .. code-block:: c++

        variable.SetMemorySpace(adios2::MemorySpace::CUDA);
        for (size_t step = 0; step < nSteps; ++step)
        {
            bpWriter.BeginStep();
            bpWriter.Put(data, gpuSimData, adios2::Mode::Deferred); // or Sync
            bpWriter.EndStep();
        }
