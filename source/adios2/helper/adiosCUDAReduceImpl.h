/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosCUDAMinMaxImpl.h
 *
 *  Created on: Jan 25, 2022
 *      Author: Jieyang Chen chenj3@ornl.gov
 */

/* Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ADIOS2_HELPER_ADIOSCUDA_REDUCE_IMPL_H_
#define ADIOS2_HELPER_ADIOSCUDA_REDUCE_IMPL_H_

#define checkCudaErrors(ans)                                                   \
    {                                                                          \
        gpuAssert((ans), __FILE__, __LINE__);                                  \
    }

inline void gpuAssert(cudaError_t code, const char *file, int line,
                      bool abort = true)
{
    if (code != cudaSuccess)
    {
        fprintf(stderr, "GPUassert: %s %s %d\n", cudaGetErrorString(code), file,
                line);
        if (abort)
            exit(code);
    }
}

namespace
{

struct MaxOp
{
    template <typename T>
    __host__ __device__ __forceinline__ T operator()(const T &a,
                                                     const T &b) const
    {
        return b > a ? b : a;
    }
};

struct MinOp
{
    template <typename T>
    __host__ __device__ __forceinline__ T operator()(const T &a,
                                                     const T &b) const
    {
        return b < a ? b : a;
    }
};

// Utility class used to avoid linker errors with extern
// unsized shared memory arrays with templated type
template <class T>
struct SharedMemory
{
    __device__ inline operator T *()
    {
        extern __shared__ int __smem[];
        return (T *)__smem;
    }

    __device__ inline operator const T *() const
    {
        extern __shared__ int __smem[];
        return (T *)__smem;
    }
};

// specialize for double to avoid unaligned memory
// access compile errors
template <>
struct SharedMemory<double>
{
    __device__ inline operator double *()
    {
        extern __shared__ double __smem_d[];
        return (double *)__smem_d;
    }

    __device__ inline operator const double *() const
    {
        extern __shared__ double __smem_d[];
        return (double *)__smem_d;
    }
};

extern "C" bool isPow2(unsigned int x) { return ((x & (x - 1)) == 0); }

template <class T, class Op>
__device__ __forceinline__ T warpReduceSum(unsigned int mask, T mySum, Op op)
{

    if constexpr (std::is_same<T, char>::value)
    {
        for (int offset = warpSize / 2; offset > 0; offset /= 2)
        {
            mySum = (char)op((int)mySum,
                             __shfl_down_sync(mask, (int)mySum, offset));
        }
    }
    else if constexpr (std::is_same<T, int8_t>::value)
    {
        for (int offset = warpSize / 2; offset > 0; offset /= 2)
        {
            mySum = (int8_t)op((int)mySum,
                               __shfl_down_sync(mask, (int)mySum, offset));
        }
    }
    else if constexpr (std::is_same<T, int16_t>::value)
    {
        for (int offset = warpSize / 2; offset > 0; offset /= 2)
        {
            mySum = (int16_t)op((int)mySum,
                                __shfl_down_sync(mask, (int)mySum, offset));
        }
    }
    else if constexpr (std::is_same<T, uint8_t>::value)
    {
        for (int offset = warpSize / 2; offset > 0; offset /= 2)
        {
            mySum = (uint8_t)op(
                (unsigned int)mySum,
                __shfl_down_sync(mask, (unsigned int)mySum, offset));
        }
    }
    else if constexpr (std::is_same<T, uint16_t>::value)
    {
        for (int offset = warpSize / 2; offset > 0; offset /= 2)
        {
            mySum = (uint16_t)op(
                (unsigned int)mySum,
                __shfl_down_sync(mask, (unsigned int)mySum, offset));
        }
    }
    else
    {
        for (int offset = warpSize / 2; offset > 0; offset /= 2)
        {
            mySum = op(mySum, __shfl_down_sync(mask, mySum, offset));
        }
    }
    return mySum;
}

unsigned int nextPow2(unsigned int x)
{
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return ++x;
}

template <typename T, unsigned int blockSize, bool nIsPow2, class Op>
__global__ void reduce(const T *__restrict__ g_idata, T *__restrict__ g_odata,
                       const size_t n)
{
    T *sdata = SharedMemory<T>();
    Op op;
    // perform first level of reduction,
    // reading from global memory, writing to shared memory
    unsigned int tid = threadIdx.x;
    unsigned int gridSize = blockSize * gridDim.x;
    unsigned int maskLength = (blockSize & 31); // 31 = warpSize-1
    maskLength = (maskLength > 0) ? (32 - maskLength) : maskLength;
    const unsigned int mask = (0xffffffff) >> maskLength;

    T mySum = g_idata[0];

    // we reduce multiple elements per thread.  The number is determined by the
    // number of active thread blocks (via gridDim).  More blocks will result
    // in a larger gridSize and therefore fewer elements per thread
    if (nIsPow2)
    {
        unsigned int i = blockIdx.x * blockSize * 2 + threadIdx.x;
        gridSize = gridSize << 1;

        while (i < n)
        {
            mySum = op(mySum, g_idata[i]);
            // ensure we don't read out of bounds -- this is optimized away for
            // powerOf2 sized arrays
            if ((i + blockSize) < n)
            {
                mySum = op(mySum, g_idata[i + blockSize]);
            }
            i += gridSize;
        }
    }
    else
    {
        unsigned int i = blockIdx.x * blockSize + threadIdx.x;
        while (i < n)
        {
            mySum = op(mySum, g_idata[i]);
            i += gridSize;
        }
    }

    mySum = warpReduceSum<T, Op>(0xffffffff, mySum, op);

    // each thread puts its local sum into shared memory
    if ((tid % warpSize) == 0)
    {
        sdata[tid / warpSize] = mySum;
    }

    __syncthreads();

    const unsigned int shmem_extent =
        (blockSize / warpSize) > 0 ? (blockSize / warpSize) : 1;
    const unsigned int ballot_result = __ballot_sync(mask, tid < shmem_extent);
    if (tid < shmem_extent)
    {
        mySum = sdata[tid];
    }

    mySum = warpReduceSum<T, Op>(0xffffffff, mySum, op);

    // write result for this block to global mem
    if (tid == 0)
    {
        g_odata[blockIdx.x] = mySum;
    }
}

template <class T, class Op>
void reduce_level(const size_t size, int threads, int blocks, const T *d_idata,
                  T *d_odata)
{
    dim3 dimBlock(threads, 1, 1);
    dim3 dimGrid(blocks, 1, 1);
    int smemSize = ((threads / 32) + 1) * sizeof(T);
    if (isPow2(size))
    {
        switch (threads)
        {
        case 1024:
            reduce<T, 1024, true, Op>
                <<<dimGrid, dimBlock, smemSize>>>(d_idata, d_odata, size);
            break;
        case 512:
            reduce<T, 512, true, Op>
                <<<dimGrid, dimBlock, smemSize>>>(d_idata, d_odata, size);
            break;

        case 256:
            reduce<T, 256, true, Op>
                <<<dimGrid, dimBlock, smemSize>>>(d_idata, d_odata, size);
            break;

        case 128:
            reduce<T, 128, true, Op>
                <<<dimGrid, dimBlock, smemSize>>>(d_idata, d_odata, size);
            break;

        case 64:
            reduce<T, 64, true, Op>
                <<<dimGrid, dimBlock, smemSize>>>(d_idata, d_odata, size);
            break;

        case 32:
            reduce<T, 32, true, Op>
                <<<dimGrid, dimBlock, smemSize>>>(d_idata, d_odata, size);
            break;

        case 16:
            reduce<T, 16, true, Op>
                <<<dimGrid, dimBlock, smemSize>>>(d_idata, d_odata, size);
            break;

        case 8:
            reduce<T, 8, true, Op>
                <<<dimGrid, dimBlock, smemSize>>>(d_idata, d_odata, size);
            break;

        case 4:
            reduce<T, 4, true, Op>
                <<<dimGrid, dimBlock, smemSize>>>(d_idata, d_odata, size);
            break;

        case 2:
            reduce<T, 2, true, Op>
                <<<dimGrid, dimBlock, smemSize>>>(d_idata, d_odata, size);
            break;

        case 1:
            reduce<T, 1, true, Op>
                <<<dimGrid, dimBlock, smemSize>>>(d_idata, d_odata, size);
            break;
        }
    }
    else
    {
        switch (threads)
        {
        case 1024:
            reduce<T, 1024, true, Op>
                <<<dimGrid, dimBlock, smemSize>>>(d_idata, d_odata, size);
            break;
        case 512:
            reduce<T, 512, false, Op>
                <<<dimGrid, dimBlock, smemSize>>>(d_idata, d_odata, size);
            break;

        case 256:
            reduce<T, 256, false, Op>
                <<<dimGrid, dimBlock, smemSize>>>(d_idata, d_odata, size);
            break;

        case 128:
            reduce<T, 128, false, Op>
                <<<dimGrid, dimBlock, smemSize>>>(d_idata, d_odata, size);
            break;

        case 64:
            reduce<T, 64, false, Op>
                <<<dimGrid, dimBlock, smemSize>>>(d_idata, d_odata, size);
            break;

        case 32:
            reduce<T, 32, false, Op>
                <<<dimGrid, dimBlock, smemSize>>>(d_idata, d_odata, size);
            break;

        case 16:
            reduce<T, 16, false, Op>
                <<<dimGrid, dimBlock, smemSize>>>(d_idata, d_odata, size);
            break;

        case 8:
            reduce<T, 8, false, Op>
                <<<dimGrid, dimBlock, smemSize>>>(d_idata, d_odata, size);
            break;

        case 4:
            reduce<T, 4, false, Op>
                <<<dimGrid, dimBlock, smemSize>>>(d_idata, d_odata, size);
            break;

        case 2:
            reduce<T, 2, false, Op>
                <<<dimGrid, dimBlock, smemSize>>>(d_idata, d_odata, size);
            break;

        case 1:
            reduce<T, 1, false, Op>
                <<<dimGrid, dimBlock, smemSize>>>(d_idata, d_odata, size);
            break;
        }
    }
}

void getNumBlocksAndThreads(const size_t n, int maxBlocks, int maxThreads,
                            int &blocks, int &threads)
{
    // get device capability, to avoid block/grid size exceed the upper bound
    cudaDeviceProp prop;
    int device;
    checkCudaErrors(cudaGetDevice(&device));
    checkCudaErrors(cudaGetDeviceProperties(&prop, device));

    threads = (n < maxThreads * 2) ? nextPow2((n + 1) / 2) : maxThreads;
    blocks = (n + (threads * 2 - 1)) / (threads * 2);

    if ((float)threads * blocks >
        (float)prop.maxGridSize[0] * prop.maxThreadsPerBlock)
    {
        printf("n is too large, please choose a smaller number!\n");
    }

    if (blocks > prop.maxGridSize[0])
    {
        printf("Grid size <%d> exceeds the device capability <%d>, set block "
               "size as "
               "%d (original %d)\n",
               blocks, prop.maxGridSize[0], threads * 2, threads);

        blocks /= 2;
        threads *= 2;
    }

    blocks = std::min(maxBlocks, blocks);
}

template <class T, class Op>
T reduce(const size_t size, int maxThreads, int maxBlocks,
         int cpuFinalThreshold, const T *d_idata)
{

    Op op;
    T gpu_result = 0;
    int numBlocks = 0;
    int numThreads = 0;
    bool needReadBack = true;

    T *d_odata = NULL;
    getNumBlocksAndThreads(size, maxBlocks, maxThreads, numBlocks, numThreads);
    checkCudaErrors(cudaMalloc((void **)&d_odata, numBlocks * sizeof(T)));

    T *h_odata = (T *)malloc(numBlocks * sizeof(T));

    T *d_intermediateSums;
    checkCudaErrors(
        cudaMalloc((void **)&d_intermediateSums, sizeof(T) * numBlocks));

    reduce_level<T, Op>(size, numThreads, numBlocks, d_idata, d_odata);

    int s = numBlocks;
    while (s > cpuFinalThreshold)
    {
        int threads = 0, blocks = 0;
        getNumBlocksAndThreads(s, maxBlocks, maxThreads, blocks, threads);
        checkCudaErrors(cudaMemcpy(d_intermediateSums, d_odata, s * sizeof(T),
                                   cudaMemcpyDeviceToDevice));
        reduce_level<T, Op>(s, threads, blocks, d_intermediateSums, d_odata);
        s = (s + (threads * 2 - 1)) / (threads * 2);
    }
    if (s > 1)
    {
        // copy result from device to host
        checkCudaErrors(cudaMemcpy(h_odata, d_odata, s * sizeof(T),
                                   cudaMemcpyDeviceToHost));
        for (int i = 0; i < s; i++)
        {
            gpu_result = op(gpu_result, h_odata[i]);
        }
        needReadBack = false;
    }

    if (needReadBack)
    {
        // copy final sum from device to host
        checkCudaErrors(cudaMemcpy(&gpu_result, d_odata, sizeof(T),
                                   cudaMemcpyDeviceToHost));
    }

    free(h_odata);
    checkCudaErrors(cudaFree(d_odata));

    return gpu_result;
}
}
#endif