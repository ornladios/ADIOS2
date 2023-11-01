/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <algorithm>
#include <ios>
#include <iostream>
#include <vector>

#include <adios2.h>

#include <hip/hip_runtime.h>

__global__ void hip_initialize(float *vec) { vec[hipBlockIdx_x] = hipBlockIdx_x; }

__global__ void hip_increment(float *vec, float val) { vec[hipBlockIdx_x] += val; }

int BPWrite(const std::string fname, const size_t N, int nSteps, const std::string engine)
{
    hipError_t hipExit;
    float *gpuSimData;
    hipExit = hipMalloc((void **)&gpuSimData, N * sizeof(float));
    if (hipExit != hipSuccess)
    {
        std::cout << "[BPWrite] error: " << hipGetErrorString(hipExit) << std::endl;
        return 1;
    }
    hipLaunchKernelGGL(hip_initialize, dim3(N), dim3(1), 0, 0, gpuSimData);
    hipExit = hipDeviceSynchronize();
    if (hipExit != hipSuccess)
    {
        std::cout << "[BPWrite] error: " << hipGetErrorString(hipExit) << std::endl;
        return 1;
    }

    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("WriteIO");
    io.SetEngine(engine);

    const adios2::Dims shape{static_cast<size_t>(N)};
    const adios2::Dims start{static_cast<size_t>(0)};
    const adios2::Dims count{N};
    auto data = io.DefineVariable<float>("data", shape, start, count);

    adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);

    for (size_t step = 0; step < nSteps; ++step)
    {
        adios2::Box<adios2::Dims> sel({0}, {N});
        data.SetSelection(sel);

        bpWriter.BeginStep();
        bpWriter.Put(data, gpuSimData);
        bpWriter.EndStep();

        hipLaunchKernelGGL(hip_increment, dim3(N), dim3(1), 0, 0, gpuSimData, 10);
        hipExit = hipDeviceSynchronize();
        if (hipExit != hipSuccess)
        {
            std::cout << "[BPWrite] error: " << hipGetErrorString(hipExit) << std::endl;
            return 1;
        }
    }

    bpWriter.Close();
    return 0;
}

int BPRead(const std::string fname, const size_t N, int nSteps, const std::string engine)
{
    hipError_t hipExit;
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("ReadIO");
    io.SetEngine(engine);

    adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);

    unsigned int step = 0;
    float *gpuSimData;
    hipExit = hipMalloc((void **)&gpuSimData, N * sizeof(float));
    if (hipExit != hipSuccess)
    {
        std::cout << "[BPWrite] error: " << hipGetErrorString(hipExit) << std::endl;
        return 1;
    }
    for (; bpReader.BeginStep() == adios2::StepStatus::OK; ++step)
    {
        auto data = io.InquireVariable<float>("data");
        const adios2::Dims start{0};
        const adios2::Dims count{N};
        const adios2::Box<adios2::Dims> sel(start, count);
        data.SetSelection(sel);

        bpReader.Get(data, gpuSimData);
        bpReader.EndStep();

        std::vector<float> cpuData(N);
        hipExit = hipMemcpy(cpuData.data(), gpuSimData, N * sizeof(float), hipMemcpyDeviceToHost);
        if (hipExit != hipSuccess)
        {
            std::cout << "[BPWrite] error: " << hipGetErrorString(hipExit) << std::endl;
            return 1;
        }
        std::cout << "Simualation step " << step << " : ";
        std::cout << cpuData.size() << " elements: " << cpuData[0];
        std::cout << " " << cpuData[1] << " ... ";
        std::cout << cpuData[cpuData.size() - 1] << std::endl;
    }
    bpReader.Close();
    return 0;
}

int main(int argc, char **argv)
{
    hipError_t hipExit;
    const int device_id = 0;
    hipExit = hipSetDevice(device_id);
    if (hipExit != hipSuccess)
    {
        std::cout << "[BPWrite] error: " << hipGetErrorString(hipExit) << std::endl;
        return 1;
    }
    const std::vector<std::string> list_of_engines = {"BPFile"};
    const size_t N = 6000;
    int nSteps = 2, ret = 0;

    for (auto engine : list_of_engines)
    {
        std::cout << "Using engine " << engine << std::endl;
        const std::string fname(engine + "_HIP_WR.bp");
        ret += BPWrite(fname, N, nSteps, engine);
        ret += BPRead(fname, N, nSteps, engine);
    }
    return ret;
}
