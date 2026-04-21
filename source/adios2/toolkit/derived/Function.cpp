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

// --- Typed element-wise operations ---
// Each Do* function: casts void pointers, handles scalar broadcast, contains the loop.
// Two levels only: *Func → DISPATCH_TYPE → Do*.

// Binary op with per-element type promotion. When LhsT==RhsT==OutT (homogeneous),
// the static_casts are no-ops and the compiler optimizes them away.
template <typename OutT, typename LhsT, typename RhsT>
void DoAdd(void *out, const derived::DerivedData &lhs, const derived::DerivedData &rhs, size_t N)
{
    OutT *o = reinterpret_cast<OutT *>(out);
    LhsT *l = reinterpret_cast<LhsT *>(lhs.Data);
    RhsT *r = reinterpret_cast<RhsT *>(rhs.Data);
    if (lhs.IsScalar)
    {
        OutT val = static_cast<OutT>(l[0]);
        for (size_t i = 0; i < N; i++)
            o[i] = val + static_cast<OutT>(r[i]);
    }
    else if (rhs.IsScalar)
    {
        OutT val = static_cast<OutT>(r[0]);
        for (size_t i = 0; i < N; i++)
            o[i] = static_cast<OutT>(l[i]) + val;
    }
    else
    {
        for (size_t i = 0; i < N; i++)
            o[i] = static_cast<OutT>(l[i]) + static_cast<OutT>(r[i]);
    }
}

template <typename OutT, typename LhsT, typename RhsT>
void DoSub(void *out, const derived::DerivedData &lhs, const derived::DerivedData &rhs, size_t N)
{
    OutT *o = reinterpret_cast<OutT *>(out);
    LhsT *l = reinterpret_cast<LhsT *>(lhs.Data);
    RhsT *r = reinterpret_cast<RhsT *>(rhs.Data);
    if (lhs.IsScalar)
    {
        OutT val = static_cast<OutT>(l[0]);
        for (size_t i = 0; i < N; i++)
            o[i] = val - static_cast<OutT>(r[i]);
    }
    else if (rhs.IsScalar)
    {
        OutT val = static_cast<OutT>(r[0]);
        for (size_t i = 0; i < N; i++)
            o[i] = static_cast<OutT>(l[i]) - val;
    }
    else
    {
        for (size_t i = 0; i < N; i++)
            o[i] = static_cast<OutT>(l[i]) - static_cast<OutT>(r[i]);
    }
}

template <typename OutT, typename LhsT, typename RhsT>
void DoMul(void *out, const derived::DerivedData &lhs, const derived::DerivedData &rhs, size_t N)
{
    OutT *o = reinterpret_cast<OutT *>(out);
    LhsT *l = reinterpret_cast<LhsT *>(lhs.Data);
    RhsT *r = reinterpret_cast<RhsT *>(rhs.Data);
    if (lhs.IsScalar)
    {
        OutT val = static_cast<OutT>(l[0]);
        for (size_t i = 0; i < N; i++)
            o[i] = val * static_cast<OutT>(r[i]);
    }
    else if (rhs.IsScalar)
    {
        OutT val = static_cast<OutT>(r[0]);
        for (size_t i = 0; i < N; i++)
            o[i] = static_cast<OutT>(l[i]) * val;
    }
    else
    {
        for (size_t i = 0; i < N; i++)
            o[i] = static_cast<OutT>(l[i]) * static_cast<OutT>(r[i]);
    }
}

template <typename OutT, typename LhsT, typename RhsT>
void DoDiv(void *out, const derived::DerivedData &lhs, const derived::DerivedData &rhs, size_t N)
{
    OutT *o = reinterpret_cast<OutT *>(out);
    LhsT *l = reinterpret_cast<LhsT *>(lhs.Data);
    RhsT *r = reinterpret_cast<RhsT *>(rhs.Data);
    if (lhs.IsScalar)
    {
        OutT val = static_cast<OutT>(l[0]);
        for (size_t i = 0; i < N; i++)
            o[i] = val / static_cast<OutT>(r[i]);
    }
    else if (rhs.IsScalar)
    {
        OutT val = static_cast<OutT>(r[0]);
        for (size_t i = 0; i < N; i++)
            o[i] = static_cast<OutT>(l[i]) / val;
    }
    else
    {
        for (size_t i = 0; i < N; i++)
            o[i] = static_cast<OutT>(l[i]) / static_cast<OutT>(r[i]);
    }
}

template <typename T>
void DoPow(void *out, const derived::DerivedData &base, const derived::DerivedData &exp, size_t N)
{
    T *o = reinterpret_cast<T *>(out);
    T *b = reinterpret_cast<T *>(base.Data);
    T *e = reinterpret_cast<T *>(exp.Data);
    if (exp.IsScalar)
    {
        T ev = e[0];
        if (ev == static_cast<T>(2))
        {
            for (size_t i = 0; i < N; i++)
                o[i] = b[i] * b[i];
        }
        else
        {
            for (size_t i = 0; i < N; i++)
                o[i] = std::pow(b[i], ev);
        }
    }
    else
    {
        for (size_t i = 0; i < N; i++)
            o[i] = std::pow(b[i], e[i]);
    }
}

// Promote: convert array from one type to another, element by element.
template <typename OutT, typename InT>
void DoPromoteTyped(void *out, void *in, size_t N)
{
    OutT *o = reinterpret_cast<OutT *>(out);
    const InT *src = reinterpret_cast<const InT *>(in);
    for (size_t i = 0; i < N; i++)
        o[i] = static_cast<OutT>(src[i]);
}

template <typename T>
void DoNegate(void *out, void *in, size_t N)
{
    T *o = reinterpret_cast<T *>(out);
    const T *src = reinterpret_cast<const T *>(in);
    for (size_t i = 0; i < N; i++)
        o[i] = static_cast<T>(0) - src[i];
}

template <typename T>
void DoSqrt(void *out, void *in, size_t N)
{
    T *o = reinterpret_cast<T *>(out);
    const T *src = reinterpret_cast<const T *>(in);
    for (size_t i = 0; i < N; i++)
        o[i] = std::sqrt(src[i]);
}

#define DEFINE_UNARY_MATH(Name, func)                                                              \
    template <typename T>                                                                          \
    void Do##Name(void *out, void *in, size_t N)                                                   \
    {                                                                                              \
        T *o = reinterpret_cast<T *>(out);                                                         \
        const T *src = reinterpret_cast<const T *>(in);                                            \
        for (size_t i = 0; i < N; i++)                                                             \
            o[i] = std::func(src[i]);                                                              \
    }
DEFINE_UNARY_MATH(Sin, sin)
DEFINE_UNARY_MATH(Cos, cos)
DEFINE_UNARY_MATH(Tan, tan)
DEFINE_UNARY_MATH(Asin, asin)
DEFINE_UNARY_MATH(Acos, acos)
DEFINE_UNARY_MATH(Atan, atan)
#undef DEFINE_UNARY_MATH

template <typename T>
void DoMagnitude3(void *out, void *a, void *b, void *c, size_t N)
{
    T *o = reinterpret_cast<T *>(out);
    const T *aa = reinterpret_cast<const T *>(a);
    const T *bb = reinterpret_cast<const T *>(b);
    const T *cc = reinterpret_cast<const T *>(c);
    for (size_t i = 0; i < N; i++)
        o[i] = std::sqrt(aa[i] * aa[i] + bb[i] * bb[i] + cc[i] * cc[i]);
}

// General N-component magnitude: out[i] = sqrt(sum of comp[k][i]^2)
template <typename T>
void DoMagnitudeN(void *out, const std::vector<derived::DerivedData> &inputs, size_t N)
{
    T *o = reinterpret_cast<T *>(out);
    // Initialize with first component squared
    const T *c0 = reinterpret_cast<const T *>(inputs[0].Data);
    for (size_t i = 0; i < N; i++)
        o[i] = c0[i] * c0[i];
    // Accumulate remaining components
    for (size_t k = 1; k < inputs.size(); k++)
    {
        const T *ck = reinterpret_cast<const T *>(inputs[k].Data);
        for (size_t i = 0; i < N; i++)
            o[i] += ck[i] * ck[i];
    }
    // Sqrt
    for (size_t i = 0; i < N; i++)
        o[i] = std::sqrt(o[i]);
}

// Dispatch a binary op, handling mixed float/double inline.
// FUNC must take three type params: FUNC<OutT, LhsT, RhsT>(out, lhs, rhs, N).
// For homogeneous types, all three are the same. For float/double mixes, uses actual types.
#define DISPATCH_BINARY(outType, lhsType, rhsType, FUNC, ...)                                      \
    do                                                                                             \
    {                                                                                              \
        if (lhsType == rhsType)                                                                    \
        {                                                                                          \
            /* Homogeneous — single type for all three params */                                 \
            switch (outType)                                                                       \
            {                                                                                      \
            case DataType::Float:                                                                  \
                FUNC<float, float, float>(__VA_ARGS__);                                            \
                break;                                                                             \
            case DataType::Double:                                                                 \
                FUNC<double, double, double>(__VA_ARGS__);                                         \
                break;                                                                             \
            case DataType::LongDouble:                                                             \
                FUNC<long double, long double, long double>(__VA_ARGS__);                          \
                break;                                                                             \
            case DataType::Int32:                                                                  \
                FUNC<int32_t, int32_t, int32_t>(__VA_ARGS__);                                      \
                break;                                                                             \
            case DataType::Int64:                                                                  \
                FUNC<int64_t, int64_t, int64_t>(__VA_ARGS__);                                      \
                break;                                                                             \
            case DataType::FloatComplex:                                                           \
                FUNC<std::complex<float>, std::complex<float>, std::complex<float>>(__VA_ARGS__);  \
                break;                                                                             \
            case DataType::DoubleComplex:                                                          \
                FUNC<std::complex<double>, std::complex<double>, std::complex<double>>(            \
                    __VA_ARGS__);                                                                  \
                break;                                                                             \
            case DataType::Int8:                                                                   \
                FUNC<int8_t, int8_t, int8_t>(__VA_ARGS__);                                         \
                break;                                                                             \
            case DataType::Int16:                                                                  \
                FUNC<int16_t, int16_t, int16_t>(__VA_ARGS__);                                      \
                break;                                                                             \
            case DataType::UInt8:                                                                  \
                FUNC<uint8_t, uint8_t, uint8_t>(__VA_ARGS__);                                      \
                break;                                                                             \
            case DataType::UInt16:                                                                 \
                FUNC<uint16_t, uint16_t, uint16_t>(__VA_ARGS__);                                   \
                break;                                                                             \
            case DataType::UInt32:                                                                 \
                FUNC<uint32_t, uint32_t, uint32_t>(__VA_ARGS__);                                   \
                break;                                                                             \
            case DataType::UInt64:                                                                 \
                FUNC<uint64_t, uint64_t, uint64_t>(__VA_ARGS__);                                   \
                break;                                                                             \
            default:                                                                               \
                helper::Throw<std::invalid_argument>("Derived", "Function", "dispatch",            \
                                                     "Unsupported type");                          \
                break;                                                                             \
            }                                                                                      \
        }                                                                                          \
        else if (outType == DataType::Double && lhsType == DataType::Float)                        \
            FUNC<double, float, double>(__VA_ARGS__);                                              \
        else if (outType == DataType::Double && rhsType == DataType::Float)                        \
            FUNC<double, double, float>(__VA_ARGS__);                                              \
        else if (outType == DataType::Double && lhsType == DataType::Int32)                        \
            FUNC<double, int32_t, double>(__VA_ARGS__);                                            \
        else if (outType == DataType::Double && rhsType == DataType::Int32)                        \
            FUNC<double, double, int32_t>(__VA_ARGS__);                                            \
        else if (outType == DataType::Double && lhsType == DataType::Int64)                        \
            FUNC<double, int64_t, double>(__VA_ARGS__);                                            \
        else if (outType == DataType::Double && rhsType == DataType::Int64)                        \
            FUNC<double, double, int64_t>(__VA_ARGS__);                                            \
        else if (outType == DataType::Float && lhsType == DataType::Int32)                         \
            FUNC<float, int32_t, float>(__VA_ARGS__);                                              \
        else if (outType == DataType::Float && rhsType == DataType::Int32)                         \
            FUNC<float, float, int32_t>(__VA_ARGS__);                                              \
        else                                                                                       \
            helper::Throw<std::invalid_argument>(                                                  \
                "Derived", "Function", "dispatch",                                                 \
                "Unsupported type combination — use PROMOTE instruction");                         \
    } while (0)

// Float-only type dispatch (for sqrt, pow, trig — not valid on integer or complex)
#define DISPATCH_FLOAT_TYPE(dtype, FUNC, ...)                                                      \
    do                                                                                             \
    {                                                                                              \
        switch (dtype)                                                                             \
        {                                                                                          \
        case DataType::Float:                                                                      \
            FUNC<float>(__VA_ARGS__);                                                              \
            break;                                                                                 \
        case DataType::Double:                                                                     \
            FUNC<double>(__VA_ARGS__);                                                             \
            break;                                                                                 \
        case DataType::LongDouble:                                                                 \
            FUNC<long double>(__VA_ARGS__);                                                        \
            break;                                                                                 \
        default:                                                                                   \
            helper::Throw<std::invalid_argument>("Derived", "Function", "dispatch",                \
                                                 "Operation requires floating-point type");        \
        }                                                                                          \
    } while (0)

// Full type dispatch for homogeneous operations: calls Func<T,T,T>(args...) for binary ops
// or Func<T>(args...) for unary ops.
#define DISPATCH_TYPE(dtype, FUNC, ...)                                                            \
    do                                                                                             \
    {                                                                                              \
        switch (dtype)                                                                             \
        {                                                                                          \
        case DataType::Float:                                                                      \
            FUNC<float>(__VA_ARGS__);                                                              \
            break;                                                                                 \
        case DataType::Double:                                                                     \
            FUNC<double>(__VA_ARGS__);                                                             \
            break;                                                                                 \
        case DataType::LongDouble:                                                                 \
            FUNC<long double>(__VA_ARGS__);                                                        \
            break;                                                                                 \
        case DataType::Int8:                                                                       \
            FUNC<int8_t>(__VA_ARGS__);                                                             \
            break;                                                                                 \
        case DataType::Int16:                                                                      \
            FUNC<int16_t>(__VA_ARGS__);                                                            \
            break;                                                                                 \
        case DataType::Int32:                                                                      \
            FUNC<int32_t>(__VA_ARGS__);                                                            \
            break;                                                                                 \
        case DataType::Int64:                                                                      \
            FUNC<int64_t>(__VA_ARGS__);                                                            \
            break;                                                                                 \
        case DataType::UInt8:                                                                      \
            FUNC<uint8_t>(__VA_ARGS__);                                                            \
            break;                                                                                 \
        case DataType::UInt16:                                                                     \
            FUNC<uint16_t>(__VA_ARGS__);                                                           \
            break;                                                                                 \
        case DataType::UInt32:                                                                     \
            FUNC<uint32_t>(__VA_ARGS__);                                                           \
            break;                                                                                 \
        case DataType::UInt64:                                                                     \
            FUNC<uint64_t>(__VA_ARGS__);                                                           \
            break;                                                                                 \
        case DataType::FloatComplex:                                                               \
            FUNC<std::complex<float>>(__VA_ARGS__);                                                \
            break;                                                                                 \
        case DataType::DoubleComplex:                                                              \
            FUNC<std::complex<double>>(__VA_ARGS__);                                               \
            break;                                                                                 \
        default:                                                                                   \
            helper::Throw<std::invalid_argument>("Derived", "Function", "dispatch",                \
                                                 "Unsupported type");                              \
        }                                                                                          \
    } while (0)

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

DerivedData AddFunc(const ExprData &exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::AddFunc");
    auto &inputData = exprData.Data;
    auto type = exprData.OutType;
    if (inputData.size() == 1)
        return AddAggregatedFunc(exprData.Output, inputData[0], type);

    size_t N = exprData.OutputSize;
    // First two inputs
    DISPATCH_BINARY(type, inputData[0].Type, inputData[1].Type, detail::DoAdd, exprData.Output,
                    inputData[0], inputData[1], N);
    // Accumulate additional inputs (n-ary sum)
    for (size_t k = 2; k < inputData.size(); k++)
    {
        DerivedData outAsDerived = {exprData.Output, {}, {}, type};
        DISPATCH_BINARY(type, type, inputData[k].Type, detail::DoAdd, exprData.Output, outAsDerived,
                        inputData[k], N);
    }
    return DerivedData({exprData.Output});
}

DerivedData SubtractFunc(const ExprData &exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::SubtractFunc");
    auto &inputData = exprData.Data;
    auto type = exprData.OutType;
    size_t N = exprData.OutputSize;
    if (inputData.size() == 2)
    {
        DISPATCH_BINARY(type, inputData[0].Type, inputData[1].Type, detail::DoSub, exprData.Output,
                        inputData[0], inputData[1], N);
    }
    else
    {
        // subtract(x, y, z) = x - y - z: first add y+z+..., then subtract from x
        DISPATCH_BINARY(type, inputData[1].Type, inputData[2].Type, detail::DoAdd, exprData.Output,
                        inputData[1], inputData[2], N);
        for (size_t k = 3; k < inputData.size(); k++)
        {
            DerivedData outAsDerived = {exprData.Output, {}, {}, type};
            DISPATCH_BINARY(type, type, inputData[k].Type, detail::DoAdd, exprData.Output,
                            outAsDerived, inputData[k], N);
        }
        DerivedData outAsDerived = {exprData.Output, {}, {}, type};
        DISPATCH_BINARY(type, inputData[0].Type, type, detail::DoSub, exprData.Output, inputData[0],
                        outAsDerived, N);
    }
    return DerivedData({exprData.Output});
}

DerivedData NegateFunc(const ExprData &exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::NegateFunc");
    DISPATCH_TYPE(exprData.OutType, detail::DoNegate, exprData.Output, exprData.Data[0].Data,
                  exprData.OutputSize);
    return DerivedData({exprData.Output});
}

DerivedData MultFunc(const ExprData &exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::MultFunc");
    auto &inputData = exprData.Data;
    auto type = exprData.OutType;
    size_t N = exprData.OutputSize;
    DISPATCH_BINARY(type, inputData[0].Type, inputData[1].Type, detail::DoMul, exprData.Output,
                    inputData[0], inputData[1], N);
    for (size_t k = 2; k < inputData.size(); k++)
    {
        DerivedData outAsDerived = {exprData.Output, {}, {}, type};
        DISPATCH_BINARY(type, type, inputData[k].Type, detail::DoMul, exprData.Output, outAsDerived,
                        inputData[k], N);
    }
    return DerivedData({exprData.Output});
}

DerivedData DivFunc(const ExprData &exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::DivFunc");
    auto &inputData = exprData.Data;
    auto type = exprData.OutType;
    size_t N = exprData.OutputSize;
    if (inputData.size() == 2)
    {
        DISPATCH_BINARY(type, inputData[0].Type, inputData[1].Type, detail::DoDiv, exprData.Output,
                        inputData[0], inputData[1], N);
    }
    else
    {
        // divide(x, y, z) = x / y / z: multiply y*z*..., then divide x by result
        DISPATCH_BINARY(type, inputData[1].Type, inputData[2].Type, detail::DoMul, exprData.Output,
                        inputData[1], inputData[2], N);
        for (size_t k = 3; k < inputData.size(); k++)
        {
            DerivedData outAsDerived = {exprData.Output, {}, {}, type};
            DISPATCH_BINARY(type, type, inputData[k].Type, detail::DoMul, exprData.Output,
                            outAsDerived, inputData[k], N);
        }
        DerivedData outAsDerived = {exprData.Output, {}, {}, type};
        DISPATCH_BINARY(type, inputData[0].Type, type, detail::DoDiv, exprData.Output, inputData[0],
                        outAsDerived, N);
    }
    return DerivedData({exprData.Output});
}

DerivedData SqrtFunc(const ExprData &exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::SqrtFunc");
    DISPATCH_FLOAT_TYPE(exprData.OutType, detail::DoSqrt, exprData.Output, exprData.Data[0].Data,
                        exprData.OutputSize);
    return DerivedData({exprData.Output});
}

DerivedData PowFunc(const ExprData &exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::PowFunc");
    auto &inputData = exprData.Data;
    size_t N = exprData.OutputSize;
    if (inputData.size() == 1)
    {
        // Default: squaring
        DISPATCH_BINARY(exprData.OutType, inputData[0].Type, inputData[0].Type, detail::DoMul,
                        exprData.Output, inputData[0], inputData[0], N);
    }
    else
    {
        DISPATCH_FLOAT_TYPE(exprData.OutType, detail::DoPow, exprData.Output, inputData[0],
                            inputData[1], N);
    }
    return DerivedData({exprData.Output});
}

DerivedData SinFunc(const ExprData &exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::SinFunc");
    DISPATCH_FLOAT_TYPE(exprData.OutType, detail::DoSin, exprData.Output, exprData.Data[0].Data,
                        exprData.OutputSize);
    return DerivedData({exprData.Output});
}

DerivedData CosFunc(const ExprData &exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::CosFunc");
    DISPATCH_FLOAT_TYPE(exprData.OutType, detail::DoCos, exprData.Output, exprData.Data[0].Data,
                        exprData.OutputSize);
    return DerivedData({exprData.Output});
}

DerivedData TanFunc(const ExprData &exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::TanFunc");
    DISPATCH_FLOAT_TYPE(exprData.OutType, detail::DoTan, exprData.Output, exprData.Data[0].Data,
                        exprData.OutputSize);
    return DerivedData({exprData.Output});
}

DerivedData AsinFunc(const ExprData &exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::AsinFunc");
    DISPATCH_FLOAT_TYPE(exprData.OutType, detail::DoAsin, exprData.Output, exprData.Data[0].Data,
                        exprData.OutputSize);
    return DerivedData({exprData.Output});
}

DerivedData AcosFunc(const ExprData &exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::AcosFunc");
    DISPATCH_FLOAT_TYPE(exprData.OutType, detail::DoAcos, exprData.Output, exprData.Data[0].Data,
                        exprData.OutputSize);
    return DerivedData({exprData.Output});
}

DerivedData AtanFunc(const ExprData &exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::AtanFunc");
    DISPATCH_FLOAT_TYPE(exprData.OutType, detail::DoAtan, exprData.Output, exprData.Data[0].Data,
                        exprData.OutputSize);
    return DerivedData({exprData.Output});
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

DerivedData MagnitudeFunc(const ExprData &exprData)
{
    PERFSTUBS_SCOPED_TIMER("derived::Function::MagnitudeFunc");
    auto &inputData = exprData.Data;
    auto type = exprData.OutType;
    if (inputData.size() == 1)
        return MagAggregatedFunc(exprData.Output, inputData[0], type);

    size_t N = exprData.OutputSize;
    if (inputData.size() == 3 && !inputData[0].IsScalar && !inputData[1].IsScalar &&
        !inputData[2].IsScalar)
    {
        // Fast path for 3-component magnitude
        DISPATCH_FLOAT_TYPE(type, detail::DoMagnitude3, exprData.Output, inputData[0].Data,
                            inputData[1].Data, inputData[2].Data, N);
    }
    else
    {
        // General N-component magnitude: out[i] = sqrt(sum of comp[k][i]^2)
        DISPATCH_FLOAT_TYPE(type, detail::DoMagnitudeN, exprData.Output, inputData, N);
    }
    return DerivedData({exprData.Output});
}

DerivedData Cross3DFunc(const ExprData &exprData)
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
DerivedData Curl3DFunc(const ExprData &exprData)
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

void PromoteArray(DataType outType, DataType inType, void *out, void *in, size_t N)
{
#define PROMOTE_CASE(OT, OC, IT, IC)                                                               \
    if (outType == OT && inType == IT)                                                             \
    {                                                                                              \
        detail::DoPromoteTyped<OC, IC>(out, in, N);                                                \
        return;                                                                                    \
    }
    PROMOTE_CASE(DataType::Double, double, DataType::Float, float)
    PROMOTE_CASE(DataType::Double, double, DataType::Int32, int32_t)
    PROMOTE_CASE(DataType::Double, double, DataType::Int64, int64_t)
    PROMOTE_CASE(DataType::Double, double, DataType::Int16, int16_t)
    PROMOTE_CASE(DataType::Double, double, DataType::Int8, int8_t)
    PROMOTE_CASE(DataType::Double, double, DataType::UInt8, uint8_t)
    PROMOTE_CASE(DataType::Double, double, DataType::UInt16, uint16_t)
    PROMOTE_CASE(DataType::Double, double, DataType::UInt32, uint32_t)
    PROMOTE_CASE(DataType::Double, double, DataType::UInt64, uint64_t)
    PROMOTE_CASE(DataType::Float, float, DataType::Int32, int32_t)
    PROMOTE_CASE(DataType::Float, float, DataType::Int16, int16_t)
    PROMOTE_CASE(DataType::Float, float, DataType::Int8, int8_t)
    PROMOTE_CASE(DataType::Float, float, DataType::UInt8, uint8_t)
    PROMOTE_CASE(DataType::Float, float, DataType::UInt16, uint16_t)
    PROMOTE_CASE(DataType::Float, float, DataType::Int64, int64_t)
    PROMOTE_CASE(DataType::Float, float, DataType::UInt32, uint32_t)
    PROMOTE_CASE(DataType::LongDouble, long double, DataType::Double, double)
    PROMOTE_CASE(DataType::LongDouble, long double, DataType::Float, float)
    PROMOTE_CASE(DataType::Int64, int64_t, DataType::Int32, int32_t)
    PROMOTE_CASE(DataType::Int64, int64_t, DataType::Int16, int16_t)
    PROMOTE_CASE(DataType::Int64, int64_t, DataType::Int8, int8_t)
    PROMOTE_CASE(DataType::Int32, int32_t, DataType::Int16, int16_t)
    PROMOTE_CASE(DataType::Int32, int32_t, DataType::Int8, int8_t)
    PROMOTE_CASE(DataType::DoubleComplex, std::complex<double>, DataType::FloatComplex,
                 std::complex<float>)
#undef PROMOTE_CASE
    helper::Throw<std::invalid_argument>("Derived", "Function", "PromoteArray",
                                         "Unsupported type promotion");
}

}
} // namespace adios2
#endif
