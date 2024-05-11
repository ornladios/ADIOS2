#ifndef ADIOS2_DERIVED_Function_CPP_
#define ADIOS2_DERIVED_Function_CPP_

#include "Function.h"
#include "Function.tcc"
#include "adios2/common/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h"
#include <cmath>

namespace adios2
{
namespace derived
{

DerivedData AddFunc(std::vector<DerivedData> inputData, DataType type)
{
    size_t dataSize = std::accumulate(std::begin(inputData[0].Count), std::end(inputData[0].Count),
                                      1, std::multiplies<size_t>());

#define declare_type_add(T)                                                                        \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        T *addValues = ApplyOneToOne<T>(inputData, dataSize, [](T a, T b) { return a + b; });      \
        return DerivedData({(void *)addValues, inputData[0].Start, inputData[0].Count});           \
    }
    ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_type_add)
    helper::Throw<std::invalid_argument>("Derived", "Function", "AddFunc",
                                         "Invalid variable types");
    return DerivedData();
}

DerivedData MagnitudeFunc(std::vector<DerivedData> inputData, DataType type)
{
    size_t dataSize = std::accumulate(std::begin(inputData[0].Count), std::end(inputData[0].Count),
                                      1, std::multiplies<size_t>());
#define declare_type_mag(T)                                                                        \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        T *magValues = ApplyOneToOne<T>(inputData, dataSize, [](T a, T b) { return a + b * b; });  \
        for (size_t i = 0; i < dataSize; i++)                                                      \
        {                                                                                          \
            magValues[i] = (T)std::sqrt(magValues[i]);                                             \
        }                                                                                          \
        return DerivedData({(void *)magValues, inputData[0].Start, inputData[0].Count});           \
    }
    ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_type_mag)
    helper::Throw<std::invalid_argument>("Derived", "Function", "MagnitudeFunc",
                                         "Invalid variable types");
    return DerivedData();
}

/*
 * Input: 3D vector field F(x,y,z)= {F1(x,y,z), F2(x,y,z), F3(x,y,z)}
 *
 *     inputData - (3) components of 3D vector field
 *
 * Computation:
 *     curl(F(x,y,z)) = (partial(F3,y) - partial(F2,z))i
 *                    + (partial(F1,z) - partial(F3,x))j
 *                    + (partial(F2,x) - partial(F1,y))k
 *
 *     boundaries are calculated only with data in block
 *         (ex: partial derivatives in x direction at point (0,0,0)
 *              only use data from (1,0,0), etc )
 */
inline size_t returnIndex(size_t x, size_t y, size_t z, size_t dims[3])
{
    return z + y * dims[2] + x * dims[2] * dims[1];
}

DerivedData Curl3DFunc(const std::vector<DerivedData> inputData, DataType type)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::Curl3DFunc");
    size_t dataSize = inputData[0].Count[0] * inputData[0].Count[1] * inputData[0].Count[2];
    size_t dims[3] = {inputData[0].Count[0], inputData[0].Count[1], inputData[0].Count[2]};

    DerivedData curl;
    // ToDo - template type
    float *data = (float *)malloc(dataSize * sizeof(float) * 3);
    curl.Start = inputData[0].Start;
    curl.Start.push_back(0);
    curl.Count = inputData[0].Count;
    curl.Count.push_back(3);

    float *input1 = (float *)inputData[0].Data;
    float *input2 = (float *)inputData[1].Data;
    float *input3 = (float *)inputData[2].Data;
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
    curl.Data = data;
    return curl;
}

Dims SameDimsFunc(std::vector<Dims> input)
{
    // check that all dimenstions are the same
    if (input.size() > 1)
    {
        Dims first_element = input[0];
        bool dim_are_equal = std::all_of(input.begin() + 1, input.end(),
                                         [&first_element](Dims x) { return x == first_element; });
        if (!dim_are_equal)
            helper::Throw<std::invalid_argument>("Derived", "Function", "SameDimsFunc",
                                                 "Invalid variable dimensions");
    }
    // return the first dimension
    return input[0];
}

// Input Dims are the same, output is combination of all inputs
Dims CurlDimsFunc(std::vector<Dims> input)
{
    // check that all dimenstions are the same
    if (input.size() > 1)
    {
        Dims first_element = input[0];
        bool dim_are_equal = std::all_of(input.begin() + 1, input.end(),
                                         [&first_element](Dims x) { return x == first_element; });
        if (!dim_are_equal)
            helper::Throw<std::invalid_argument>("Derived", "Function", "CurlDimsFunc",
                                                 "Invalid variable dimensions");
    }
    // return original dimensions with added dimension of number of inputs
    Dims output = input[0];
    output.push_back(input.size());
    return output;
}

#define declare_template_instantiation(T)                                                          \
    T *ApplyOneToOne(std::vector<DerivedData>, size_t, std::function<T(T, T)>);

ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

}
} // namespace adios2
#endif
