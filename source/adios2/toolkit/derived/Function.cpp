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

template <class T, class Op>
T ApplyExprOnConst(const std::vector<std::string> &vec, T init, Op op)
{
    T result = init;
    for (const auto &s : vec)
    {
        std::istringstream iss(s);
        T value;
        if (!(iss >> value) || !iss.eof())
        {
            throw std::invalid_argument("Invalid conversion from string to target type.");
        }
        result = op(result, value);
    }
    return result;
};

template <class T>
T *AggregateOnLastDim(T *data, size_t dataSize, size_t nVariables, std::function<T(T, T)> compFct)
{
    T *outValues = (T *)malloc(dataSize * sizeof(T));
    if (outValues == nullptr)
    {
        helper::Throw<std::invalid_argument>("Derived", "Function", "ApplyOneToOne",
                                             "Error allocating memory for the derived variable");
    }
    memset(outValues, 0, dataSize * sizeof(T));
    for (size_t i = 0; i < dataSize; i++)
    {
        size_t start = nVariables * i;
        for (size_t variable = 0; variable < nVariables; ++variable)
        {
            T dataElem = *(data + start + variable);
            outValues[i] = compFct(outValues[i], dataElem);
        }
    }
    return outValues;
}

inline size_t returnIndex(size_t x, size_t y, size_t z, const size_t dims[3])
{
    return z + y * dims[2] + x * dims[2] * dims[1];
}

template <class T>
T *ApplyCross3D(const T *Ax, const T *Ay, const T *Az, const T *Bx, const T *By, const T *Bz,
                const size_t dataSize)
{
    T *data = (T *)malloc(dataSize * sizeof(T) * 3);
    for (size_t i = 0; i < dataSize; ++i)
    {
        data[3 * i] = (Ay[i] * Bz[i]) - (Az[i] * By[i]);
        data[3 * i + 1] = (Ax[i] * Bz[i]) - (Az[i] * Bx[i]);
        data[3 * i + 2] = (Ax[i] * By[i]) - (Ay[i] * Bx[i]);
    }
    return data;
}

template <class T>
T *ApplyCurl(const T *input1, const T *input2, const T *input3, const size_t dims[3])
{
    size_t dataSize = dims[0] * dims[1] * dims[2];
    T *data = (T *)malloc(dataSize * sizeof(T) * 3);
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
DerivedData AddAggregatedFunc(DerivedData inputData, DataType type)
{
    // the aggregation is done over the last dimension so the total size is d1 * d2 * .. d_n-1
    size_t dataSize = std::accumulate(std::begin(inputData.Count), std::end(inputData.Count) - 1, 1,
                                      std::multiplies<size_t>());
#define declare_type_agradd(T)                                                                     \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        size_t numVar = inputData.Count.back();                                                    \
        T *addValues =                                                                             \
            detail::AggregateOnLastDim<T>(reinterpret_cast<T *>(inputData.Data), dataSize, numVar, \
                                          [](T a, T b) { return a + b; });                         \
        return DerivedData({(void *)addValues});                                                   \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_agradd)
    helper::Throw<std::invalid_argument>("Derived", "Function", "AddAggregateFunc",
                                         "Invalid variable types");
    return DerivedData();
}

DerivedData AddFunc(ExprData exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::AddFunc");
    auto inputData = exprData.Data;
    auto type = exprData.OutType;
    // if there is only one element return the aggregate result
    if (inputData.size() == 1 && exprData.Const.size() == 0)
        return AddAggregatedFunc(inputData[0], type);

    size_t dataSize = std::accumulate(std::begin(inputData[0].Count), std::end(inputData[0].Count),
                                      1, std::multiplies<size_t>());
    // apply the expression over the constants
    // and then apply the expression over the variables and constant
#define declare_type_add(T)                                                                        \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        T initVal = detail::ApplyExprOnConst<T, std::plus<T>>(exprData.Const, 0, std::plus<T>());  \
        T *addValues = detail::ApplyOneToOne<T>(                                                   \
            inputData.begin(), inputData.end(), dataSize, [](T a, T b) { return a + b; },          \
            initVal);                                                                              \
        return DerivedData({(void *)addValues});                                                   \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_add)
    helper::Throw<std::invalid_argument>("Derived", "Function", "AddFunc",
                                         "Invalid variable types");
    return DerivedData();
}

// Perform a subtraction from the first variable of all other variables in the std::vector
DerivedData SubtractFunc(ExprData exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::SubtractFunc");
    auto inputData = exprData.Data;
    auto type = exprData.OutType;
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
        return DerivedData({(void *)subtractValues});                                              \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_subtract)
    helper::Throw<std::invalid_argument>("Derived", "Function", "SubtractFunc",
                                         "Invalid variable types");
    return DerivedData();
}

// Perform a reduce multiply over all variables in the std::vector
DerivedData MultFunc(ExprData exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::MultFunc");
    auto inputData = exprData.Data;
    auto type = exprData.OutType;
    size_t dataSize = std::accumulate(std::begin(inputData[0].Count), std::end(inputData[0].Count),
                                      1, std::multiplies<size_t>());

#define declare_type_mult(T)                                                                       \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        T initVal = detail::ApplyExprOnConst<T, std::multiplies<T>>(exprData.Const, 1,             \
                                                                    std::multiplies<T>());         \
        T *multValues = detail::ApplyOneToOne<T>(                                                  \
            inputData.begin(), inputData.end(), dataSize, [](T a, T b) { return a * b; },          \
            initVal);                                                                              \
        return DerivedData({(void *)multValues});                                                  \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_mult)
    helper::Throw<std::invalid_argument>("Derived", "Function", "MultFunc",
                                         "Invalid variable types");
    return DerivedData();
}

// Perform a division from the first variable of all other variables in the std::vector
DerivedData DivFunc(ExprData exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::DivFunc");
    auto inputData = exprData.Data;
    auto type = exprData.OutType;
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
        return DerivedData({(void *)divValues});                                                   \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_div)
    helper::Throw<std::invalid_argument>("Derived", "Function", "DivFunc",
                                         "Invalid variable types");
    return DerivedData();
}

// Apply Sqrt over all elements in the variable
DerivedData SqrtFunc(ExprData exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::SqrtFunc");
    auto inputData = exprData.Data;
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
        return DerivedData({(void *)sqrtValues});
    }
#define declare_type_sqrt(T)                                                                       \
    else if (inputType == helper::GetDataType<T>())                                                \
    {                                                                                              \
        double *sqrtValues = (double *)malloc(dataSize * sizeof(double));                          \
        std::transform(reinterpret_cast<T *>(inputData[0].Data),                                   \
                       reinterpret_cast<T *>(inputData[0].Data) + dataSize, sqrtValues,            \
                       [](T &a) { return std::sqrt(a); });                                         \
        return DerivedData({(void *)sqrtValues});                                                  \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_sqrt)
    helper::Throw<std::invalid_argument>("Derived", "Function", "SqrtFunc",
                                         "Invalid variable types");
    return DerivedData();
}

// Apply Pow over all elements in the variable
DerivedData PowFunc(ExprData exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::PowFunc");
    auto inputData = exprData.Data;
    if (inputData.size() != 1 || exprData.Const.size() > 1)
    {
        helper::Throw<std::invalid_argument>("Derived", "Function", "PowFunc",
                                             "Invalid number of arguments passed to PowFunc");
    }
    size_t dataSize = std::accumulate(std::begin(inputData[0].Count), std::end(inputData[0].Count),
                                      1, std::multiplies<size_t>());
    DataType inputType = inputData[0].Type;
    size_t base = 2;
    if (exprData.Const.size() > 0)
        base = static_cast<size_t>(std::stoull(exprData.Const[0]));

    if (inputType == DataType::LongDouble)
    {
        long double *powValues = (long double *)malloc(dataSize * sizeof(long double));
        std::transform(reinterpret_cast<long double *>(inputData[0].Data),
                       reinterpret_cast<long double *>(inputData[0].Data) + dataSize, powValues,
                       [base](long double &a) { return std::pow(a, base); });
        return DerivedData({(void *)powValues});
    }
#define declare_type_pow(T)                                                                        \
    else if (inputType == helper::GetDataType<T>())                                                \
    {                                                                                              \
        double *powValues = (double *)malloc(dataSize * sizeof(double));                           \
        std::transform(reinterpret_cast<T *>(inputData[0].Data),                                   \
                       reinterpret_cast<T *>(inputData[0].Data) + dataSize, powValues,             \
                       [base](T &a) { return std::pow(a, base); });                                \
        return DerivedData({(void *)powValues});                                                   \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_pow)
    helper::Throw<std::invalid_argument>("Derived", "Function", "PowFunc",
                                         "Invalid variable types");
    return DerivedData();
}

DerivedData SinFunc(ExprData exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::SinFunc");
    auto inputData = exprData.Data;
    if (inputData.size() != 1)
    {
        helper::Throw<std::invalid_argument>("Derived", "Function", "SinFunc",
                                             "Invalid number of arguments passed to SinFunc");
    }
    size_t dataSize = std::accumulate(std::begin(inputData[0].Count), std::end(inputData[0].Count),
                                      1, std::multiplies<size_t>());
    DataType inputType = inputData[0].Type;

    if (inputType == DataType::LongDouble)
    {
        long double *sinValues = (long double *)malloc(dataSize * sizeof(long double));
        std::transform(reinterpret_cast<long double *>(inputData[0].Data),
                       reinterpret_cast<long double *>(inputData[0].Data) + dataSize, sinValues,
                       [](long double &a) { return std::sin(a); });
        return DerivedData({(void *)sinValues});
    }
#define declare_type_sin(T)                                                                        \
    if (inputType == helper::GetDataType<T>())                                                     \
    {                                                                                              \
        if (inputType != DataType::LongDouble)                                                     \
        {                                                                                          \
            double *sinValues = (double *)malloc(dataSize * sizeof(double));                       \
            std::transform(reinterpret_cast<T *>(inputData[0].Data),                               \
                           reinterpret_cast<T *>(inputData[0].Data) + dataSize, sinValues,         \
                           [](T &a) { return std::sin(a); });                                      \
            return DerivedData({(void *)sinValues});                                               \
        }                                                                                          \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_sin)
    helper::Throw<std::invalid_argument>("Derived", "Function", "SinFunc",
                                         "Invalid variable types");
    return DerivedData();
}

DerivedData CosFunc(ExprData exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::CosFunc");
    auto inputData = exprData.Data;
    if (inputData.size() != 1)
    {
        helper::Throw<std::invalid_argument>("Derived", "Function", "CosFunc",
                                             "Invalid number of arguments passed to CosFunc");
    }
    size_t dataSize = std::accumulate(std::begin(inputData[0].Count), std::end(inputData[0].Count),
                                      1, std::multiplies<size_t>());
    DataType inputType = inputData[0].Type;

    if (inputType == DataType::LongDouble)
    {
        long double *cosValues = (long double *)malloc(dataSize * sizeof(long double));
        std::transform(reinterpret_cast<long double *>(inputData[0].Data),
                       reinterpret_cast<long double *>(inputData[0].Data) + dataSize, cosValues,
                       [](long double &a) { return std::cos(a); });
        return DerivedData({(void *)cosValues});
    }
#define declare_type_cos(T)                                                                        \
    if (inputType == helper::GetDataType<T>())                                                     \
    {                                                                                              \
        if (inputType != DataType::LongDouble)                                                     \
        {                                                                                          \
            double *cosValues = (double *)malloc(dataSize * sizeof(double));                       \
            std::transform(reinterpret_cast<T *>(inputData[0].Data),                               \
                           reinterpret_cast<T *>(inputData[0].Data) + dataSize, cosValues,         \
                           [](T &a) { return std::cos(a); });                                      \
            return DerivedData({(void *)cosValues});                                               \
        }                                                                                          \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_cos)
    helper::Throw<std::invalid_argument>("Derived", "Function", "CosFunc",
                                         "Invalid variable types");
    return DerivedData();
}

DerivedData TanFunc(ExprData exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::TanFunc");
    auto inputData = exprData.Data;
    if (inputData.size() != 1)
    {
        helper::Throw<std::invalid_argument>("Derived", "Function", "TanFunc",
                                             "Invalid number of arguments passed to TanFunc");
    }
    size_t dataSize = std::accumulate(std::begin(inputData[0].Count), std::end(inputData[0].Count),
                                      1, std::multiplies<size_t>());
    DataType inputType = inputData[0].Type;

    if (inputType == DataType::LongDouble)
    {
        long double *tanValues = (long double *)malloc(dataSize * sizeof(long double));
        std::transform(reinterpret_cast<long double *>(inputData[0].Data),
                       reinterpret_cast<long double *>(inputData[0].Data) + dataSize, tanValues,
                       [](long double &a) { return std::tan(a); });
        return DerivedData({(void *)tanValues});
    }
#define declare_type_tan(T)                                                                        \
    if (inputType == helper::GetDataType<T>())                                                     \
    {                                                                                              \
        if (inputType != DataType::LongDouble)                                                     \
        {                                                                                          \
            double *tanValues = (double *)malloc(dataSize * sizeof(double));                       \
            std::transform(reinterpret_cast<T *>(inputData[0].Data),                               \
                           reinterpret_cast<T *>(inputData[0].Data) + dataSize, tanValues,         \
                           [](T &a) { return std::tan(a); });                                      \
            return DerivedData({(void *)tanValues});                                               \
        }                                                                                          \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_tan)
    helper::Throw<std::invalid_argument>("Derived", "Function", "TanFunc",
                                         "Invalid variable types");
    return DerivedData();
}

DerivedData AsinFunc(ExprData exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::AsinFunc");
    auto inputData = exprData.Data;
    if (inputData.size() != 1)
    {
        helper::Throw<std::invalid_argument>("Derived", "Function", "AsinFunc",
                                             "Invalid number of arguments passed to AsinFunc");
    }
    size_t dataSize = std::accumulate(std::begin(inputData[0].Count), std::end(inputData[0].Count),
                                      1, std::multiplies<size_t>());
    DataType inputType = inputData[0].Type;

    if (inputType == DataType::LongDouble)
    {
        long double *asinValues = (long double *)malloc(dataSize * sizeof(long double));
        std::transform(reinterpret_cast<long double *>(inputData[0].Data),
                       reinterpret_cast<long double *>(inputData[0].Data) + dataSize, asinValues,
                       [](long double &a) { return std::asin(a); });
        return DerivedData({(void *)asinValues});
    }
#define declare_type_asin(T)                                                                       \
    if (inputType == helper::GetDataType<T>())                                                     \
    {                                                                                              \
        if (inputType != DataType::LongDouble)                                                     \
        {                                                                                          \
            double *asinValues = (double *)malloc(dataSize * sizeof(double));                      \
            std::transform(reinterpret_cast<T *>(inputData[0].Data),                               \
                           reinterpret_cast<T *>(inputData[0].Data) + dataSize, asinValues,        \
                           [](T &a) { return std::asin(a); });                                     \
            return DerivedData({(void *)asinValues});                                              \
        }                                                                                          \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_asin)
    helper::Throw<std::invalid_argument>("Derived", "Function", "AsinFunc",
                                         "Invalid variable types");
    return DerivedData();
}

DerivedData AcosFunc(ExprData exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::AcosFunc");
    auto inputData = exprData.Data;
    if (inputData.size() != 1)
    {
        helper::Throw<std::invalid_argument>("Derived", "Function", "AcosFunc",
                                             "Invalid number of arguments passed to AcosFunc");
    }
    size_t dataSize = std::accumulate(std::begin(inputData[0].Count), std::end(inputData[0].Count),
                                      1, std::multiplies<size_t>());
    DataType inputType = inputData[0].Type;

    if (inputType == DataType::LongDouble)
    {
        long double *acosValues = (long double *)malloc(dataSize * sizeof(long double));
        std::transform(reinterpret_cast<long double *>(inputData[0].Data),
                       reinterpret_cast<long double *>(inputData[0].Data) + dataSize, acosValues,
                       [](long double &a) { return std::acos(a); });
        return DerivedData({(void *)acosValues});
    }
#define declare_type_acos(T)                                                                       \
    if (inputType == helper::GetDataType<T>())                                                     \
    {                                                                                              \
        if (inputType != DataType::LongDouble)                                                     \
        {                                                                                          \
            double *acosValues = (double *)malloc(dataSize * sizeof(double));                      \
            std::transform(reinterpret_cast<T *>(inputData[0].Data),                               \
                           reinterpret_cast<T *>(inputData[0].Data) + dataSize, acosValues,        \
                           [](T &a) { return std::acos(a); });                                     \
            return DerivedData({(void *)acosValues});                                              \
        }                                                                                          \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_acos)
    helper::Throw<std::invalid_argument>("Derived", "Function", "AcosFunc",
                                         "Invalid variable types");
    return DerivedData();
}

DerivedData AtanFunc(ExprData exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::AtanFunc");
    auto inputData = exprData.Data;
    if (inputData.size() != 1)
    {
        helper::Throw<std::invalid_argument>("Derived", "Function", "AtanFunc",
                                             "Invalid number of arguments passed to AtanFunc");
    }
    size_t dataSize = std::accumulate(std::begin(inputData[0].Count), std::end(inputData[0].Count),
                                      1, std::multiplies<size_t>());
    DataType inputType = inputData[0].Type;

    if (inputType == DataType::LongDouble)
    {
        long double *atanValues = (long double *)malloc(dataSize * sizeof(long double));
        std::transform(reinterpret_cast<long double *>(inputData[0].Data),
                       reinterpret_cast<long double *>(inputData[0].Data) + dataSize, atanValues,
                       [](long double &a) { return std::atan(a); });
        return DerivedData({(void *)atanValues});
    }
#define declare_type_atan(T)                                                                       \
    if (inputType == helper::GetDataType<T>())                                                     \
    {                                                                                              \
        if (inputType != DataType::LongDouble)                                                     \
        {                                                                                          \
            double *atanValues = (double *)malloc(dataSize * sizeof(double));                      \
            std::transform(reinterpret_cast<T *>(inputData[0].Data),                               \
                           reinterpret_cast<T *>(inputData[0].Data) + dataSize, atanValues,        \
                           [](T &a) { return std::atan(a); });                                     \
            return DerivedData({(void *)atanValues});                                              \
        }                                                                                          \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_atan)
    helper::Throw<std::invalid_argument>("Derived", "Function", "AtanFunc",
                                         "Invalid variable types");
    return DerivedData();
}

/* Magnitude can work on aggregated or separated  vectors*/
DerivedData MagAggregatedFunc(DerivedData inputData, DataType type)
{
    // the aggregation is done over the last dimension so the total size is d1 * d2 * .. d_n-1
    size_t dataSize = std::accumulate(std::begin(inputData.Count), std::end(inputData.Count) - 1, 1,
                                      std::multiplies<size_t>());

#define declare_type_agrmag(T)                                                                     \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        size_t numVar = inputData.Count.back();                                                    \
        T *magValues =                                                                             \
            detail::AggregateOnLastDim<T>(reinterpret_cast<T *>(inputData.Data), dataSize, numVar, \
                                          [](T a, T b) { return a + b * b; });                     \
        for (size_t i = 0; i < dataSize; i++)                                                      \
        {                                                                                          \
            magValues[i] = (T)std::sqrt(magValues[i]);                                             \
        }                                                                                          \
        return DerivedData({(void *)magValues});                                                   \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_agrmag)
    helper::Throw<std::invalid_argument>("Derived", "Function", "MagAggregateFunc",
                                         "Invalid variable types");
    return DerivedData();
}

DerivedData MagnitudeFunc(ExprData exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::MagnitudeFunc");
    auto inputData = exprData.Data;
    auto type = exprData.OutType;
    // if there is only one element return the aggregate result
    if (inputData.size() == 1)
        return MagAggregatedFunc(inputData[0], type);
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
        return DerivedData({(void *)magValues});                                                   \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_mag)
    helper::Throw<std::invalid_argument>("Derived", "Function", "MagnitudeFunc",
                                         "Invalid variable types");
    return DerivedData();
}

DerivedData Cross3DFunc(const ExprData exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::Cross3DFunc");
    auto inputData = exprData.Data;
    auto type = exprData.OutType;
    if (inputData.size() != 6)
    {
        helper::Throw<std::invalid_argument>("Derived", "Function", "Cross3DFunc",
                                             "Invalid number of arguments passed to Cross3DFunc");
    }
    size_t dataSize = std::accumulate(std::begin(inputData[0].Count), std::end(inputData[0].Count),
                                      1, std::multiplies<size_t>());

    DerivedData cross;

#define declare_type_cross(T)                                                                      \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        T *Ax = (T *)inputData[0].Data;                                                            \
        T *Ay = (T *)inputData[1].Data;                                                            \
        T *Az = (T *)inputData[2].Data;                                                            \
        T *Bx = (T *)inputData[3].Data;                                                            \
        T *By = (T *)inputData[4].Data;                                                            \
        T *Bz = (T *)inputData[5].Data;                                                            \
        cross.Data = detail::ApplyCross3D(Ax, Ay, Az, Bx, By, Bz, dataSize);                       \
        return cross;                                                                              \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_cross)
    helper::Throw<std::invalid_argument>("Derived", "Function", "Cross3DFunc",
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
DerivedData Curl3DFunc(const ExprData exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::Curl3DFunc");
    auto inputData = exprData.Data;
    auto type = exprData.OutType;

    // if the data is aggregated into a single array
    if (inputData.size() == 1)
    {
        // Curl is implemented for 3D arrays
        if (inputData[0].Count.size() != 4 && inputData[0].Count[3] != 3)
            helper::Throw<std::invalid_argument>(
                "Derived", "Function", "Curl3DFunc",
                "Invalid number of aggregated operands or operand dimentions for Curl");
        size_t dims[3] = {inputData[0].Count[0], inputData[0].Count[1], inputData[0].Count[2]};
        size_t dataSize =
            std::accumulate(std::begin(inputData[0].Count), std::end(inputData[0].Count) - 1, 1,
                            std::multiplies<size_t>());
        DerivedData curl;
        curl.Data = NULL;
#define declare_type_curlagg(T)                                                                    \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        T *input1 = (T *)inputData[0].Data;                                                        \
        T *input2 = (T *)inputData[0].Data + dataSize;                                             \
        T *input3 = (T *)inputData[0].Data + 2 * dataSize;                                         \
        curl.Data = detail::ApplyCurl(input1, input2, input3, dims);                               \
        return curl;                                                                               \
    }
        ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_curlagg)
        helper::Throw<std::invalid_argument>("Derived", "Function", "Curl3DFunc",
                                             "Invalid variable types");
        return DerivedData();
    }

    // if the data is not aggregated, the function expects 3 3D operands
    if (inputData.size() != 3)
    {
        helper::Throw<std::invalid_argument>("Derived", "Function", "Curl3DFunc",
                                             "Invalid number of operands for Curl, 3 expected, " +
                                                 std::to_string(inputData.size()) + " provided");
    }
    if (inputData[0].Count.size() != 3)
    {
        helper::Throw<std::invalid_argument>("Derived", "Function", "Curl3DFunc",
                                             "Invalid operand dimensions for Curl, 3 expected, " +
                                                 std::to_string(inputData[0].Count.size()) +
                                                 " provided");
    }
    size_t dims[3] = {inputData[0].Count[0], inputData[0].Count[1], inputData[0].Count[2]};

    DerivedData curl;
    curl.Data = NULL;
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

/* Functions that return output dimensions
 * Input: A list of variable dimensions (start, count, shape)
 * Output: (start, count, shape) of the output operation */

std::tuple<Dims, Dims, Dims> SameDimsFunc(std::vector<std::tuple<Dims, Dims, Dims>> input,
                                          bool constants)
{
    // check that all dimenstions are the same
    if (input.size() > 1)
    {
        auto first_element = input[0];
        bool dim_are_equal = std::all_of(
            input.begin() + 1, input.end(),
            [&first_element](std::tuple<Dims, Dims, Dims> x) { return x == first_element; });
        if (!dim_are_equal)
            helper::Throw<std::invalid_argument>("Derived", "Function", "SameDimsFunc",
                                                 "Invalid variable dimensions");
    }
    if (input.size() >= 1)
    {
        // return the first dimension
        return input[0];
    }
    helper::Throw<std::invalid_argument>("Derived", "Function", "SameDimsFunc",
                                         "Geting dims for expression without operands");
    return {{}, {}, {}};
}

std::tuple<Dims, Dims, Dims> SameDimsWithAgrFunc(std::vector<std::tuple<Dims, Dims, Dims>> input,
                                                 bool constants)
{
    if (input.size() > 1 || constants)
        return SameDimsFunc(input, constants);
    if (input.size() == 0)
    {
        helper::Throw<std::invalid_argument>("Derived", "Function", "SameDimsAgrFunc",
                                             "Geting dims for expression without operands");
        return {{}, {}, {}};
    }
    Dims outStart(std::get<0>(input[0]).size() - 1);
    Dims outCount(std::get<1>(input[0]).size() - 1);
    Dims outShape(std::get<2>(input[0]).size() - 1);
    std::copy(std::get<0>(input[0]).begin(), std::get<0>(input[0]).end() - 1, outStart.begin());
    std::copy(std::get<1>(input[0]).begin(), std::get<1>(input[0]).end() - 1, outCount.begin());
    std::copy(std::get<2>(input[0]).begin(), std::get<2>(input[0]).end() - 1, outShape.begin());
    return {outStart, outCount, outShape};
}

std::tuple<Dims, Dims, Dims> Cross3DDimsFunc(std::vector<std::tuple<Dims, Dims, Dims>> input,
                                             bool constants)
{
    // check that all dimenstions are the same
    if (input.size() > 1)
    {
        auto first_element = input[0];
        bool dim_are_equal = std::all_of(
            input.begin() + 1, input.end(),
            [&first_element](std::tuple<Dims, Dims, Dims> x) { return x == first_element; });
        if (!dim_are_equal)
            helper::Throw<std::invalid_argument>("Derived", "Function", "Cross3DDimsFunc",
                                                 "Invalid variable dimensions");
    }
    if (input.size() == 0)
    {
        helper::Throw<std::invalid_argument>("Derived", "Function", "Cross3DDims",
                                             "Geting dims for expression without operands");
        return {{}, {}, {}};
    }
    // return original dimensions with added dimension of number of inputs
    std::tuple<Dims, Dims, Dims> output = input[0];
    std::get<0>(output).push_back(0);
    std::get<1>(output).push_back(3);
    std::get<2>(output).push_back(3);
    return output;
}

// Input Dims are the same, output is combination of all inputs
std::tuple<Dims, Dims, Dims> CurlDimsFunc(std::vector<std::tuple<Dims, Dims, Dims>> input,
                                          bool constants)
{
    // check that all dimenstions are the same
    if (input.size() > 1)
    {
        auto first_element = input[0];
        bool dim_are_equal = std::all_of(
            input.begin() + 1, input.end(),
            [&first_element](std::tuple<Dims, Dims, Dims> x) { return x == first_element; });
        if (!dim_are_equal)
            helper::Throw<std::invalid_argument>("Derived", "Function", "CurlDimsFunc",
                                                 "Invalid variable dimensions");
    }

    // if curl is computed on arrays aggregated into one array on the last dimension
    // we need to remove this dimension and set the last dimension to 3 since we only
    // support curl for 3D arrays
    if (input.size() == 1)
    {
        Dims outStart(std::get<0>(input[0]).size() - 1);
        Dims outCount(std::get<1>(input[0]).size() - 1);
        Dims outShape(std::get<2>(input[0]).size() - 1);
        std::copy(std::get<0>(input[0]).begin(), std::get<0>(input[0]).end() - 1, outStart.begin());
        std::copy(std::get<1>(input[0]).begin(), std::get<1>(input[0]).end() - 1, outCount.begin());
        std::copy(std::get<2>(input[0]).begin(), std::get<2>(input[0]).end() - 1, outShape.begin());
        outStart.push_back(0);
        outCount.push_back(3);
        outShape.push_back(3);
        return {outStart, outCount, outShape};
    }
    if (input.size() == 0)
    {
        helper::Throw<std::invalid_argument>("Derived", "Function", "CurlDims",
                                             "Geting dims for expression without operands");
        return {{}, {}, {}};
    }
    // return original dimensions with added dimension of number of inputs
    // since we only support curl for 3D arrays, the number of inputs is 3
    std::tuple<Dims, Dims, Dims> output = input[0];
    std::get<0>(output).push_back(0);
    std::get<1>(output).push_back(3);
    std::get<2>(output).push_back(3);
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
