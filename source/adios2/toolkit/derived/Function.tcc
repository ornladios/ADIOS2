#ifndef ADIOS2_DERIVED_Function_TCC_
#define ADIOS2_DERIVED_Function_TCC_

#include "Function.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <numeric>

namespace adios2
{
namespace derived
{

template <class T>
T *ApplyOneToOne(std::vector<DerivedData> inputData, size_t dataSize,
                 std::function<T(T, T)> compFct)
{
    T *outValues = (T *)malloc(dataSize * sizeof(T));
    if (outValues == nullptr)
    {
        std::cout << "Allocation failed for the derived data" << std::endl;
        // TODO - throw an exception
    }
    memset((void *)outValues, 0, dataSize * sizeof(T));
    for (auto &variable : inputData)
    {
        for (size_t i = 0; i < dataSize; i++)
        {
            T data = *(reinterpret_cast<T *>(variable.Data) + i);
            outValues[i] = compFct(outValues[i], data);
        }
    }
    return outValues;
}

inline size_t returnIndex(size_t x, size_t y, size_t z, const size_t dims[3])
{
    return z + y * dims[2] + x * dims[2] * dims[1];
}

template <class T>
T *ApplyCurl(const T *input1, const T *input2, const T *input3, const size_t dims[3])
{
    size_t dataSize = dims[0] * dims[1] * dims[2];
    T *data = (T *)malloc(dataSize * sizeof(float) * 3);
    size_t index = 0;
    for (int i = 0; i < dims[0]; ++i)
    {
        size_t prev_i = std::max(0, i - 1), next_i = std::min((int)dims[0] - 1, i + 1);
        for (int j = 0; j < dims[1]; ++j)
        {
            size_t prev_j = std::max(0, j - 1), next_j = std::min((int)dims[1] - 1, j + 1);
            for (int k = 0; k < dims[2]; ++k)
            {
                size_t prev_k = std::max(0, k - 1), next_k = std::min((int)dims[2] - 1, k + 1);
                // curl[0] = dv3 / dy - dv2 / dz
                data[3 * index] = (input3[returnIndex(i, next_j, k, dims)] -
                                   input3[returnIndex(i, prev_j, k, dims)]) /
                                  (next_j - prev_j);
                data[3 * index] += (input2[returnIndex(i, j, prev_k, dims)] -
                                    input2[returnIndex(i, j, next_k, dims)]) /
                                   (next_k - prev_k);
                // curl[1] = dv1 / dz - dv3 / dx
                data[3 * index + 1] = (input1[returnIndex(i, j, next_k, dims)] -
                                       input1[returnIndex(i, j, prev_k, dims)]) /
                                      (next_k - prev_k);
                data[3 * index + 1] += (input3[returnIndex(prev_i, j, k, dims)] -
                                        input3[returnIndex(next_i, j, k, dims)]) /
                                       (next_i - prev_i);
                // curl[2] = dv2 / dx - dv1 / dy
                data[3 * index + 2] = (input2[returnIndex(next_i, j, k, dims)] -
                                       input2[returnIndex(prev_i, j, k, dims)]) /
                                      (next_i - prev_i);
                data[3 * index + 2] += (input1[returnIndex(i, prev_j, k, dims)] -
                                        input1[returnIndex(i, next_j, k, dims)]) /
                                       (next_j - prev_j);
                index++;
            }
        }
    }
    return data;
}

// types not supported for curl
std::complex<float> *ApplyCurl(const std::complex<float> * /*input 1*/,
                               const std::complex<float> * /*input 2*/,
                               const std::complex<float> * /*input 3*/, const size_t[3] /*dims*/)
{
    return NULL;
}

std::complex<double> *ApplyCurl(const std::complex<double> * /*input 1*/,
                                const std::complex<double> * /*input 2*/,
                                const std::complex<double> * /*input 3*/, const size_t[3] /*dims*/)
{
    return NULL;
}

}
}
#endif
