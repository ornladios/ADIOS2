/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_DERIVED_ExprCodeStream_H_
#define ADIOS2_DERIVED_ExprCodeStream_H_

#include "DerivedData.h"
#include "ExprNode.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace adios2
{
namespace derived
{

struct TypedConstant
{
    std::string StringVal; // original string from parser
    double DoubleVal = 0;
    int64_t IntVal = 0;
    DataType Type = DataType::None;
    bool Resolved = false; // true after ResolveTypes
};

enum class SelectionRule
{
    Identity,   // element-wise: output region = input region
    ExpandHalo, // stencil op (curl): expand spatial dims by HaloSize
    Reshape     // dimension-changing op (cross, aggregated magnitude)
};

struct ExprInstruction
{
    detail::ExpressionOperator Op;
    std::vector<size_t> InputBufs; // buffer IDs for operands
    size_t OutputBuf;              // buffer ID for result
    DataType OutputType = DataType::None;
    SelectionRule SelRule = SelectionRule::Identity;
    int HaloSize = 0; // only used with ExpandHalo
};

struct BufferDescriptor
{
    bool IsInput = false;    // true = leaf variable data from caller
    bool IsConstant = false; // true = broadcast constant
    std::string VarName;     // only for IsInput
    TypedConstant ConstVal;  // only for IsConstant
    size_t FirstUse = 0;
    size_t LastUse = 0;
    size_t PhysicalSlot = 0; // assigned by PlanBuffers
    DataType Type = DataType::None;
};

struct ExprCodeStream
{
    std::vector<ExprInstruction> Instructions;
    std::vector<BufferDescriptor> Buffers;
    size_t OutputBufID = 0;
    size_t NumTempSlots = 0;
    DataType OutputType = DataType::None;
    std::vector<std::string> InputVarNames; // deduplicated
    std::string ExprString;                 // original expression for serialization
};

// Pipeline passes — free functions

/** Resolve types bottom-up on the expression tree. Sets Type on every node.
    Validates type combinations. Must be called before GenerateCode. */
void ResolveTreeTypes(ExprNode &tree, const std::map<std::string, DataType> &varTypes);

/** GenerateCode: convert ExprNode tree to linear ExprCodeStream. Tree must have types resolved.
    Emits PROMOTE instructions for mixed-type combinations not handled inline. */
ExprCodeStream GenerateCode(const ExprNode &root);

/** SemanticsPass: type propagation, constant resolution/folding, strength reduction,
    validation, selection rules. Replaces ResolveTypes + ConstantFold. */
void SemanticsPass(ExprCodeStream &cs, const std::map<std::string, DataType> &varTypes);

/** PlanBuffers: assign physical buffer slots for temp reuse. */
void PlanBuffers(ExprCodeStream &cs);

// Query functions on a compiled code stream

/** Compute output dims from input variable dims. */
std::tuple<Dims, Dims, Dims>
GetDims(const ExprCodeStream &cs,
        const std::map<std::string, std::tuple<Dims, Dims, Dims>> &nameToDims);

/** Execute the code stream over numBlocks of data. */
std::vector<DerivedData> Execute(const ExprCodeStream &cs, size_t numBlocks,
                                 std::map<std::string, std::vector<DerivedData>> &nameToData);

/** Compute input selections needed for a given output selection.
    Walks instructions backward, applying each SelectionRule.
    Returns a map from input variable name to (start, count). */
std::map<std::string, std::pair<Dims, Dims>>
ComputeInputSelections(const ExprCodeStream &cs, const Dims &outputStart, const Dims &outputCount);

/** Dump the compiled code stream for debugging. */
std::string DumpCodeStream(const ExprCodeStream &cs);

}
}
#endif
