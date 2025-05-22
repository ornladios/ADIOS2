/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * bpStepsWriteReadCBindings.cu  Simple example of writing and reading data through ADIOS2 BP engine
 * with multiple simulations steps for every IO step using CUDA (using the C bindings)
 */

#include <adios2_c.h>
#include <cuda_runtime.h>
#include <stdio.h>

__global__ void update_array(float *vect, int val) { vect[blockIdx.x] += val; }

void writer(adios2_adios *adios, const char *fname, const size_t Nx, unsigned int nSteps)
{
    // Initialize the simulation data
    float *gpuSimData;
    cudaMalloc(&gpuSimData, Nx * sizeof(float));
    cudaMemset(gpuSimData, 0, Nx);

    // Set up the ADIOS structures
    adios2_io *bpIO = adios2_declare_io(adios, "WriteIO");

    size_t shape[1];
    size_t start[1];
    size_t count[1];
    shape[0] = Nx;
    start[0] = 0;
    count[0] = Nx;

    adios2_variable *bpFloats = adios2_define_variable(
        bpIO, "bpFloats", adios2_type_float, 1, shape, start, count, adios2_constant_dims_true);

    adios2_engine *bpWriter = adios2_open(bpIO, fname, adios2_mode_write);

    adios2_step_status err;
    for (unsigned int step = 0; step < nSteps; ++step)
    {
        adios2_begin_step(bpWriter, adios2_step_mode_append, 0.0f, &err);
        adios2_set_memory_space(bpFloats, adios2_memory_space_gpu);
        adios2_put(bpWriter, bpFloats, gpuSimData, adios2_mode_sync);
        adios2_end_step(bpWriter);

        // Update values in the simulation data
        update_array<<<Nx, 1>>>(gpuSimData, 10);
    }

    adios2_close(bpWriter);
    cudaFree(gpuSimData);
}

void reader(adios2_adios *adios, const char *fname, const size_t Nx, unsigned int nSteps)
{
    adios2_step_status status;

    adios2_io *bpIO = adios2_declare_io(adios, "ReadIO");

    adios2_engine *bpReader = adios2_open(bpIO, fname, adios2_mode_read);

    float *gpuSimData;
    cudaMalloc(&gpuSimData, Nx * sizeof(float));
    cudaMemset(gpuSimData, 0, Nx);

    while (adios2_begin_step(bpReader, adios2_step_mode_read, -1., &status) == adios2_error_none)
    {
        if (status == adios2_step_status_end_of_stream)
        {
            break;
        }

        adios2_variable *bpFloats = adios2_inquire_variable(bpIO, "bpFloats");
        size_t start[1];
        size_t count[1];
        start[0] = 0;
        count[0] = Nx;
        adios2_set_selection(bpFloats, 1, start, count);
        adios2_set_memory_space(bpFloats, adios2_memory_space_gpu);
        adios2_get(bpReader, bpFloats, gpuSimData, adios2_mode_sync);
        adios2_end_step(bpReader);
    }
    adios2_close(bpReader);
    cudaFree(gpuSimData);
}

int main(int argc, char **argv)
{
    const int device_id = 1;
    cudaSetDevice(device_id);

    const char filename[30] = "BPStepsWriteReadCBindings.bp";
    const unsigned int nSteps = 2;
    const unsigned int Nx = 3;

    adios2_adios *adios = adios2_init_serial();
    writer(adios, filename, Nx, nSteps);
    reader(adios, filename, Nx, nSteps);

    return 0;
}
