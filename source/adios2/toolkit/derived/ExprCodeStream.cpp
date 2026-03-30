/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_DERIVED_ExprCodeStream_CPP_
#define ADIOS2_DERIVED_ExprCodeStream_CPP_

#include "ExprCodeStream.h"
#include "Function.h"
#include "adios2/common/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h"
#include "adios2/helper/adiosLog.h"

#include <algorithm>
#include <cstring>
#include <functional>
#include <numeric>
#include <set>
#include <sstream>

namespace adios2
{
namespace derived
{

// --- Operator dispatch tables (moved from Expression.cpp) ---

struct OperatorFunctions
{
    std::function<DerivedData(ExprData)> ComputeFct;
    std::function<std::tuple<Dims, Dims, Dims>(std::vector<std::tuple<Dims, Dims, Dims>>)> DimsFct;
    std::function<DataType(DataType)> TypeFct;
};

static const std::map<detail::ExpressionOperator, OperatorFunctions> OpFunctions = {
    {detail::ExpressionOperator::OP_ADD, {AddFunc, SameDimsWithAgrFunc, SameTypeFunc}},
    {detail::ExpressionOperator::OP_SUBTRACT, {SubtractFunc, SameDimsFunc, SameTypeFunc}},
    {detail::ExpressionOperator::OP_NEGATE, {NegateFunc, SameDimsFunc, SameTypeFunc}},
    {detail::ExpressionOperator::OP_MULT, {MultFunc, SameDimsFunc, SameTypeFunc}},
    {detail::ExpressionOperator::OP_DIV, {DivFunc, SameDimsFunc, SameTypeFunc}},
    {detail::ExpressionOperator::OP_POW, {PowFunc, SameDimsFunc, FloatTypeFunc}},
    {detail::ExpressionOperator::OP_SQRT, {SqrtFunc, SameDimsFunc, FloatTypeFunc}},
    {detail::ExpressionOperator::OP_SIN, {SinFunc, SameDimsFunc, FloatTypeFunc}},
    {detail::ExpressionOperator::OP_COS, {CosFunc, SameDimsFunc, FloatTypeFunc}},
    {detail::ExpressionOperator::OP_TAN, {TanFunc, SameDimsFunc, FloatTypeFunc}},
    {detail::ExpressionOperator::OP_ASIN, {AsinFunc, SameDimsFunc, FloatTypeFunc}},
    {detail::ExpressionOperator::OP_ACOS, {AcosFunc, SameDimsFunc, FloatTypeFunc}},
    {detail::ExpressionOperator::OP_ATAN, {AtanFunc, SameDimsFunc, FloatTypeFunc}},
    {detail::ExpressionOperator::OP_MAGN, {MagnitudeFunc, SameDimsWithAgrFunc, SameTypeFunc}},
    {detail::ExpressionOperator::OP_CROSS, {Cross3DFunc, Cross3DDimsFunc, SameTypeFunc}},
    {detail::ExpressionOperator::OP_CURL, {Curl3DFunc, CurlDimsFunc, SameTypeFunc}}};

// --- ExprNode utility functions ---

std::vector<std::string> VariableNameList(const ExprNode &node)
{
    std::vector<std::string> result;
    if (node.IsVar())
    {
        result.push_back(node.VarName);
    }
    else
    {
        for (const auto &child : node.Children)
        {
            auto childVars = VariableNameList(child);
            result.insert(result.end(), childVars.begin(), childVars.end());
        }
    }
    return result;
}

static const std::map<detail::ExpressionOperator, std::string> OpNameMap = {
    {detail::ExpressionOperator::OP_NULL, "NULL"},
    {detail::ExpressionOperator::OP_ADD, "ADD"},
    {detail::ExpressionOperator::OP_SUBTRACT, "SUBTRACT"},
    {detail::ExpressionOperator::OP_NEGATE, "NEGATE"},
    {detail::ExpressionOperator::OP_MULT, "MULT"},
    {detail::ExpressionOperator::OP_DIV, "DIV"},
    {detail::ExpressionOperator::OP_SQRT, "SQRT"},
    {detail::ExpressionOperator::OP_POW, "POW"},
    {detail::ExpressionOperator::OP_SIN, "SIN"},
    {detail::ExpressionOperator::OP_COS, "COS"},
    {detail::ExpressionOperator::OP_TAN, "TAN"},
    {detail::ExpressionOperator::OP_ASIN, "ASIN"},
    {detail::ExpressionOperator::OP_ACOS, "ACOS"},
    {detail::ExpressionOperator::OP_ATAN, "ATAN"},
    {detail::ExpressionOperator::OP_MAGN, "MAGNITUDE"},
    {detail::ExpressionOperator::OP_CROSS, "CROSS"},
    {detail::ExpressionOperator::OP_CURL, "CURL"}};

std::string ToStringExpr(const ExprNode &node)
{
    if (node.IsVar())
        return "{" + node.VarName + "}";
    if (node.IsConst())
        return node.Const;

    auto it = OpNameMap.find(node.Op);
    std::string opName = (it != OpNameMap.end()) ? it->second : "UNKNOWN";

    std::string result = opName + "(";
    for (size_t i = 0; i < node.Children.size(); i++)
    {
        if (i > 0)
            result += ",";
        result += ToStringExpr(node.Children[i]);
    }
    result += ")";
    return result;
}

static bool IsIntegerType(DataType type)
{
    return type == DataType::Int8 || type == DataType::Int16 || type == DataType::Int32 ||
           type == DataType::Int64 || type == DataType::UInt8 || type == DataType::UInt16 ||
           type == DataType::UInt32 || type == DataType::UInt64;
}

// --- DumpCodeStream ---

static std::string OpName(detail::ExpressionOperator op)
{
    auto it = OpNameMap.find(op);
    return (it != OpNameMap.end()) ? it->second : "???";
}

std::string DumpCodeStream(const ExprCodeStream &cs)
{
    std::ostringstream os;
    os << "ExprCodeStream: \"" << cs.ExprString << "\"\n";
    os << "  OutputType: " << ToString(cs.OutputType) << "\n";
    os << "  OutputBuf: buf" << cs.OutputBufID << "\n";
    os << "  TempSlots: " << cs.NumTempSlots << "\n";

    os << "  Buffers:\n";
    for (size_t i = 0; i < cs.Buffers.size(); i++)
    {
        const auto &buf = cs.Buffers[i];
        os << "    buf" << i << ": ";
        if (buf.IsInput)
            os << "input \"" << buf.VarName << "\"";
        else if (buf.IsConstant)
        {
            os << "const ";
            if (buf.ConstVal.Resolved)
            {
                if (IsIntegerType(buf.Type))
                    os << buf.ConstVal.IntVal;
                else
                    os << buf.ConstVal.DoubleVal;
            }
            else
                os << buf.ConstVal.StringVal;
            os << " [slot " << buf.PhysicalSlot << "]";
        }
        else
            os << "temp [slot " << buf.PhysicalSlot << "]";
        if (buf.Type != DataType::None)
            os << " (" << ToString(buf.Type) << ")";
        os << "\n";
    }

    os << "  Instructions:\n";
    for (size_t i = 0; i < cs.Instructions.size(); i++)
    {
        const auto &instr = cs.Instructions[i];
        os << "    " << i << ": " << OpName(instr.Op) << " [";
        for (size_t j = 0; j < instr.InputBufs.size(); j++)
        {
            if (j > 0)
                os << ", ";
            os << "buf" << instr.InputBufs[j];
        }
        os << "] -> buf" << instr.OutputBuf << "\n";
    }

    return os.str();
}

// --- GenerateCode: ExprNode tree -> ExprCodeStream ---

struct GenerateCodeContext
{
    ExprCodeStream &cs;
    std::map<std::string, size_t> varBufMap; // dedup variable buffers
};

static size_t GenerateCodeNode(const ExprNode &node, GenerateCodeContext &ctx)
{
    if (node.IsVar())
    {
        // Deduplicate: same variable name reuses the same buffer
        auto it = ctx.varBufMap.find(node.VarName);
        if (it != ctx.varBufMap.end())
            return it->second;

        size_t bufID = ctx.cs.Buffers.size();
        BufferDescriptor buf;
        buf.IsInput = true;
        buf.VarName = node.VarName;
        ctx.cs.Buffers.push_back(buf);
        ctx.varBufMap[node.VarName] = bufID;

        // Track input var names (deduplicated)
        if (std::find(ctx.cs.InputVarNames.begin(), ctx.cs.InputVarNames.end(), node.VarName) ==
            ctx.cs.InputVarNames.end())
        {
            ctx.cs.InputVarNames.push_back(node.VarName);
        }
        return bufID;
    }

    if (node.IsConst())
    {
        size_t bufID = ctx.cs.Buffers.size();
        BufferDescriptor buf;
        buf.IsConstant = true;
        buf.ConstVal.StringVal = node.Const;
        ctx.cs.Buffers.push_back(buf);
        return bufID;
    }

    // Operator node: generate code for children first (post-order), then emit instruction
    std::vector<size_t> inputBufs;
    for (const auto &child : node.Children)
    {
        inputBufs.push_back(GenerateCodeNode(child, ctx));
    }

    // Create output buffer for this instruction
    size_t outBufID = ctx.cs.Buffers.size();
    BufferDescriptor outBuf;
    ctx.cs.Buffers.push_back(outBuf);

    // Create instruction
    ExprInstruction instr;
    instr.Op = node.Op;
    instr.InputBufs = inputBufs;
    instr.OutputBuf = outBufID;

    size_t instrIdx = ctx.cs.Instructions.size();
    ctx.cs.Instructions.push_back(instr);

    // Update buffer use ranges
    for (size_t bufID : inputBufs)
    {
        auto &buf = ctx.cs.Buffers[bufID];
        if (buf.FirstUse == 0 && buf.LastUse == 0 && instrIdx > 0)
            buf.FirstUse = instrIdx;
        buf.LastUse = instrIdx;
    }
    ctx.cs.Buffers[outBufID].FirstUse = instrIdx;
    ctx.cs.Buffers[outBufID].LastUse = instrIdx;

    return outBufID;
}

ExprCodeStream GenerateCode(const ExprNode &root)
{
    ExprCodeStream cs;
    GenerateCodeContext ctx{cs, {}};
    cs.OutputBufID = GenerateCodeNode(root, ctx);
    return cs;
}

// --- ResolveTypes ---

void ResolveTypes(ExprCodeStream &cs, const std::map<std::string, DataType> &varTypes)
{
    // Set types on input variable buffers
    for (auto &buf : cs.Buffers)
    {
        if (buf.IsInput)
        {
            auto it = varTypes.find(buf.VarName);
            if (it != varTypes.end())
                buf.Type = it->second;
        }
    }

    // Walk instructions in order
    for (auto &instr : cs.Instructions)
    {
        // Find the first non-constant input type
        DataType inputType = DataType::None;
        for (size_t bufID : instr.InputBufs)
        {
            auto &buf = cs.Buffers[bufID];
            if (!buf.IsConstant && buf.Type != DataType::None)
            {
                inputType = buf.Type;
                break;
            }
        }

        if (inputType == DataType::None)
            inputType = DataType::Double; // all-constant fallback

        // Resolve constant buffer types based on context
        for (size_t bufID : instr.InputBufs)
        {
            auto &buf = cs.Buffers[bufID];
            if (buf.IsConstant && !buf.ConstVal.Resolved)
            {
                buf.Type = inputType;
                buf.ConstVal.Type = inputType;

                // Parse the string value
                if (IsIntegerType(inputType))
                {
                    std::istringstream iss(buf.ConstVal.StringVal);
                    iss >> buf.ConstVal.IntVal;
                }
                else
                {
                    std::istringstream iss(buf.ConstVal.StringVal);
                    iss >> buf.ConstVal.DoubleVal;
                }
                buf.ConstVal.Resolved = true;
            }
        }

        // Apply TypeFct to get output type
        auto opIt = OpFunctions.find(instr.Op);
        if (opIt == OpFunctions.end())
        {
            helper::Throw<std::invalid_argument>("Derived", "ExprCodeStream", "ResolveTypes",
                                                 "Unknown operator in expression code stream");
        }
        instr.OutputType = opIt->second.TypeFct(inputType);
        cs.Buffers[instr.OutputBuf].Type = instr.OutputType;
    }

    cs.OutputType = cs.Buffers[cs.OutputBufID].Type;
}

// --- ConstantFold ---

// Evaluate a binary op on two typed constant values, produce a result TypedConstant
static TypedConstant EvalConstBinOp(detail::ExpressionOperator op, const TypedConstant &lhs,
                                    const TypedConstant &rhs, DataType outType)
{
    TypedConstant result;
    result.Type = outType;
    result.Resolved = true;

    // Promote both to double for computation
    double a = IsIntegerType(lhs.Type) ? (double)lhs.IntVal : lhs.DoubleVal;
    double b = IsIntegerType(rhs.Type) ? (double)rhs.IntVal : rhs.DoubleVal;
    double r = 0;

    switch (op)
    {
    case detail::ExpressionOperator::OP_ADD:
        r = a + b;
        break;
    case detail::ExpressionOperator::OP_SUBTRACT:
        r = a - b;
        break;
    case detail::ExpressionOperator::OP_MULT:
        r = a * b;
        break;
    case detail::ExpressionOperator::OP_DIV:
        r = a / b;
        break;
    case detail::ExpressionOperator::OP_POW:
        r = std::pow(a, b);
        break;
    default:
        r = a; // fallback — shouldn't happen for binary ops
        break;
    }

    if (IsIntegerType(outType))
    {
        result.IntVal = (int64_t)r;
        result.DoubleVal = r;
    }
    else
    {
        result.DoubleVal = r;
        result.IntVal = (int64_t)r;
    }
    result.StringVal = std::to_string(result.DoubleVal);
    return result;
}

// Evaluate a unary op on a typed constant value
static TypedConstant EvalConstUnaryOp(detail::ExpressionOperator op, const TypedConstant &operand,
                                      DataType outType)
{
    TypedConstant result;
    result.Type = outType;
    result.Resolved = true;

    double a = IsIntegerType(operand.Type) ? (double)operand.IntVal : operand.DoubleVal;
    double r = 0;

    switch (op)
    {
    case detail::ExpressionOperator::OP_NEGATE:
        r = -a;
        break;
    case detail::ExpressionOperator::OP_SQRT:
        r = std::sqrt(a);
        break;
    case detail::ExpressionOperator::OP_SIN:
        r = std::sin(a);
        break;
    case detail::ExpressionOperator::OP_COS:
        r = std::cos(a);
        break;
    case detail::ExpressionOperator::OP_TAN:
        r = std::tan(a);
        break;
    case detail::ExpressionOperator::OP_ASIN:
        r = std::asin(a);
        break;
    case detail::ExpressionOperator::OP_ACOS:
        r = std::acos(a);
        break;
    case detail::ExpressionOperator::OP_ATAN:
        r = std::atan(a);
        break;
    default:
        r = a;
        break;
    }

    if (IsIntegerType(outType))
    {
        result.IntVal = (int64_t)r;
        result.DoubleVal = r;
    }
    else
    {
        result.DoubleVal = r;
        result.IntVal = (int64_t)r;
    }
    result.StringVal = std::to_string(result.DoubleVal);
    return result;
}

void ConstantFold(ExprCodeStream &cs)
{
    // Iterate until no more folding is possible
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (size_t i = 0; i < cs.Instructions.size(); i++)
        {
            auto &instr = cs.Instructions[i];

            // Check if all inputs are constants
            bool allConst = true;
            for (size_t bufID : instr.InputBufs)
            {
                if (!cs.Buffers[bufID].IsConstant)
                {
                    allConst = false;
                    break;
                }
            }
            if (!allConst)
                continue;

            // Evaluate the constant expression
            TypedConstant result;
            if (instr.InputBufs.size() == 1)
            {
                result = EvalConstUnaryOp(instr.Op, cs.Buffers[instr.InputBufs[0]].ConstVal,
                                          instr.OutputType);
            }
            else if (instr.InputBufs.size() == 2)
            {
                result = EvalConstBinOp(instr.Op, cs.Buffers[instr.InputBufs[0]].ConstVal,
                                        cs.Buffers[instr.InputBufs[1]].ConstVal, instr.OutputType);
            }
            else
            {
                // N-ary (e.g. add(1, 2, 3)) — fold left-to-right
                result = cs.Buffers[instr.InputBufs[0]].ConstVal;
                for (size_t j = 1; j < instr.InputBufs.size(); j++)
                {
                    result =
                        EvalConstBinOp(instr.Op, result, cs.Buffers[instr.InputBufs[j]].ConstVal,
                                       instr.OutputType);
                }
            }

            // Convert the output buffer to a constant buffer
            auto &outBuf = cs.Buffers[instr.OutputBuf];
            outBuf.IsConstant = true;
            outBuf.ConstVal = result;
            outBuf.Type = instr.OutputType;

            // Remove the instruction
            cs.Instructions.erase(cs.Instructions.begin() + i);
            i--; // re-check this index

            // Update FirstUse/LastUse for remaining instructions
            for (size_t j = 0; j < cs.Instructions.size(); j++)
            {
                for (size_t bufID : cs.Instructions[j].InputBufs)
                {
                    auto &buf = cs.Buffers[bufID];
                    if (buf.FirstUse > j)
                        buf.FirstUse = j;
                    buf.LastUse = j;
                }
                cs.Buffers[cs.Instructions[j].OutputBuf].FirstUse = j;
                cs.Buffers[cs.Instructions[j].OutputBuf].LastUse = j;
            }

            changed = true;
        }
    }

    // Mark unreferenced buffers as dead (clear IsConstant so they're not materialized)
    std::set<size_t> referenced;
    for (const auto &instr : cs.Instructions)
    {
        for (size_t bufID : instr.InputBufs)
            referenced.insert(bufID);
        referenced.insert(instr.OutputBuf);
    }
    referenced.insert(cs.OutputBufID);
    for (size_t i = 0; i < cs.Buffers.size(); i++)
    {
        if (!referenced.count(i) && !cs.Buffers[i].IsInput)
            cs.Buffers[i].IsConstant = false;
    }

    // Update OutputType in case the entire expression was folded
    cs.OutputType = cs.Buffers[cs.OutputBufID].Type;
}

// --- PlanBuffers (greedy slot reuse) ---

void PlanBuffers(ExprCodeStream &cs)
{
    // Input buffers don't use the pool — they point to caller data.
    // Constant and temp buffers share a pool indexed by PhysicalSlot.
    // Greedy reuse: scan instructions, release slots after LastUse,
    // reuse freed slots. Output slot must not alias any input slot
    // of the same instruction.

    std::vector<bool> slotFree;
    size_t numPoolSlots = 0;

    auto allocSlot = [&]() -> size_t {
        for (size_t s = 0; s < slotFree.size(); s++)
        {
            if (slotFree[s])
            {
                slotFree[s] = false;
                return s;
            }
        }
        slotFree.push_back(false);
        return numPoolSlots++;
    };

    // Input buffers don't use the pool (caller data).
    // Constant buffers get their own dedicated pool slots (not reused).
    for (size_t i = 0; i < cs.Buffers.size(); i++)
    {
        if (cs.Buffers[i].IsConstant)
            cs.Buffers[i].PhysicalSlot = allocSlot();
    }

    // Assign slots for temp buffers via greedy reuse
    for (size_t instrIdx = 0; instrIdx < cs.Instructions.size(); instrIdx++)
    {
        auto &instr = cs.Instructions[instrIdx];
        size_t outBuf = instr.OutputBuf;
        auto &outDesc = cs.Buffers[outBuf];

        // Skip non-temp buffers
        if (outDesc.IsInput || outDesc.IsConstant)
            continue;

        // Release input buffers whose LastUse is this instruction BEFORE allocating
        // output, so their slots become available — but collect which slots they used
        // to avoid aliasing with the output
        std::set<size_t> inputSlots;
        for (size_t bufID : instr.InputBufs)
        {
            auto &buf = cs.Buffers[bufID];
            if (!buf.IsInput && !buf.IsConstant)
                inputSlots.insert(buf.PhysicalSlot);
            if (!buf.IsInput && !buf.IsConstant && buf.LastUse == instrIdx)
            {
                slotFree[buf.PhysicalSlot] = true;
            }
        }

        // Allocate output slot, avoiding input slots of this instruction
        size_t slot = numPoolSlots; // fallback: new slot
        for (size_t s = 0; s < slotFree.size(); s++)
        {
            if (slotFree[s] && inputSlots.find(s) == inputSlots.end())
            {
                slot = s;
                slotFree[s] = false;
                break;
            }
        }
        if (slot == numPoolSlots)
        {
            slotFree.push_back(false);
            numPoolSlots++;
        }
        outDesc.PhysicalSlot = slot;
    }

    cs.NumTempSlots = numPoolSlots;
}

// --- GetDims ---

std::tuple<Dims, Dims, Dims>
GetDims(const ExprCodeStream &cs,
        const std::map<std::string, std::tuple<Dims, Dims, Dims>> &nameToDims)
{
    // Build a dims table indexed by buffer ID
    std::vector<std::tuple<Dims, Dims, Dims>> bufDims(cs.Buffers.size());

    // Find reference dims from first input variable
    std::tuple<Dims, Dims, Dims> refDims = {{}, {}, {}};
    for (const auto &buf : cs.Buffers)
    {
        if (buf.IsInput)
        {
            auto it = nameToDims.find(buf.VarName);
            if (it != nameToDims.end())
            {
                refDims = it->second;
                break;
            }
        }
    }

    // Set input buffer dims
    for (size_t i = 0; i < cs.Buffers.size(); i++)
    {
        const auto &buf = cs.Buffers[i];
        if (buf.IsInput)
        {
            auto it = nameToDims.find(buf.VarName);
            if (it != nameToDims.end())
                bufDims[i] = it->second;
        }
        else if (buf.IsConstant)
        {
            bufDims[i] = refDims; // broadcast
        }
    }

    // Walk instructions, compute output dims
    for (const auto &instr : cs.Instructions)
    {
        std::vector<std::tuple<Dims, Dims, Dims>> inputDims;
        for (size_t bufID : instr.InputBufs)
        {
            inputDims.push_back(bufDims[bufID]);
        }
        auto opIt = OpFunctions.find(instr.Op);
        bufDims[instr.OutputBuf] = opIt->second.DimsFct(inputDims);
    }

    return bufDims[cs.OutputBufID];
}

// --- Execute ---

// Buffer pool: slots grow when needed, never shrink. Reused across blocks.
struct BufferSlot
{
    void *Ptr = nullptr;
    size_t Capacity = 0; // bytes
};

static void *PoolAlloc(std::vector<BufferSlot> &pool, size_t slot, size_t bytes)
{
    if (slot >= pool.size())
        pool.resize(slot + 1);
    if (pool[slot].Capacity < bytes)
    {
        free(pool[slot].Ptr);
        pool[slot].Ptr = malloc(bytes);
        pool[slot].Capacity = bytes;
    }
    return pool[slot].Ptr;
}

static void PoolFreeAll(std::vector<BufferSlot> &pool)
{
    for (auto &slot : pool)
    {
        free(slot.Ptr);
        slot.Ptr = nullptr;
        slot.Capacity = 0;
    }
}

static size_t ElementSize(DataType type)
{
    size_t s = 0;
#define declare_type_elemsize(T)                                                                   \
    if (type == helper::GetDataType<T>())                                                          \
        s = sizeof(T);
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_elemsize)
    return s;
}

// Fill a pre-allocated buffer with a typed constant value
static void FillConstant(void *buf, const TypedConstant &c, size_t count, DataType type)
{
    bool isInt = IsIntegerType(type);
#define declare_type_fill(T)                                                                       \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        T *out = reinterpret_cast<T *>(buf);                                                       \
        T val = isInt ? (T)c.IntVal : (T)c.DoubleVal;                                              \
        for (size_t i = 0; i < count; i++)                                                         \
            out[i] = val;                                                                          \
        return;                                                                                    \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_fill)
}

std::vector<DerivedData> Execute(const ExprCodeStream &cs, size_t numBlocks,
                                 std::map<std::string, std::vector<DerivedData>> &nameToData)
{
    std::vector<DerivedData> outputData(numBlocks);
    std::vector<BufferSlot> pool;

    for (size_t blk = 0; blk < numBlocks; blk++)
    {
        std::vector<DerivedData> bufData(cs.Buffers.size());

        // Set up input buffers from caller data
        for (size_t i = 0; i < cs.Buffers.size(); i++)
        {
            if (cs.Buffers[i].IsInput)
                bufData[i] = nameToData[cs.Buffers[i].VarName][blk];
        }

        // Determine reference shape for constant broadcast
        size_t refDataSize = 0;
        Dims refStart, refCount;
        for (size_t i = 0; i < cs.Buffers.size(); i++)
        {
            if (cs.Buffers[i].IsInput)
            {
                refStart = bufData[i].Start;
                refCount = bufData[i].Count;
                refDataSize = std::accumulate(refCount.begin(), refCount.end(), (size_t)1,
                                              std::multiplies<size_t>());
                break;
            }
        }
        if (refDataSize == 0)
            refDataSize = 1;

        // Materialize constants into pool slots
        for (size_t i = 0; i < cs.Buffers.size(); i++)
        {
            const auto &buf = cs.Buffers[i];
            if (buf.IsConstant)
            {
                size_t bytes = refDataSize * ElementSize(buf.Type);
                void *ptr = PoolAlloc(pool, buf.PhysicalSlot, bytes);
                FillConstant(ptr, buf.ConstVal, refDataSize, buf.Type);
                bufData[i] = {ptr, refStart, refCount, buf.Type};
            }
        }

        // Execute instructions — allocate output from pool, compute writes directly into it
        for (size_t instrIdx = 0; instrIdx < cs.Instructions.size(); instrIdx++)
        {
            const auto &instr = cs.Instructions[instrIdx];

            std::vector<DerivedData> instrInputs;
            for (size_t bufID : instr.InputBufs)
                instrInputs.push_back(bufData[bufID]);

            // Compute output dims
            std::vector<std::tuple<Dims, Dims, Dims>> instrDims;
            for (size_t bufID : instr.InputBufs)
                instrDims.push_back(
                    {bufData[bufID].Start, bufData[bufID].Count, bufData[bufID].Count});

            auto opIt = OpFunctions.find(instr.Op);
            auto outDims = opIt->second.DimsFct(instrDims);
            Dims outCount = std::get<1>(outDims);
            size_t outSize = std::accumulate(outCount.begin(), outCount.end(), (size_t)1,
                                             std::multiplies<size_t>());

            // Allocate output from pool, pass to compute function
            size_t outBytes = outSize * ElementSize(instr.OutputType);
            size_t slot = cs.Buffers[instr.OutputBuf].PhysicalSlot;
            void *outBuf = PoolAlloc(pool, slot, outBytes);

            ExprData exprInputData({instrInputs, instr.OutputType, outBuf, outSize});
            opIt->second.ComputeFct(exprInputData);

            bufData[instr.OutputBuf] = {outBuf, std::get<0>(outDims), outCount, instr.OutputType};
        }

        // Copy final output out of pool for caller (caller will free it)
        DerivedData &finalBuf = bufData[cs.OutputBufID];
        size_t finalBytes = std::accumulate(finalBuf.Count.begin(), finalBuf.Count.end(), (size_t)1,
                                            std::multiplies<size_t>()) *
                            ElementSize(finalBuf.Type);
        void *callerBuf = malloc(finalBytes);
        memcpy(callerBuf, finalBuf.Data, finalBytes);
        outputData[blk] = {callerBuf, finalBuf.Start, finalBuf.Count, finalBuf.Type};
    }

    PoolFreeAll(pool);
    return outputData;
}

}
}
#endif
