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
                 std::function<T(T, T)> compFct, T initVal = (T)0)
{
    T *outValues = (T *)malloc(dataSize * sizeof(T));
    if (outValues == nullptr)
    {
        helper::Throw<std::invalid_argument>("Derived", "Function", "ApplyOneToOne",
                                             "Error allocating memory for the derived variable");
    }

    // initialize the output buffer with the data from the first buffer
    for (size_t i = 0; i < dataSize; i++)
    {
        outValues[i] = initVal;
    }
    // apply the aggregation function on all other buffers
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
// Perform a reduce sum over all variables in the std::vector
DerivedData AddFunc(std::vector<DerivedData> inputData, DataType type)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::AddFunc");
    size_t dataSize = std::accumulate(std::begin(inputData[0].Count), std::end(inputData[0].Count),
                                      1, std::multiplies<size_t>());

#define declare_type_add(T)                                                                        \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        T *addValues = detail::ApplyOneToOne<T>(inputData.begin(), inputData.end(), dataSize,      \
                                                [](T a, T b) { return a + b; });                   \
        return DerivedData({(void *)addValues, inputData[0].Start, inputData[0].Count});           \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_add)
    helper::Throw<std::invalid_argument>("Derived", "Function", "AddFunc",
                                         "Invalid variable types");
    return DerivedData();
}

// Perform a subtraction from the first variable of all other variables in the std::vector
DerivedData SubtractFunc(std::vector<DerivedData> inputData, DataType type)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::SubtractFunc");
    size_t dataSize = std::accumulate(std::begin(inputData[0].Count), std::end(inputData[0].Count),
                                      1, std::multiplies<size_t>());

// Perform a reduce sum over all variables in the std::vector except the first one
// and remove this sum from the first buffer
#define declare_type_subtract(T)                                                                   \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        T *subtractValues = detail::ApplyOneToOne<T>(inputData.begin() + 1, inputData.end(),       \
                                                     dataSize, [](T a, T b) { return a + b; });    \
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

// Perform a reduce multiply over all variables in the std::vector
DerivedData MultFunc(std::vector<DerivedData> inputData, DataType type)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::MultFunc");
    size_t dataSize = std::accumulate(std::begin(inputData[0].Count), std::end(inputData[0].Count),
                                      1, std::multiplies<size_t>());

#define declare_type_mult(T)                                                                       \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        T *multValues = detail::ApplyOneToOne<T>(                                                  \
            inputData.begin(), inputData.end(), dataSize, [](T a, T b) { return a * b; }, 1);      \
        return DerivedData({(void *)multValues, inputData[0].Start, inputData[0].Count});          \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_mult)
    helper::Throw<std::invalid_argument>("Derived", "Function", "MultFunc",
                                         "Invalid variable types");
    return DerivedData();
}

// Perform a division from the first variable of all other variables in the std::vector
DerivedData DivFunc(std::vector<DerivedData> inputData, DataType type)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::DivFunc");
    size_t dataSize = std::accumulate(std::begin(inputData[0].Count), std::end(inputData[0].Count),
                                      1, std::multiplies<size_t>());

// Perform a reduce multiply over all variables in the std::vector except the first one
// and divide this value from the first buffer
#define declare_type_div(T)                                                                        \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        T *divValues = detail::ApplyOneToOne<T>(                                                   \
            inputData.begin() + 1, inputData.end(), dataSize, [](T a, T b) { return a * b; }, 1);  \
        for (size_t i = 0; i < dataSize; i++)                                                      \
            divValues[i] = *(reinterpret_cast<T *>(inputData[0].Data) + i) / divValues[i];         \
        return DerivedData({(void *)divValues, inputData[0].Start, inputData[0].Count});           \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_div)
    helper::Throw<std::invalid_argument>("Derived", "Function", "DivFunc",
                                         "Invalid variable types");
    return DerivedData();
}

// Apply Sqrt over all elements in the variable
DerivedData SqrtFunc(std::vector<DerivedData> inputData, DataType type)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::SqrtFunc");
    if (inputData.size() != 1)
    {
        helper::Throw<std::invalid_argument>("Derived", "Function", "SqrtFunc",
                                             "Invalid number of arguments passed to SqrtFunc");
    }
    size_t dataSize = std::accumulate(std::begin(inputData[0].Count), std::end(inputData[0].Count),
                                      1, std::multiplies<size_t>());
    DataType inputType = inputData[0].Type;

    if (inputType == DataType::LongDouble)
    {
        long double *sqrtValues = (long double *)malloc(dataSize * sizeof(long double));
        std::transform(reinterpret_cast<long double *>(inputData[0].Data),
                       reinterpret_cast<long double *>(inputData[0].Data) + dataSize, sqrtValues,
                       [](long double &a) { return std::sqrt(a); });
        return DerivedData({(void *)sqrtValues, inputData[0].Start, inputData[0].Count});
    }
#define declare_type_sqrt(T)                                                                       \
    if (inputType == helper::GetDataType<T>())                                                     \
    {                                                                                              \
        if (inputType != DataType::LongDouble)                                                     \
        {                                                                                          \
            double *sqrtValues = (double *)malloc(dataSize * sizeof(double));                      \
            std::transform(reinterpret_cast<T *>(inputData[0].Data),                               \
                           reinterpret_cast<T *>(inputData[0].Data) + dataSize, sqrtValues,        \
                           [](T &a) { return std::sqrt(a); });                                     \
            return DerivedData({(void *)sqrtValues, inputData[0].Start, inputData[0].Count});      \
        }                                                                                          \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_sqrt)
    helper::Throw<std::invalid_argument>("Derived", "Function", "SqrtFunc",
                                         "Invalid variable types");
    return DerivedData();
}

// Apply Pow over all elements in the variable
DerivedData PowFunc(std::vector<DerivedData> inputData, DataType type)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::PowFunc");
    if (inputData.size() != 1)
    {
        helper::Throw<std::invalid_argument>("Derived", "Function", "PowFunc",
                                             "Invalid number of arguments passed to PowFunc");
    }
    size_t dataSize = std::accumulate(std::begin(inputData[0].Count), std::end(inputData[0].Count),
                                      1, std::multiplies<size_t>());
    DataType inputType = inputData[0].Type;

    if (inputType == DataType::LongDouble)
    {
        long double *powValues = (long double *)malloc(dataSize * sizeof(long double));
        std::transform(reinterpret_cast<long double *>(inputData[0].Data),
                       reinterpret_cast<long double *>(inputData[0].Data) + dataSize, powValues,
                       [](long double &a) { return std::pow(a, 2); });
        return DerivedData({(void *)powValues, inputData[0].Start, inputData[0].Count});
    }
#define declare_type_pow(T)                                                                        \
    if (inputType == helper::GetDataType<T>())                                                     \
    {                                                                                              \
        if (inputType != DataType::LongDouble)                                                     \
        {                                                                                          \
            double *powValues = (double *)malloc(dataSize * sizeof(double));                       \
            std::transform(reinterpret_cast<T *>(inputData[0].Data),                               \
                           reinterpret_cast<T *>(inputData[0].Data) + dataSize, powValues,         \
                           [](T &a) { return std::pow(a, 2); });                                   \
            return DerivedData({(void *)powValues, inputData[0].Start, inputData[0].Count});       \
        }                                                                                          \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_pow)
    helper::Throw<std::invalid_argument>("Derived", "Function", "PowFunc",
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
        T *magValues = detail::ApplyOneToOne<T>(inputData.begin(), inputData.end(), dataSize,      \
                                                [](T a, T b) { return a + b * b; });               \
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

DataType SameTypeFunc(DataType input) { return input; }

DataType FloatTypeFunc(DataType input)
{
    if (input == DataType::LongDouble)
        return input;
    if ((input == DataType::FloatComplex) || (input == DataType::DoubleComplex))
        return input;
    return DataType::Double;
}
}
} // namespace adios2
#endif
