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

// Operator dispatch table: compute function + dimension function per operator.
// Single source of truth for operator dispatch (PROMOTE handled separately in Execute).
struct OperatorFunctions
{
    std::function<DerivedData(const ExprData &)> ComputeFct;
    std::function<std::tuple<Dims, Dims, Dims>(std::vector<std::tuple<Dims, Dims, Dims>>)> DimsFct;
};

static const std::map<detail::ExpressionOperator, OperatorFunctions> OpFunctions = {
    {detail::ExpressionOperator::OP_ADD, {AddFunc, SameDimsWithAgrFunc}},
    {detail::ExpressionOperator::OP_SUBTRACT, {SubtractFunc, SameDimsFunc}},
    {detail::ExpressionOperator::OP_NEGATE, {NegateFunc, SameDimsFunc}},
    {detail::ExpressionOperator::OP_MULT, {MultFunc, SameDimsFunc}},
    {detail::ExpressionOperator::OP_DIV, {DivFunc, SameDimsFunc}},
    {detail::ExpressionOperator::OP_POW, {PowFunc, SameDimsFunc}},
    {detail::ExpressionOperator::OP_SQRT, {SqrtFunc, SameDimsFunc}},
    {detail::ExpressionOperator::OP_SIN, {SinFunc, SameDimsFunc}},
    {detail::ExpressionOperator::OP_COS, {CosFunc, SameDimsFunc}},
    {detail::ExpressionOperator::OP_TAN, {TanFunc, SameDimsFunc}},
    {detail::ExpressionOperator::OP_ASIN, {AsinFunc, SameDimsFunc}},
    {detail::ExpressionOperator::OP_ACOS, {AcosFunc, SameDimsFunc}},
    {detail::ExpressionOperator::OP_ATAN, {AtanFunc, SameDimsFunc}},
    {detail::ExpressionOperator::OP_MAGN, {MagnitudeFunc, SameDimsWithAgrFunc}},
    {detail::ExpressionOperator::OP_CROSS, {Cross3DFunc, Cross3DDimsFunc}},
    {detail::ExpressionOperator::OP_CURL, {Curl3DFunc, CurlDimsFunc}}};

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
    {detail::ExpressionOperator::OP_CURL, "CURL"},
    {detail::ExpressionOperator::OP_PROMOTE, "PROMOTE"}};

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

static bool IsUnsignedType(DataType type)
{
    return type == DataType::UInt8 || type == DataType::UInt16 || type == DataType::UInt32 ||
           type == DataType::UInt64;
}

static bool IsComplexType(DataType type)
{
    return type == DataType::FloatComplex || type == DataType::DoubleComplex;
}

static bool IsFloatOp(detail::ExpressionOperator op)
{
    return op == detail::ExpressionOperator::OP_SQRT || op == detail::ExpressionOperator::OP_POW ||
           op == detail::ExpressionOperator::OP_SIN || op == detail::ExpressionOperator::OP_COS ||
           op == detail::ExpressionOperator::OP_TAN || op == detail::ExpressionOperator::OP_ASIN ||
           op == detail::ExpressionOperator::OP_ACOS || op == detail::ExpressionOperator::OP_ATAN;
}

// C-style type promotion: return the wider of two types.
static DataType PromoteType(DataType a, DataType b)
{
    if (a == b)
        return a;
    if (helper::GetDataTypeSize(a) > helper::GetDataTypeSize(b))
        return a;
    if (helper::GetDataTypeSize(b) > helper::GetDataTypeSize(a))
        return b;
    // Same size: prefer float over int
    if (!IsIntegerType(a) && IsIntegerType(b))
        return a;
    if (!IsIntegerType(b) && IsIntegerType(a))
        return b;
    // Same size integers: unsigned wins (C promotion rule)
    if (IsUnsignedType(a))
        return a;
    if (IsUnsignedType(b))
        return b;
    return a;
}

// --- ResolveTreeTypes ---

static void ResolveTreeTypesNode(ExprNode &node, const std::map<std::string, DataType> &varTypes,
                                 DataType contextType)
{
    if (node.IsVar())
    {
        auto it = varTypes.find(node.VarName);
        if (it != varTypes.end())
            node.Type = it->second;
        else
            node.Type = DataType::Double; // unknown variable fallback
        return;
    }

    if (node.IsConst())
    {
        // Constants take the type of their context (parent's promoted type).
        // On first pass (contextType==None), stay None so we don't affect promotion.
        node.Type = contextType;
        return;
    }

    // First pass: resolve children (constants don't know context yet, use None)
    for (auto &child : node.Children)
        ResolveTreeTypesNode(child, varTypes, DataType::None);

    // Compute promoted type from non-constant children
    DataType promoted = DataType::None;
    bool hasComplex = false, hasNonComplex = false;
    for (auto &child : node.Children)
    {
        if (child.IsConst() && child.Type == DataType::None)
            continue; // unresolved constant, skip
        if (child.Type == DataType::None)
            continue;
        if (IsComplexType(child.Type))
            hasComplex = true;
        else
            hasNonComplex = true;
        if (promoted == DataType::None)
            promoted = child.Type;
        else
            promoted = PromoteType(promoted, child.Type);
    }
    if (promoted == DataType::None)
        promoted = DataType::Double;

    // For trig/sqrt/pow: integer promotes to double
    if (IsFloatOp(node.Op) && IsIntegerType(promoted))
        promoted = DataType::Double;

    // Validation
    if (hasComplex && hasNonComplex)
        helper::Throw<std::invalid_argument>("Derived", "ExprNode", "ResolveTreeTypes",
                                             "Cannot mix complex and non-complex types");
    if (IsFloatOp(node.Op) && hasComplex)
        helper::Throw<std::invalid_argument>("Derived", "ExprNode", "ResolveTreeTypes",
                                             "Transcendental/sqrt/pow not supported on complex");
    if (IsUnsignedType(promoted) && node.Op == detail::ExpressionOperator::OP_NEGATE)
        helper::Throw<std::invalid_argument>("Derived", "ExprNode", "ResolveTreeTypes",
                                             "Negate not valid on unsigned types");
    if (node.Op == detail::ExpressionOperator::OP_CROSS && node.Children.size() != 6)
        helper::Throw<std::invalid_argument>("Derived", "ExprNode", "ResolveTreeTypes",
                                             "Cross product requires exactly 6 inputs");
    if (node.Op == detail::ExpressionOperator::OP_CURL && node.Children.size() != 1 &&
        node.Children.size() != 3)
        helper::Throw<std::invalid_argument>("Derived", "ExprNode", "ResolveTreeTypes",
                                             "Curl requires 1 or 3 inputs");

    node.Type = promoted;

    // Second pass: re-resolve constants now that we know the promoted type
    for (auto &child : node.Children)
    {
        if (child.IsConst())
            child.Type = promoted;
    }
}

void ResolveTreeTypes(ExprNode &tree, const std::map<std::string, DataType> &varTypes)
{
    ResolveTreeTypesNode(tree, varTypes, DataType::None);
}

// --- HasHalo ---

bool HasHalo(const ExprCodeStream &cs)
{
    for (const auto &instr : cs.Instructions)
        if (instr.SelRule == SelectionRule::ExpandHalo)
            return true;
    return false;
}

// --- ComputeInputSelections ---

std::map<std::string, std::pair<Dims, Dims>>
ComputeInputSelections(const ExprCodeStream &cs, const Dims &outputStart, const Dims &outputCount)
{
    // Per-buffer selection: (start, count). Populated from output backward.
    std::vector<std::pair<Dims, Dims>> bufSel(cs.Buffers.size());
    std::vector<bool> bufSelSet(cs.Buffers.size(), false);

    // Seed with the requested output selection
    bufSel[cs.OutputBufID] = {outputStart, outputCount};
    bufSelSet[cs.OutputBufID] = true;

    // Walk instructions in reverse order
    for (int i = static_cast<int>(cs.Instructions.size()) - 1; i >= 0; i--)
    {
        const auto &instr = cs.Instructions[i];

        // If this instruction's output doesn't have a selection, skip
        if (!bufSelSet[instr.OutputBuf])
            continue;

        const auto &outStart = bufSel[instr.OutputBuf].first;
        const auto &outCount = bufSel[instr.OutputBuf].second;

        // Propagate selection to input buffers based on SelectionRule
        for (size_t bufID : instr.InputBufs)
        {
            if (cs.Buffers[bufID].IsConstant)
                continue; // constants don't need selection

            Dims inStart, inCount;
            switch (instr.SelRule)
            {
            case SelectionRule::Identity:
                // Element-wise: input selection = output selection
                inStart = outStart;
                inCount = outCount;
                break;

            case SelectionRule::ExpandHalo: {
                // Stencil ops (curl): output has trailing component dim (3).
                // Separated curl (3 inputs, each 3D): strip the component dim.
                // Aggregated curl (1 input, 4D [x,y,z,3]): keep the component dim
                // but use 0 (= full extent) so it picks up all components.
                size_t spatialDims = outStart.size();
                if (instr.Op == detail::ExpressionOperator::OP_CURL && spatialDims > 0 &&
                    (outCount.back() == 3 || outCount.back() == 0))
                    spatialDims--; // strip component dim from output

                inStart = Dims(outStart.begin(), outStart.begin() + spatialDims);
                inCount = Dims(outCount.begin(), outCount.begin() + spatialDims);

                // Expand each spatial dimension by the halo size
                for (size_t d = 0; d < spatialDims; d++)
                {
                    size_t halo = static_cast<size_t>(instr.HaloSize);
                    inStart[d] = (inStart[d] >= halo) ? inStart[d] - halo : 0;
                    inCount[d] = inCount[d] + 2 * halo;
                    // Clamp will happen at read time when actual array bounds are known
                }

                if (instr.InputBufs.size() == 1 && instr.Op == detail::ExpressionOperator::OP_CURL)
                {
                    // Aggregated curl: input is 4D [x,y,z,components].
                    // Add component dim back with 0 = full extent.
                    inStart.push_back(0);
                    inCount.push_back(0);
                }
                break;
            }

            case SelectionRule::Reshape:
                // Dimension-changing ops: reverse the dimension transformation.
                if (instr.Op == detail::ExpressionOperator::OP_CROSS)
                {
                    // Cross output has trailing dim of 3. Input doesn't.
                    inStart = Dims(outStart.begin(), outStart.end() - 1);
                    inCount = Dims(outCount.begin(), outCount.end() - 1);
                }
                else if (instr.Op == detail::ExpressionOperator::OP_MAGN &&
                         instr.InputBufs.size() == 1)
                {
                    // Aggregated magnitude: output removed last dim.
                    // Input needs spatial dims + full component dim.
                    // We don't know the component count here, so use 0
                    // to signal "read full extent" for that dimension.
                    inStart = outStart;
                    inStart.push_back(0);
                    inCount = outCount;
                    inCount.push_back(0); // 0 = full extent, resolved at read time
                }
                else if (instr.Op == detail::ExpressionOperator::OP_ADD &&
                         instr.InputBufs.size() == 1)
                {
                    // Aggregated add: output removed last dim.
                    // Input needs spatial dims + full last dim.
                    inStart = outStart;
                    inStart.push_back(0);
                    inCount = outCount;
                    inCount.push_back(0); // 0 = full extent, resolved at read time
                }
                else if (instr.Op == detail::ExpressionOperator::OP_CURL &&
                         instr.InputBufs.size() == 1)
                {
                    // Aggregated curl: output removed last dim and added 3.
                    // Input: spatial dims + full component dim.
                    inStart = Dims(outStart.begin(), outStart.end() - 1);
                    inStart.push_back(0);
                    inCount = Dims(outCount.begin(), outCount.end() - 1);
                    inCount.push_back(0);
                }
                else
                {
                    // Separated multi-input ops (magnitude, curl with 3 inputs):
                    // each input has same spatial dims as output (minus any trailing
                    // component dim)
                    if (!outCount.empty() && (outCount.back() == 3 || outCount.back() == 0) &&
                        (instr.Op == detail::ExpressionOperator::OP_CURL ||
                         instr.Op == detail::ExpressionOperator::OP_CROSS))
                    {
                        inStart = Dims(outStart.begin(), outStart.end() - 1);
                        inCount = Dims(outCount.begin(), outCount.end() - 1);
                    }
                    else
                    {
                        inStart = outStart;
                        inCount = outCount;
                    }
                }
                break;
            }

            // If this buffer already has a selection (from another instruction), take the union
            if (bufSelSet[bufID])
            {
                auto &existing = bufSel[bufID];
                for (size_t d = 0; d < inStart.size() && d < existing.first.size(); d++)
                {
                    size_t newEnd =
                        std::max(existing.first[d] + existing.second[d], inStart[d] + inCount[d]);
                    existing.first[d] = std::min(existing.first[d], inStart[d]);
                    existing.second[d] = newEnd - existing.first[d];
                }
            }
            else
            {
                bufSel[bufID] = {inStart, inCount};
                bufSelSet[bufID] = true;
            }
        }
    }

    // Collect selections for input variables
    std::map<std::string, std::pair<Dims, Dims>> result;
    for (size_t b = 0; b < cs.Buffers.size(); b++)
    {
        if (cs.Buffers[b].IsInput && bufSelSet[b])
        {
            result[cs.Buffers[b].VarName] = bufSel[b];
        }
    }
    return result;
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

static bool IsMixedTypeHandledInline(DataType outType, DataType lhsType, DataType rhsType);

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
        buf.Type = node.Type;
        ctx.cs.Buffers.push_back(buf);
        ctx.varBufMap[node.VarName] = bufID;

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
        buf.Type = node.Type;
        ctx.cs.Buffers.push_back(buf);
        return bufID;
    }

    // Operator node: generate code for children first (post-order)
    std::vector<size_t> inputBufs;
    for (const auto &child : node.Children)
    {
        size_t childBuf = GenerateCodeNode(child, ctx);

        // If child type doesn't match this node's type, may need PROMOTE.
        // Binary ops: skip if the combo is handled inline by DISPATCH_BINARY.
        // Unary ops: always promote if types differ.
        bool needsPromote = false;
        if (child.Type != DataType::None && child.Type != node.Type)
        {
            if (node.Children.size() == 2)
                needsPromote = !IsMixedTypeHandledInline(node.Type, child.Type, node.Type);
            else
                needsPromote = true;
        }
        if (needsPromote)
        {
            size_t promoteBufID = ctx.cs.Buffers.size();
            BufferDescriptor promoteBuf;
            promoteBuf.Type = node.Type;
            ctx.cs.Buffers.push_back(promoteBuf);

            ExprInstruction promoteInstr;
            promoteInstr.Op = detail::ExpressionOperator::OP_PROMOTE;
            promoteInstr.InputBufs = {childBuf};
            promoteInstr.OutputBuf = promoteBufID;
            promoteInstr.OutputType = node.Type;

            size_t promoteIdx = ctx.cs.Instructions.size();
            ctx.cs.Instructions.push_back(promoteInstr);
            ctx.cs.Buffers[childBuf].LastUse = promoteIdx;
            ctx.cs.Buffers[promoteBufID].FirstUse = promoteIdx;
            ctx.cs.Buffers[promoteBufID].LastUse = promoteIdx;

            childBuf = promoteBufID;
        }

        inputBufs.push_back(childBuf);
    }

    // Create output buffer for this instruction
    size_t outBufID = ctx.cs.Buffers.size();
    BufferDescriptor outBuf;
    outBuf.Type = node.Type;
    ctx.cs.Buffers.push_back(outBuf);

    // Create instruction
    ExprInstruction instr;
    instr.Op = node.Op;
    instr.InputBufs = inputBufs;
    instr.OutputBuf = outBufID;
    instr.OutputType = node.Type;

    size_t instrIdx = ctx.cs.Instructions.size();
    ctx.cs.Instructions.push_back(instr);

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
    cs.OutputType = root.Type;
    return cs;
}

// --- Constant evaluation helpers (used by SemanticsPass) ---

// Evaluate a binary op on two typed constant values, produce a result TypedConstant
static TypedConstant EvalConstBinOp(detail::ExpressionOperator op, const TypedConstant &lhs,
                                    const TypedConstant &rhs, DataType outType)
{
    TypedConstant result;
    result.Type = outType;
    result.Resolved = true;

    if (IsIntegerType(outType))
    {
        int64_t a = lhs.IntVal, b = rhs.IntVal, r = 0;
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
            r = (b != 0) ? a / b : 0;
            break;
        default:
            r = a;
            break;
        }
        result.IntVal = r;
        result.DoubleVal = static_cast<double>(r);
    }
    else
    {
        double a = IsIntegerType(lhs.Type) ? static_cast<double>(lhs.IntVal) : lhs.DoubleVal;
        double b = IsIntegerType(rhs.Type) ? static_cast<double>(rhs.IntVal) : rhs.DoubleVal;
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
            r = a;
            break;
        }
        result.DoubleVal = r;
        result.IntVal = static_cast<int64_t>(r);
    }
    result.StringVal = std::to_string(IsIntegerType(outType) ? static_cast<double>(result.IntVal)
                                                             : result.DoubleVal);
    return result;
}

// Evaluate a unary op on a typed constant value
static TypedConstant EvalConstUnaryOp(detail::ExpressionOperator op, const TypedConstant &operand,
                                      DataType outType)
{
    TypedConstant result;
    result.Type = outType;
    result.Resolved = true;

    if (IsIntegerType(outType) && op == detail::ExpressionOperator::OP_NEGATE)
    {
        result.IntVal = -operand.IntVal;
        result.DoubleVal = static_cast<double>(result.IntVal);
    }
    else
    {
        double a =
            IsIntegerType(operand.Type) ? static_cast<double>(operand.IntVal) : operand.DoubleVal;
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
        result.DoubleVal = r;
        result.IntVal = static_cast<int64_t>(r);
    }
    result.StringVal = std::to_string(result.DoubleVal);
    return result;
}

// --- SemanticsPass ---

// Returns true if DISPATCH_BINARY handles this type combination inline (no PROMOTE needed).
static bool IsMixedTypeHandledInline(DataType outType, DataType lhsType, DataType rhsType)
{
    if (lhsType == rhsType)
        return true; // homogeneous — always handled
    // Inline mixed combos in DISPATCH_BINARY:
    if (outType == DataType::Double &&
        (lhsType == DataType::Float || rhsType == DataType::Float || lhsType == DataType::Int32 ||
         rhsType == DataType::Int32 || lhsType == DataType::Int64 || rhsType == DataType::Int64))
        return true;
    if (outType == DataType::Float && (lhsType == DataType::Int32 || rhsType == DataType::Int32))
        return true;
    return false;
}

static SelectionRule SelectionRuleForOp(detail::ExpressionOperator op)
{
    switch (op)
    {
    case detail::ExpressionOperator::OP_CURL:
        return SelectionRule::ExpandHalo;
    case detail::ExpressionOperator::OP_CROSS:
    case detail::ExpressionOperator::OP_MAGN:
        return SelectionRule::Reshape;
    default:
        return SelectionRule::Identity;
    }
}

void SemanticsPass(ExprCodeStream &cs, const std::map<std::string, DataType> & /*varTypes*/)
{
    // Types, validation, and PROMOTE insertion are handled by ResolveTreeTypes + GenerateCode.
    // This pass handles: constant resolution, constant folding, strength reduction, selection
    // rules.

    for (size_t instrIdx = 0; instrIdx < cs.Instructions.size(); instrIdx++)
    {
        auto &instr = cs.Instructions[instrIdx];

        // Resolve constant buffer string values to numeric values
        for (size_t bufID : instr.InputBufs)
        {
            auto &buf = cs.Buffers[bufID];
            if (buf.IsConstant && !buf.ConstVal.Resolved)
            {
                buf.ConstVal.Type = buf.Type;
                if (IsIntegerType(buf.Type))
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

        // Set selection rule
        instr.SelRule = SelectionRuleForOp(instr.Op);
        if (instr.Op == detail::ExpressionOperator::OP_ADD && instr.InputBufs.size() == 1)
            instr.SelRule = SelectionRule::Reshape;
        if (instr.SelRule == SelectionRule::ExpandHalo)
            instr.HaloSize = 1;

        // Strength reduction: x^2 -> x*x
        if (instr.Op == detail::ExpressionOperator::OP_POW && instr.InputBufs.size() == 2)
        {
            auto &expBuf = cs.Buffers[instr.InputBufs[1]];
            if (expBuf.IsConstant && expBuf.ConstVal.Resolved && expBuf.ConstVal.DoubleVal == 2.0)
            {
                instr.Op = detail::ExpressionOperator::OP_MULT;
                instr.InputBufs = {instr.InputBufs[0], instr.InputBufs[0]};
                instr.OutputType = cs.Buffers[instr.InputBufs[0]].Type;
                cs.Buffers[instr.OutputBuf].Type = instr.OutputType;
            }
        }

        // Constant folding
        bool allConst = true;
        for (size_t bufID : instr.InputBufs)
        {
            if (!cs.Buffers[bufID].IsConstant)
            {
                allConst = false;
                break;
            }
        }
        if (allConst)
        {
            TypedConstant result;
            if (instr.InputBufs.size() == 1)
                result = EvalConstUnaryOp(instr.Op, cs.Buffers[instr.InputBufs[0]].ConstVal,
                                          instr.OutputType);
            else if (instr.InputBufs.size() == 2)
                result = EvalConstBinOp(instr.Op, cs.Buffers[instr.InputBufs[0]].ConstVal,
                                        cs.Buffers[instr.InputBufs[1]].ConstVal, instr.OutputType);
            else
            {
                result = cs.Buffers[instr.InputBufs[0]].ConstVal;
                for (size_t j = 1; j < instr.InputBufs.size(); j++)
                    result =
                        EvalConstBinOp(instr.Op, result, cs.Buffers[instr.InputBufs[j]].ConstVal,
                                       instr.OutputType);
            }

            auto &outBuf = cs.Buffers[instr.OutputBuf];
            outBuf.IsConstant = true;
            outBuf.ConstVal = result;
            outBuf.Type = instr.OutputType;

            cs.Instructions.erase(cs.Instructions.begin() + instrIdx);
            instrIdx--;
            continue;
        }
    }

    // Update use ranges and dead code elimination
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
        if (instr.Op == detail::ExpressionOperator::OP_PROMOTE)
        {
            bufDims[instr.OutputBuf] = inputDims[0]; // PROMOTE doesn't change dims
        }
        else
        {
            auto opIt = OpFunctions.find(instr.Op);
            bufDims[instr.OutputBuf] = opIt->second.DimsFct(inputDims);
        }
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

// Allocate a single-element buffer for a typed constant
static void *AllocScalarConstant(const TypedConstant &c, DataType type)
{
    size_t elemSize = ElementSize(type);
    void *buf = malloc(elemSize);
    bool isInt = IsIntegerType(type);
#define declare_type_scalar(T)                                                                     \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        *reinterpret_cast<T *>(buf) = isInt ? (T)c.IntVal : (T)c.DoubleVal;                        \
        return buf;                                                                                \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_scalar)
    return buf;
}

std::vector<DerivedData> Execute(const ExprCodeStream &cs, size_t numBlocks,
                                 std::map<std::string, std::vector<DerivedData>> &nameToData,
                                 const Dims &outputStart, const Dims &outputCount)
{
    bool hasOutputSelection = !outputCount.empty();
    std::vector<DerivedData> outputData(numBlocks);
    std::vector<BufferSlot> pool;

    // Allocate scalar constant buffers once (persist across blocks)
    std::vector<void *> constBufs(cs.Buffers.size(), nullptr);
    for (size_t i = 0; i < cs.Buffers.size(); i++)
    {
        if (cs.Buffers[i].IsConstant)
            constBufs[i] = AllocScalarConstant(cs.Buffers[i].ConstVal, cs.Buffers[i].Type);
    }

    for (size_t blk = 0; blk < numBlocks; blk++)
    {
        std::vector<DerivedData> bufData(cs.Buffers.size());

        // Set up input and constant buffers
        for (size_t i = 0; i < cs.Buffers.size(); i++)
        {
            const auto &buf = cs.Buffers[i];
            if (buf.IsInput)
                bufData[i] = nameToData[buf.VarName][blk];
            else if (buf.IsConstant)
                bufData[i] = {constBufs[i], {}, {}, buf.Type, true};
        }

        // Execute instructions
        for (size_t instrIdx = 0; instrIdx < cs.Instructions.size(); instrIdx++)
        {
            const auto &instr = cs.Instructions[instrIdx];

            // Gather inputs
            std::vector<DerivedData> instrInputs;
            instrInputs.reserve(instr.InputBufs.size());
            for (size_t bufID : instr.InputBufs)
                instrInputs.push_back(bufData[bufID]);

            // Compute output dims: element-wise ops use first non-scalar input dims.
            // Cross/curl/magnitude use DimsFct (called rarely).
            Dims outStart, outCount;
            if (instr.SelRule == SelectionRule::Identity)
            {
                // Element-wise: output dims = first non-scalar input
                for (size_t bufID : instr.InputBufs)
                {
                    if (!bufData[bufID].IsScalar)
                    {
                        outStart = bufData[bufID].Start;
                        outCount = bufData[bufID].Count;
                        break;
                    }
                }
            }
            else
            {
                // Dimension-changing ops: use DimsFct
                std::vector<std::tuple<Dims, Dims, Dims>> instrDims;
                Dims refStart, refCount;
                for (size_t bufID : instr.InputBufs)
                {
                    if (!bufData[bufID].IsScalar)
                    {
                        refStart = bufData[bufID].Start;
                        refCount = bufData[bufID].Count;
                        break;
                    }
                }
                for (size_t bufID : instr.InputBufs)
                {
                    if (bufData[bufID].IsScalar)
                        instrDims.push_back({refStart, refCount, refCount});
                    else
                        instrDims.push_back(
                            {bufData[bufID].Start, bufData[bufID].Count, bufData[bufID].Count});
                }
                auto opIt = OpFunctions.find(instr.Op);
                auto dims = opIt->second.DimsFct(instrDims);
                outStart = std::get<0>(dims);
                outCount = std::get<1>(dims);
            }

            bool isFinal = (instr.OutputBuf == cs.OutputBufID);

            size_t outSize = std::accumulate(outCount.begin(), outCount.end(), (size_t)1,
                                             std::multiplies<size_t>());

            // Allocate output: final instruction writes directly to caller buffer
            size_t outBytes = outSize * ElementSize(instr.OutputType);
            void *outBuf;
            if (isFinal)
                outBuf = malloc(outBytes);
            else
                outBuf = PoolAlloc(pool, cs.Buffers[instr.OutputBuf].PhysicalSlot, outBytes);

            const ExprData exprInputData({instrInputs, instr.OutputType, outBuf, outSize});
            if (instr.Op == detail::ExpressionOperator::OP_PROMOTE)
            {
                PromoteArray(instr.OutputType, instrInputs[0].Type, outBuf, instrInputs[0].Data,
                             outSize);
            }
            else
            {
                auto opIt = OpFunctions.find(instr.Op);
                if (opIt == OpFunctions.end())
                    helper::Throw<std::invalid_argument>("Derived", "ExprCodeStream", "Execute",
                                                         "Unknown operator");
                opIt->second.ComputeFct(exprInputData);
            }

            bufData[instr.OutputBuf] = {outBuf, outStart, outCount, instr.OutputType};
        }
        DerivedData &finalBuf = bufData[cs.OutputBufID];
        if (hasOutputSelection && finalBuf.Count != outputCount)
        {
            // Output is larger than requested (e.g. curl with halo). Trim to selection.
            size_t trimSize = std::accumulate(outputCount.begin(), outputCount.end(), (size_t)1,
                                              std::multiplies<size_t>());
            size_t trimBytes = trimSize * ElementSize(finalBuf.Type);
            void *trimBuf = malloc(trimBytes);
            helper::NdCopy((const char *)finalBuf.Data, finalBuf.Start, finalBuf.Count, true, true,
                           (char *)trimBuf, outputStart, outputCount, true, true,
                           ElementSize(finalBuf.Type), helper::CoreDims(), helper::CoreDims(),
                           helper::CoreDims(), helper::CoreDims(), false, MemorySpace::Host);
            free(finalBuf.Data);
            outputData[blk] = {trimBuf, outputStart, outputCount, finalBuf.Type};
        }
        else
        {
            outputData[blk] = {finalBuf.Data, finalBuf.Start, finalBuf.Count, finalBuf.Type};
        }
    }

    // Free scalar constants and pool
    for (auto ptr : constBufs)
        free(ptr);
    PoolFreeAll(pool);
    return outputData;
}

}
}
#endif
