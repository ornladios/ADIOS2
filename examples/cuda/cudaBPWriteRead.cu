/*
 * Simple example of writing and reading data
 * through ADIOS2 BP engine with multiple simulations steps
 * for every IO step.
 */

#include <ios>
#include <iostream>
#include <string>
#include <vector>

#include <adios2.h>

#include <cuda_runtime.h>

__global__ void update_array(float *vect, int val) { vect[blockIdx.x] += val; }

std::string engine("BP5");

int BPWrite(const std::string fname, const size_t N, int nSteps,
            const std::string engine)
{
    // Initialize the simulation data
    float *gpuSimData;
    cudaMalloc(&gpuSimData, N * sizeof(float));
    cudaMemset(gpuSimData, 0, N);

    // Set up the ADIOS structures
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("WriteIO");
    io.SetEngine(engine);

    // Declare an array for the ADIOS data of size (NumOfProcesses * N)
    const adios2::Dims shape{static_cast<size_t>(N)};
    const adios2::Dims start{static_cast<size_t>(0)};
    const adios2::Dims count{N};
    auto data = io.DefineVariable<float>("data", shape, start, count);

    adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);

    // Simulation steps
    for (size_t step = 0; step < nSteps; ++step)
    {
        // Make a 1D selection to describe the local dimensions of the
        // variable we write and its offsets in the global spaces
        adios2::Box<adios2::Dims> sel({0}, {N});
        data.SetSelection(sel);

        // Start IO step every write step
        bpWriter.BeginStep();
        data.SetMemorySpace(adios2::MemorySpace::GPU);
        bpWriter.Put(data, gpuSimData);
        bpWriter.EndStep();

        // Update values in the simulation data
        update_array<<<N, 1>>>(gpuSimData, 10);
    }

    bpWriter.Close();
    return 0;
}

int BPRead(const std::string fname, const size_t N, int nSteps,
           const std::string engine)
{
    // Create ADIOS structures
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("ReadIO");
    io.SetEngine(engine);

    adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);

    unsigned int step = 0;
    float *gpuSimData;
    cudaMalloc(&gpuSimData, N * sizeof(float));
    cudaMemset(gpuSimData, 0, N);
    for (; bpReader.BeginStep() == adios2::StepStatus::OK; ++step)
    {
        auto data = io.InquireVariable<float>("data");
        std::vector<float> simData(N);
        const adios2::Dims start{0};
        const adios2::Dims count{N};
        const adios2::Box<adios2::Dims> sel(start, count);
        data.SetSelection(sel);

        data.SetMemorySpace(adios2::MemorySpace::GPU);
        bpReader.Get(data, gpuSimData); //, adios2::Mode::Deferred);
        bpReader.EndStep();
        cudaMemcpy(simData.data(), gpuSimData, N * sizeof(float),
                   cudaMemcpyDeviceToHost);
        std::cout << "Simualation step " << step << " : ";
        std::cout << simData.size() << " elements: " << simData[1] << std::endl;
    }
    bpReader.Close();
    return 0;
}

int main(int argc, char **argv)
{
    if (argv[1])
        engine = argv[1];
    std::cout << "Using engine " << engine << std::endl;

    const std::string fname("Cuda" + engine + "wr.bp");
    const int device_id = 1;
    cudaSetDevice(device_id);
    const size_t N = 6000;
    int nSteps = 10, ret = 0;

    ret += BPWrite(fname, N, nSteps, engine);
    ret += BPRead(fname, N, nSteps, engine);
    return ret;
}
