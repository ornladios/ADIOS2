#ifndef ADIOS2_DERIVED_Function_CPP_
#define ADIOS2_DERIVED_Function_CPP_

#include "Function.h"
#include "adios2/helper/adiosFunctions.h"
#include "adios2/helper/adiosLog.h"
#include <adios2-perfstubs-interface.h>
#include <cmath>

namespace adios2
{
namespace detail
{
template <class T, class Iterator>
T *ApplyOneToOne(Iterator inputBegin, Iterator inputEnd, size_t dataSize,
                 std::function<T(T, T)> compFct)
{
    T *outValues = (T *)malloc(dataSize * sizeof(T));
    if (outValues == nullptr)
    {
        helper::Throw<std::invalid_argument>("Derived", "Function", "ApplyOneToOne",
                                             "Error allocating memory for the derived variable");
    }
    memset(outValues, 0, dataSize * sizeof(T));
    for (Iterator variable = inputBegin; variable != inputEnd; ++variable)
    {
        for (size_t i = 0; i < dataSize; i++)
        {
            T data = *(reinterpret_cast<T *>((*variable).Data) + i);
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
    for (int i = 0; i < (int)dims[0]; ++i)
    {
        size_t prev_i = std::max(0, i - 1), next_i = std::min((int)dims[0] - 1, i + 1);
        for (int j = 0; j < (int)dims[1]; ++j)
        {
            size_t prev_j = std::max(0, j - 1), next_j = std::min((int)dims[1] - 1, j + 1);
            for (int k = 0; k < (int)dims[2]; ++k)
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
}

namespace derived
{
DerivedData AddFunc(std::vector<DerivedData> inputData, DataType type)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::AddFunc");
    size_t dataSize = std::accumulate(std::begin(inputData[0].Count), std::end(inputData[0].Count),
                                      1, std::multiplies<size_t>());

#define declare_type_add(T)                                                                        \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        T *addValues = detail::ApplyOneToOne<T>(inputData.begin(), inputData.end(), dataSize,         \
                                             [](T a, T b) { return a + b; });                      \
        return DerivedData({(void *)addValues, inputData[0].Start, inputData[0].Count});           \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_add)
    helper::Throw<std::invalid_argument>("Derived", "Function", "AddFunc",
                                         "Invalid variable types");
    return DerivedData();
}

DerivedData SubtractFunc(std::vector<DerivedData> inputData, DataType type)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::AddFunc");
    size_t dataSize = std::accumulate(std::begin(inputData[0].Count), std::end(inputData[0].Count),
                                      1, std::multiplies<size_t>());

#define declare_type_subtract(T)                                                                   \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        T *subtractValues = detail::ApplyOneToOne<T>(inputData.begin() + 1, inputData.end(),          \
                                                  dataSize, [](T a, T b) { return a + b; });       \
        for (size_t i = 0; i < dataSize; i++)                                                      \
            subtractValues[i] =                                                                    \
                *(reinterpret_cast<T *>(inputData[0].Data) + i) - subtractValues[i];               \
        return DerivedData({(void *)subtractValues, inputData[0].Start, inputData[0].Count});      \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_subtract)
    helper::Throw<std::invalid_argument>("Derived", "Function", "SubtractFunc",
                                         "Invalid variable types");
    return DerivedData();
}

DerivedData MagnitudeFunc(std::vector<DerivedData> inputData, DataType type)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::MagnitudeFunc");
    size_t dataSize = std::accumulate(std::begin(inputData[0].Count), std::end(inputData[0].Count),
                                      1, std::multiplies<size_t>());
#define declare_type_mag(T)                                                                        \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        T *magValues = detail::ApplyOneToOne<T>(inputData.begin(), inputData.end(), dataSize,         \
                                             [](T a, T b) { return a + b * b; });                  \
        for (size_t i = 0; i < dataSize; i++)                                                      \
        {                                                                                          \
            magValues[i] = (T)std::sqrt(magValues[i]);                                             \
        }                                                                                          \
        return DerivedData({(void *)magValues, inputData[0].Start, inputData[0].Count});           \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_mag)
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
DerivedData Curl3DFunc(const std::vector<DerivedData> inputData, DataType type)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::Curl3DFunc");
    size_t dims[3] = {inputData[0].Count[0], inputData[0].Count[1], inputData[0].Count[2]};

    DerivedData curl;
    curl.Start = inputData[0].Start;
    curl.Start.push_back(0);
    curl.Count = inputData[0].Count;
    curl.Count.push_back(3);

#define declare_type_curl(T)                                                                       \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        T *input1 = (T *)inputData[0].Data;                                                        \
        T *input2 = (T *)inputData[1].Data;                                                        \
        T *input3 = (T *)inputData[2].Data;                                                        \
        curl.Data = detail::ApplyCurl(input1, input2, input3, dims);                               \
        return curl;                                                                               \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_curl)
    helper::Throw<std::invalid_argument>("Derived", "Function", "Curl3DFunc",
                                         "Invalid variable types");
    return DerivedData();
}

std::vector<DerivedData> ExtractDimensionN(DerivedData inputData, DataType type, size_t dim)
{
    size_t num_data_sets = inputData.Count[dim];
    size_t num_chunks = 1;
    size_t chunk_length = 1;
    for (size_t i = 0; i < dim; ++i)
    {
        num_chunks *= inputData.Count[i];
    }
    for (size_t i = dim + 1; i < inputData.Count.size(); ++i)
    {
        chunk_length *= inputData.Count[i];
    }

    Dims set_Start;
    Dims set_Count;
    for (size_t i = 0; i < inputData.Start.size(); ++i)
    {
        if (i != dim)
        {
            set_Start.push_back(inputData.Start[i]);
            set_Count.push_back(inputData.Count[i]);
        }
    }

    std::vector<DerivedData> result;
    size_t chunk_size = chunk_length * helper::GetDataTypeSize(type);
    // TO DO - FREE
    for (size_t i = 0; i < num_data_sets; ++i)
        result.push_back({malloc(num_chunks * chunk_size), set_Start, set_Count});

    // How does Start factor in?
    //  size_t data_iter = 0;
    char *input_ptr = (char *)inputData.Data;
    for (size_t chunk = 0; chunk < num_chunks; ++chunk)
    {
        for (size_t data_set = 0; data_set < num_data_sets; ++data_set)
        {
            char *result_ptr = (char *)(result[data_set].Data);
            memcpy(result_ptr + (chunk * chunk_size),
                   input_ptr + (((num_data_sets * chunk) + data_set) * chunk_size), chunk_size);

            // memcpy(&(result[data_set].Data[chunk * chunk_length]), &(inputData.Data[data_iter]),
            /*
          for (size_t chunk_iter = 0; chunk_iter < chunk_length; ++chunk_iter)
            {
              result[data_set].Data[(chunk * chunk_length) + chunk_iter] =
          inputData.Data[data_iter++];
              }*/
        }
    }

    return result;
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

}
} // namespace adios2
#endif
