#################
 GPU-aware I/O
#################

The ``Put`` and ``Get`` functions in the BP4 and BP5 engines can receive user buffers allocated on the host or the device in both Sync and Deferred modes.

.. note::
    Currently only CUDA and HIP allocated buffers are supported for device data.

If ADIOS2 is built without GPU support, only buffers allocated on the host are supported. If ADIOS2 is built with any GPU support, by default, the library will automatically detect where does the buffer memory physically resides.

Users can also provide information about where the buffer was allocated by using the ``SetMemorySpace`` function within each variable.

.. code-block:: c++

    enum class MemorySpace
    {
        Detect, ///< Detect the memory space automatically
        Host,   ///< Host memory space (default)
        GPU     ///< GPU memory spaces
    };

ADIOS2 can use a CUDA or Kokkos backend for enabling GPU support. Only one backend can be active at a given time based on how ADIOS2 is build.

**********************************
Building ADIOS2 with a GPU backend
**********************************


Building with CUDA enabled
--------------------------

If there is no CUDA toolkit installed, cmake will turn CUDA off automatically. ADIOS2 default behavior for ``ADIOS2_USE_CUDA`` is to enable CUDA if it can find a CUDA toolkit on the system. In case the system has a CUDA toolkit installed, but it is desired to build ADIOS2 without CUDA enabled ``-DADIOS2_USE_CUDA=OFF`` must be used.

When building ADIOS2 with CUDA enabled, the user is responsible with setting the correct ``CMAKE_CUDA_ARCHITECTURES`` (e.g. for Summit the ``CMAKE_CUDA_ARCHITECTURES`` needs to be set to 70 to match the NVIDIA Volta V100).

Building with Kokkos enabled
--------------------------

The Kokkos library can be used to enable GPU within ADIOS2. Based on how Kokkos is build, either the CUDA or HIP backend will be enabled. Building with Kokkos requires ``-DADIOS2_USE_Kokkos=ON``. The user is responsible to set the ``CMAKE_CUDA_ARCHITECTURES`` to the same architecture used when configuring the Kokkos library it links against.

.. note::
    Kokkos version >= 3.7 is required to enable the GPU backend in ADIOS2


*************
Writing GPU code
*************

The following is a simple example of writing data to storage directly from a GPU buffer allocated with CUDA relying on the automatic detection of device pointers in ADIOS2. The ADIOS2 API is identical to codes using Host buffers for both the read and write logic.

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


If the ``SetMemorySpace`` function is used, the ADIOS2 library will not detect automatically where the buffer was allocated and will use the information provided by the user for all subsequent Puts or Gets. Example:

.. code-block:: c++

    variable.SetMemorySpace(adios2::MemorySpace::CUDA);
    for (size_t step = 0; step < nSteps; ++step)
    {
        bpWriter.BeginStep();
        bpWriter.Put(data, gpuSimData, adios2::Mode::Deferred); // or Sync
        bpWriter.EndStep();
    }

Underneath, ADIOS2 uses the backend used at build time to transfer the data. If ADIOS2 was build with CUDA, only CUDA buffers can be provided. If ADIOS2 was build with Kokkos (with CUDA enabled) only CUDA buffers can be provided. If ADIOS2 was build with Kokkos (with HIP enabled) only HIP buffers can be provided.

Using Kokkos buffers
--------------------------

ADIOS2 supports GPU buffers provided in the form of ``Kokkos::View`` directly in the Put/Get calls. The memory space can be automatically detected or provided by the user, in the same way as in the CUDA example.

.. code-block:: c++

   Kokkos::View<float *, Kokkos::CudaSpace> gpuSimData("data", N);
   bpWriter.Put(data, gpuSimData);

If the CUDA backend is being used (and not Kokkos) to enable GPU support in ADIOS2, Kokkos applications can still directly pass ``Kokkos::View`` as long as the correct external header is included: ``#include <adios2/cxx11/KokkosView.h>``.
