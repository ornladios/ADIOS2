/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
template <class T, class Op, class Iterator>
void ApplyOneToOne(T *outValues, Iterator inputBegin, Iterator inputEnd, size_t dataSize, Op op,
                   T initVal = (T)0)
{
    for (size_t i = 0; i < dataSize; i++)
        outValues[i] = initVal;
    for (Iterator variable = inputBegin; variable != inputEnd; ++variable)
    {
        T *src = reinterpret_cast<T *>((*variable).Data);
        if ((*variable).IsScalar)
        {
            T val = src[0];
            for (size_t i = 0; i < dataSize; i++)
                outValues[i] = op(outValues[i], val);
        }
        else
        {
            for (size_t i = 0; i < dataSize; i++)
                outValues[i] = op(outValues[i], src[i]);
        }
    }
}

template <class T, class Op>
void AggregateOnLastDim(T *outValues, T *data, size_t dataSize, size_t nVariables, Op op)
{
    memset(outValues, 0, dataSize * sizeof(T));
    for (size_t i = 0; i < dataSize; i++)
    {
        size_t start = nVariables * i;
        for (size_t variable = 0; variable < nVariables; ++variable)
            outValues[i] = op(outValues[i], data[start + variable]);
    }
}

inline size_t returnIndex(size_t x, size_t y, size_t z, const size_t dims[3])
{
    return z + y * dims[2] + x * dims[2] * dims[1];
}

template <class T>
void ApplyCross3D(T *data, const T *Ax, const T *Ay, const T *Az, const T *Bx, const T *By,
                  const T *Bz, const size_t dataSize)
{
    for (size_t i = 0; i < dataSize; ++i)
    {
        data[3 * i] = (Ay[i] * Bz[i]) - (Az[i] * By[i]);
        data[3 * i + 1] = (Ax[i] * Bz[i]) - (Az[i] * Bx[i]);
        data[3 * i + 2] = (Ax[i] * By[i]) - (Ay[i] * Bx[i]);
    }
}

template <class T>
void ApplyCurl(T *data, const T *input1, const T *input2, const T *input3, const size_t dims[3])
{
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
}

}

namespace derived
{
// Perform a reduce sum over all variables in the std::vector
DerivedData AddAggregatedFunc(void *output, DerivedData inputData, DataType type)
{
    // the aggregation is done over the last dimension so the total size is d1 * d2 * .. d_n-1
    size_t dataSize = std::accumulate(std::begin(inputData.Count), std::end(inputData.Count) - 1, 1,
                                      std::multiplies<size_t>());
#define declare_type_agradd(T)                                                                     \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        size_t numVar = inputData.Count.back();                                                    \
        detail::AggregateOnLastDim<T>(reinterpret_cast<T *>(output),                               \
                                      reinterpret_cast<T *>(inputData.Data), dataSize, numVar,     \
                                      [](auto a, auto b) { return a + b; });                       \
        return DerivedData({output});                                                              \
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
    if (inputData.size() == 1)
        return AddAggregatedFunc(exprData.Output, inputData[0], type);

    size_t dataSize = exprData.OutputSize;
#define declare_type_add(T)                                                                        \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        T *addValues = reinterpret_cast<T *>(exprData.Output);                                     \
        detail::ApplyOneToOne<T>(addValues, inputData.begin(), inputData.end(), dataSize,          \
                                 [](auto a, auto b) { return a + b; });                            \
        return DerivedData({exprData.Output});                                                     \
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
    size_t dataSize = exprData.OutputSize;

// Perform a reduce sum over all variables in the std::vector except the first one
// and remove this sum from the first buffer
#define declare_type_subtract(T)                                                                   \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        T *out = reinterpret_cast<T *>(exprData.Output);                                           \
        detail::ApplyOneToOne<T>(out, inputData.begin() + 1, inputData.end(), dataSize,            \
                                 [](auto a, auto b) { return a + b; });                            \
        T *lhs = reinterpret_cast<T *>(inputData[0].Data);                                         \
        if (inputData[0].IsScalar)                                                                 \
        {                                                                                          \
            T val = lhs[0];                                                                        \
            for (size_t i = 0; i < dataSize; i++)                                                  \
                out[i] = val - out[i];                                                             \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            for (size_t i = 0; i < dataSize; i++)                                                  \
                out[i] = lhs[i] - out[i];                                                          \
        }                                                                                          \
        return DerivedData({exprData.Output});                                                     \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_subtract)
    helper::Throw<std::invalid_argument>("Derived", "Function", "SubtractFunc",
                                         "Invalid variable types");
    return DerivedData();
}

// Negate all elements in the variable
DerivedData NegateFunc(ExprData exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::NegateFunc");
    auto inputData = exprData.Data;
    if (inputData.size() != 1)
    {
        helper::Throw<std::invalid_argument>("Derived", "Function", "NegateFunc",
                                             "Invalid number of arguments passed to NegateFunc");
    }
    size_t dataSize = exprData.OutputSize;
    DataType inputType = inputData[0].Type;

#define declare_type_negate(T)                                                                     \
    if (inputType == helper::GetDataType<T>())                                                     \
    {                                                                                              \
        T *negValues = reinterpret_cast<T *>(exprData.Output);                                     \
        std::transform(reinterpret_cast<T *>(inputData[0].Data),                                   \
                       reinterpret_cast<T *>(inputData[0].Data) + dataSize, negValues,             \
                       [](T &a) { return -a; });                                                   \
        return DerivedData({exprData.Output});                                                     \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_negate)
    helper::Throw<std::invalid_argument>("Derived", "Function", "NegateFunc",
                                         "Invalid variable types");
    return DerivedData();
}

// Perform a reduce multiply over all variables in the std::vector
DerivedData MultFunc(ExprData exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::MultFunc");
    auto inputData = exprData.Data;
    auto type = exprData.OutType;
    size_t dataSize = exprData.OutputSize;

#define declare_type_mult(T)                                                                       \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        T *multValues = reinterpret_cast<T *>(exprData.Output);                                    \
        detail::ApplyOneToOne<T>(                                                                  \
            multValues, inputData.begin(), inputData.end(), dataSize,                              \
            [](auto a, auto b) { return a * b; }, (T)1);                                           \
        return DerivedData({exprData.Output});                                                     \
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
    size_t dataSize = exprData.OutputSize;

// Perform a reduce multiply over all variables in the std::vector except the first one
// and divide this value from the first buffer
#define declare_type_div(T)                                                                        \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        T *out = reinterpret_cast<T *>(exprData.Output);                                           \
        detail::ApplyOneToOne<T>(                                                                  \
            out, inputData.begin() + 1, inputData.end(), dataSize,                                 \
            [](auto a, auto b) { return a * b; }, (T)1);                                           \
        T *lhs = reinterpret_cast<T *>(inputData[0].Data);                                         \
        if (inputData[0].IsScalar)                                                                 \
        {                                                                                          \
            T val = lhs[0];                                                                        \
            for (size_t i = 0; i < dataSize; i++)                                                  \
                out[i] = val / out[i];                                                             \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            for (size_t i = 0; i < dataSize; i++)                                                  \
                out[i] = lhs[i] / out[i];                                                          \
        }                                                                                          \
        return DerivedData({exprData.Output});                                                     \
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
    size_t dataSize = exprData.OutputSize;
    DataType inputType = inputData[0].Type;

    if (inputType == DataType::LongDouble)
    {
        long double *sqrtValues = reinterpret_cast<long double *>(exprData.Output);
        std::transform(reinterpret_cast<long double *>(inputData[0].Data),
                       reinterpret_cast<long double *>(inputData[0].Data) + dataSize, sqrtValues,
                       [](auto &a) { return std::sqrt(a); });
        return DerivedData({exprData.Output});
    }
#define declare_type_sqrt(T)                                                                       \
    else if (inputType == helper::GetDataType<T>())                                                \
    {                                                                                              \
        double *sqrtValues = reinterpret_cast<double *>(exprData.Output);                          \
        std::transform(reinterpret_cast<T *>(inputData[0].Data),                                   \
                       reinterpret_cast<T *>(inputData[0].Data) + dataSize, sqrtValues,            \
                       [](auto &a) { return std::sqrt(a); });                                      \
        return DerivedData({exprData.Output});                                                     \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_sqrt)
    helper::Throw<std::invalid_argument>("Derived", "Function", "SqrtFunc",
                                         "Invalid variable types");
    return DerivedData();
}

// Apply Pow over all elements: base array raised to exponent array element-wise
// With 1 operand, defaults to squaring; with 2 operands, uses second as exponent
DerivedData PowFunc(ExprData exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::PowFunc");
    auto inputData = exprData.Data;
    if (inputData.size() < 1 || inputData.size() > 2)
    {
        helper::Throw<std::invalid_argument>("Derived", "Function", "PowFunc",
                                             "Invalid number of arguments passed to PowFunc");
    }
    size_t dataSize = exprData.OutputSize;
    DataType inputType = inputData[0].Type;

    if (inputType == DataType::LongDouble)
    {
        long double *out = reinterpret_cast<long double *>(exprData.Output);
        long double *base = reinterpret_cast<long double *>(inputData[0].Data);
        if (inputData.size() == 2)
        {
            long double *expData = reinterpret_cast<long double *>(inputData[1].Data);
            if (inputData[1].IsScalar)
            {
                long double exp = expData[0];
                if (exp == 2.0L)
                    for (size_t i = 0; i < dataSize; i++)
                        out[i] = base[i] * base[i];
                else
                    for (size_t i = 0; i < dataSize; i++)
                        out[i] = std::pow(base[i], exp);
            }
            else
                for (size_t i = 0; i < dataSize; i++)
                    out[i] = std::pow(base[i], expData[i]);
        }
        else
            for (size_t i = 0; i < dataSize; i++)
                out[i] = base[i] * base[i];
        return DerivedData({exprData.Output});
    }
#define declare_type_pow(T)                                                                        \
    else if (inputType == helper::GetDataType<T>())                                                \
    {                                                                                              \
        double *out = reinterpret_cast<double *>(exprData.Output);                                 \
        T *base = reinterpret_cast<T *>(inputData[0].Data);                                        \
        if (inputData.size() == 2)                                                                 \
        {                                                                                          \
            T *expData = reinterpret_cast<T *>(inputData[1].Data);                                 \
            if (inputData[1].IsScalar)                                                             \
            {                                                                                      \
                double exp = (double)expData[0];                                                   \
                if (exp == 2.0)                                                                    \
                    for (size_t i = 0; i < dataSize; i++)                                          \
                        out[i] = (double)base[i] * (double)base[i];                                \
                else                                                                               \
                    for (size_t i = 0; i < dataSize; i++)                                          \
                        out[i] = std::pow((double)base[i], exp);                                   \
            }                                                                                      \
            else                                                                                   \
                for (size_t i = 0; i < dataSize; i++)                                              \
                    out[i] = std::pow((double)base[i], (double)expData[i]);                        \
        }                                                                                          \
        else                                                                                       \
            for (size_t i = 0; i < dataSize; i++)                                                  \
                out[i] = (double)base[i] * (double)base[i];                                        \
        return DerivedData({exprData.Output});                                                     \
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
    size_t dataSize = exprData.OutputSize;
    DataType inputType = inputData[0].Type;

    if (inputType == DataType::LongDouble)
    {
        long double *sinValues = reinterpret_cast<long double *>(exprData.Output);
        std::transform(reinterpret_cast<long double *>(inputData[0].Data),
                       reinterpret_cast<long double *>(inputData[0].Data) + dataSize, sinValues,
                       [](auto &a) { return std::sin(a); });
        return DerivedData({exprData.Output});
    }
#define declare_type_sin(T)                                                                        \
    if (inputType == helper::GetDataType<T>())                                                     \
    {                                                                                              \
        if (inputType != DataType::LongDouble)                                                     \
        {                                                                                          \
            double *sinValues = reinterpret_cast<double *>(exprData.Output);                       \
            std::transform(reinterpret_cast<T *>(inputData[0].Data),                               \
                           reinterpret_cast<T *>(inputData[0].Data) + dataSize, sinValues,         \
                           [](T &a) { return std::sin(a); });                                      \
            return DerivedData({exprData.Output});                                                 \
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
    size_t dataSize = exprData.OutputSize;
    DataType inputType = inputData[0].Type;

    if (inputType == DataType::LongDouble)
    {
        long double *cosValues = reinterpret_cast<long double *>(exprData.Output);
        std::transform(reinterpret_cast<long double *>(inputData[0].Data),
                       reinterpret_cast<long double *>(inputData[0].Data) + dataSize, cosValues,
                       [](auto &a) { return std::cos(a); });
        return DerivedData({exprData.Output});
    }
#define declare_type_cos(T)                                                                        \
    if (inputType == helper::GetDataType<T>())                                                     \
    {                                                                                              \
        if (inputType != DataType::LongDouble)                                                     \
        {                                                                                          \
            double *cosValues = reinterpret_cast<double *>(exprData.Output);                       \
            std::transform(reinterpret_cast<T *>(inputData[0].Data),                               \
                           reinterpret_cast<T *>(inputData[0].Data) + dataSize, cosValues,         \
                           [](T &a) { return std::cos(a); });                                      \
            return DerivedData({exprData.Output});                                                 \
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
    size_t dataSize = exprData.OutputSize;
    DataType inputType = inputData[0].Type;

    if (inputType == DataType::LongDouble)
    {
        long double *tanValues = reinterpret_cast<long double *>(exprData.Output);
        std::transform(reinterpret_cast<long double *>(inputData[0].Data),
                       reinterpret_cast<long double *>(inputData[0].Data) + dataSize, tanValues,
                       [](auto &a) { return std::tan(a); });
        return DerivedData({exprData.Output});
    }
#define declare_type_tan(T)                                                                        \
    if (inputType == helper::GetDataType<T>())                                                     \
    {                                                                                              \
        if (inputType != DataType::LongDouble)                                                     \
        {                                                                                          \
            double *tanValues = reinterpret_cast<double *>(exprData.Output);                       \
            std::transform(reinterpret_cast<T *>(inputData[0].Data),                               \
                           reinterpret_cast<T *>(inputData[0].Data) + dataSize, tanValues,         \
                           [](T &a) { return std::tan(a); });                                      \
            return DerivedData({exprData.Output});                                                 \
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
    size_t dataSize = exprData.OutputSize;
    DataType inputType = inputData[0].Type;

    if (inputType == DataType::LongDouble)
    {
        long double *asinValues = reinterpret_cast<long double *>(exprData.Output);
        std::transform(reinterpret_cast<long double *>(inputData[0].Data),
                       reinterpret_cast<long double *>(inputData[0].Data) + dataSize, asinValues,
                       [](auto &a) { return std::asin(a); });
        return DerivedData({exprData.Output});
    }
#define declare_type_asin(T)                                                                       \
    if (inputType == helper::GetDataType<T>())                                                     \
    {                                                                                              \
        if (inputType != DataType::LongDouble)                                                     \
        {                                                                                          \
            double *asinValues = reinterpret_cast<double *>(exprData.Output);                      \
            std::transform(reinterpret_cast<T *>(inputData[0].Data),                               \
                           reinterpret_cast<T *>(inputData[0].Data) + dataSize, asinValues,        \
                           [](T &a) { return std::asin(a); });                                     \
            return DerivedData({exprData.Output});                                                 \
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
    size_t dataSize = exprData.OutputSize;
    DataType inputType = inputData[0].Type;

    if (inputType == DataType::LongDouble)
    {
        long double *acosValues = reinterpret_cast<long double *>(exprData.Output);
        std::transform(reinterpret_cast<long double *>(inputData[0].Data),
                       reinterpret_cast<long double *>(inputData[0].Data) + dataSize, acosValues,
                       [](auto &a) { return std::acos(a); });
        return DerivedData({exprData.Output});
    }
#define declare_type_acos(T)                                                                       \
    if (inputType == helper::GetDataType<T>())                                                     \
    {                                                                                              \
        if (inputType != DataType::LongDouble)                                                     \
        {                                                                                          \
            double *acosValues = reinterpret_cast<double *>(exprData.Output);                      \
            std::transform(reinterpret_cast<T *>(inputData[0].Data),                               \
                           reinterpret_cast<T *>(inputData[0].Data) + dataSize, acosValues,        \
                           [](T &a) { return std::acos(a); });                                     \
            return DerivedData({exprData.Output});                                                 \
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
    size_t dataSize = exprData.OutputSize;
    DataType inputType = inputData[0].Type;

    if (inputType == DataType::LongDouble)
    {
        long double *atanValues = reinterpret_cast<long double *>(exprData.Output);
        std::transform(reinterpret_cast<long double *>(inputData[0].Data),
                       reinterpret_cast<long double *>(inputData[0].Data) + dataSize, atanValues,
                       [](auto &a) { return std::atan(a); });
        return DerivedData({exprData.Output});
    }
#define declare_type_atan(T)                                                                       \
    if (inputType == helper::GetDataType<T>())                                                     \
    {                                                                                              \
        if (inputType != DataType::LongDouble)                                                     \
        {                                                                                          \
            double *atanValues = reinterpret_cast<double *>(exprData.Output);                      \
            std::transform(reinterpret_cast<T *>(inputData[0].Data),                               \
                           reinterpret_cast<T *>(inputData[0].Data) + dataSize, atanValues,        \
                           [](T &a) { return std::atan(a); });                                     \
            return DerivedData({exprData.Output});                                                 \
        }                                                                                          \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_atan)
    helper::Throw<std::invalid_argument>("Derived", "Function", "AtanFunc",
                                         "Invalid variable types");
    return DerivedData();
}

/* Magnitude can work on aggregated or separated  vectors*/
DerivedData MagAggregatedFunc(void *output, DerivedData inputData, DataType type)
{
    // the aggregation is done over the last dimension so the total size is d1 * d2 * .. d_n-1
    size_t dataSize = std::accumulate(std::begin(inputData.Count), std::end(inputData.Count) - 1, 1,
                                      std::multiplies<size_t>());

#define declare_type_agrmag(T)                                                                     \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        size_t numVar = inputData.Count.back();                                                    \
        T *magValues = reinterpret_cast<T *>(output);                                              \
        detail::AggregateOnLastDim<T>(magValues, reinterpret_cast<T *>(inputData.Data), dataSize,  \
                                      numVar, [](auto a, auto b) { return a + b * b; });           \
        for (size_t i = 0; i < dataSize; i++)                                                      \
        {                                                                                          \
            magValues[i] = (T)std::sqrt(magValues[i]);                                             \
        }                                                                                          \
        return DerivedData({output});                                                              \
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
        return MagAggregatedFunc(exprData.Output, inputData[0], type);
    size_t dataSize = exprData.OutputSize;
#define declare_type_mag(T)                                                                        \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        T *magValues = reinterpret_cast<T *>(exprData.Output);                                     \
        detail::ApplyOneToOne<T>(magValues, inputData.begin(), inputData.end(), dataSize,          \
                                 [](auto a, auto b) { return a + b * b; });                        \
        for (size_t i = 0; i < dataSize; i++)                                                      \
        {                                                                                          \
            magValues[i] = (T)std::sqrt(magValues[i]);                                             \
        }                                                                                          \
        return DerivedData({exprData.Output});                                                     \
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
    // dataSize is per-component input size; output is 3x this
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
        detail::ApplyCross3D(reinterpret_cast<T *>(exprData.Output), Ax, Ay, Az, Bx, By, Bz,       \
                             dataSize);                                                            \
        cross.Data = exprData.Output;                                                              \
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
        DerivedData curl;
        curl.Data = NULL;
#define declare_type_curlagg(T)                                                                    \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        T *input1 = (T *)inputData[0].Data;                                                        \
        T *input2 = (T *)inputData[0].Data + dims[0] * dims[1] * dims[2];                          \
        T *input3 = (T *)inputData[0].Data + 2 * dims[0] * dims[1] * dims[2];                      \
        detail::ApplyCurl(reinterpret_cast<T *>(exprData.Output), input1, input2, input3, dims);   \
        curl.Data = exprData.Output;                                                               \
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
        detail::ApplyCurl(reinterpret_cast<T *>(exprData.Output), input1, input2, input3, dims);   \
        curl.Data = exprData.Output;                                                               \
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

std::tuple<Dims, Dims, Dims> SameDimsFunc(std::vector<std::tuple<Dims, Dims, Dims>> input)
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

std::tuple<Dims, Dims, Dims> SameDimsWithAgrFunc(std::vector<std::tuple<Dims, Dims, Dims>> input)
{
    if (input.size() > 1)
        return SameDimsFunc(input);
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

std::tuple<Dims, Dims, Dims> Cross3DDimsFunc(std::vector<std::tuple<Dims, Dims, Dims>> input)
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
std::tuple<Dims, Dims, Dims> CurlDimsFunc(std::vector<std::tuple<Dims, Dims, Dims>> input)
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
