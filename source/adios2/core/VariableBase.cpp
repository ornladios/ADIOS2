/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * VariableBase.cpp
 *
 *  Created on: May 11, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "VariableBase.h"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <algorithm> //std::count
#include <iterator>  //std::next
#include <stdexcept> //std::invalid_argument
/// \endcond

#include "adios2/common/ADIOSMacros.h"
#include "adios2/core/Engine.h"
#include "adios2/core/IO.h"
#include "adios2/core/Variable.h"
#include "adios2/helper/adiosFunctions.h" //helper::GetTotalSize
#include "adios2/helper/adiosString.h"
#include "adios2/operator/OperatorFactory.h"

#ifdef ADIOS2_HAVE_CUDA
#include <cuda_runtime.h>
#endif

namespace adios2
{
namespace core
{

VariableBase::VariableBase(const std::string &name, const DataType type,
                           const size_t elementSize, const Dims &shape,
                           const Dims &start, const Dims &count,
                           const bool constantDims)
: m_Name(name), m_Type(type), m_ElementSize(elementSize), m_Shape(shape),
  m_Start(start), m_Count(count), m_ConstantDims(constantDims)
{
    InitShapeType();
}

size_t VariableBase::TotalSize() const noexcept
{
    return helper::GetTotalSize(m_Count);
}

MemorySpace VariableBase::GetMemorySpace(const void *ptr)
{
#ifdef ADIOS2_HAVE_GPU_SUPPORT
    if (m_MemSpace != MemorySpace::Detect)
    {
        return m_MemSpace;
    }
#endif

#ifdef ADIOS2_HAVE_CUDA
    cudaPointerAttributes attr;
    cudaPointerGetAttributes(&attr, ptr);
    if (attr.type == cudaMemoryTypeDevice)
    {
        return MemorySpace::CUDA;
    }
#endif
    return MemorySpace::Host;
}

void VariableBase::SetMemorySpace(const MemorySpace mem) { m_MemSpace = mem; }

void VariableBase::SetShape(const adios2::Dims &shape)
{
    if (m_Type == helper::GetDataType<std::string>())
    {
        helper::Throw<std::invalid_argument>(
            "Core", "VariableBase", "SetShape",
            "string variable " + m_Name +
                " is always LocalValue, can't change "
                "shape, in call to SetShape");
    }

    if (m_SingleValue)
    {
        helper::Throw<std::invalid_argument>(
            "Core", "VariableBase", "SetShape",
            "selection is not valid for single value variable " + m_Name +
                ", in call to SetShape");
    }

    if (m_ConstantDims)
    {
        helper::Throw<std::invalid_argument>(
            "Core", "VariableBase", "SetShape",
            "selection is not valid for constant shape variable " + m_Name +
                ", in call to SetShape");
    }

    if (m_ShapeID == ShapeID::LocalArray)
    {
        helper::Throw<std::invalid_argument>("Core", "VariableBase", "SetShape",
                                             "can't assign shape dimensions "
                                             "to local array variable " +
                                                 m_Name +
                                                 ", in call to SetShape");
    }

    m_Shape = shape;
}

void VariableBase::SetBlockSelection(const size_t blockID)
{
    m_BlockID = blockID;
    m_SelectionType = SelectionType::WriteBlock;
}

void VariableBase::SetSelection(const Box<Dims> &boxDims)
{
    const Dims &start = boxDims.first;
    const Dims &count = boxDims.second;

    if (m_Type == helper::GetDataType<std::string>() &&
        m_ShapeID != ShapeID::GlobalArray)
    {
        helper::Throw<std::invalid_argument>(
            "Core", "VariableBase", "SetSelection",
            "string variable " + m_Name +
                " not a GlobalArray, it can't have a "
                "selection, in call to SetSelection");
    }

    if (m_SingleValue && m_ShapeID != ShapeID::GlobalArray)
    {
        helper::Throw<std::invalid_argument>(
            "Core", "VariableBase", "SetSelection",
            "selection is not valid for single value variable " + m_Name +
                ", in call to SetSelection");
    }

    if (m_ConstantDims)
    {
        helper::Throw<std::invalid_argument>(
            "Core", "VariableBase", "SetSelection",
            "selection is not valid for constant shape variable " + m_Name +
                ", in call to SetSelection");
    }

    if (m_ShapeID == ShapeID::GlobalArray &&
        (m_Shape.size() != count.size() || m_Shape.size() != start.size()))
    {
        helper::Throw<std::invalid_argument>(
            "Core", "VariableBase", "SetSelection",
            "count and start must be the "
            "same size as shape for variable " +
                m_Name + ", in call to SetSelection");
    }

    if (m_ShapeID == ShapeID::JoinedArray && !start.empty())
    {
        helper::Throw<std::invalid_argument>(
            "Core", "VariableBase", "SetSelection",
            "start argument must be empty "
            "for joined array variable " +
                m_Name + ", in call to SetSelection");
    }

    m_Start = start;
    m_Count = count;
    m_SelectionType = SelectionType::BoundingBox;
}

void VariableBase::SetMemorySelection(const Box<Dims> &memorySelection)
{
    const Dims &memoryStart = memorySelection.first;
    const Dims &memoryCount = memorySelection.second;

    if (m_SingleValue)
    {
        helper::Throw<std::invalid_argument>(
            "Core", "VariableBase", "SetMemorySelection",
            "memory start is not valid "
            "for single value variable " +
                m_Name + ", in call to SetMemorySelection");
    }

    if (m_Start.size() != memoryStart.size())
    {
        helper::Throw<std::invalid_argument>(
            "Core", "VariableBase", "SetMemorySelection",
            "memoryStart size must be "
            "the same as variable " +
                m_Name + " start size " + std::to_string(m_Start.size()) +
                ", in call to SetMemorySelection");
    }

    if (m_Count.size() != memoryCount.size())
    {
        helper::Throw<std::invalid_argument>(
            "Core", "VariableBase", "SetMemorySelection",
            "memoryCount size must be "
            "the same as variable " +
                m_Name + " count size " + std::to_string(m_Count.size()) +
                ", in call to SetMemorySelection");
    }

    // TODO might have to remove for reading
    for (size_t i = 0; i < memoryCount.size(); ++i)
    {
        if (memoryCount[i] < m_Count[i])
        {
            const std::string indexStr = std::to_string(i);
            const std::string memoryCountStr = std::to_string(memoryCount[i]);
            const std::string countStr = std::to_string(m_Count[i]);

            helper::Throw<std::invalid_argument>(
                "Core", "VariableBase", "SetMemorySelection",
                "memoyCount[" + indexStr + "]= " + memoryCountStr +
                    " can not be smaller than variable count[" + indexStr +
                    "]= " + countStr + " for variable " + m_Name +
                    ", in call to SetMemorySelection");
        }
    }

    m_MemoryStart = memorySelection.first;
    m_MemoryCount = memorySelection.second;
}

size_t VariableBase::GetAvailableStepsStart() const
{
    return m_AvailableStepsStart;
}

size_t VariableBase::GetAvailableStepsCount() const
{
    return m_AvailableStepsCount;
}

void VariableBase::SetStepSelection(const Box<size_t> &boxSteps)
{
    if (boxSteps.second == 0)
    {
        helper::Throw<std::invalid_argument>(
            "Core", "VariableBase", "SetStepSelection",
            "boxSteps.second count argument "
            " can't be zero, from variable " +
                m_Name + ", in call to SetStepSelection");
    }

    m_StepsStart = boxSteps.first;
    m_StepsCount = boxSteps.second;
    m_RandomAccess = true;
    if (m_ShapeID == ShapeID::GlobalArray)
    {
        /* Handle Global Array with changing shape over steps */
        const auto it = m_AvailableShapes.find(m_StepsStart + 1);
        if (it != m_AvailableShapes.end())
        {
            m_Shape = it->second;
        }
    }
}

size_t VariableBase::AddOperation(const std::string &type,
                                  const Params &parameters) noexcept
{
    auto op = MakeOperator(type, parameters);
    if (op->IsDataTypeValid(m_Type))
    {
        m_Operations.push_back(op);
    }
    else
    {
        helper::Log("Variable", "VariableBase", "AddOperation",
                    "Operator " + op->m_TypeString +
                        " does not support data type " + ToString(m_Type) +
                        ", operator not added",
                    helper::LogMode::WARNING);
    }
    return m_Operations.size() - 1;
}

size_t VariableBase::AddOperation(std::shared_ptr<core::Operator> op) noexcept
{
    m_Operations.push_back(op);
    return m_Operations.size() - 1;
}

void VariableBase::RemoveOperations() noexcept { m_Operations.clear(); }

void VariableBase::SetOperationParameter(const size_t operationID,
                                         const std::string key,
                                         const std::string value)
{
    if (operationID >= m_Operations.size())
    {
        helper::Throw<std::invalid_argument>(
            "Core", "VariableBase", "SetOperationParameter",
            "invalid operationID " + std::to_string(operationID) +
                ", check returned id from AddOperation, in call to "
                "SetOperationParameter");
    }

    m_Operations[operationID]->SetParameter(key, value);
}

void VariableBase::CheckDimensions(const std::string hint) const
{
    if (m_ShapeID == ShapeID::GlobalArray)
    {
        if (m_Start.empty() || m_Count.empty())
        {
            helper::Throw<std::invalid_argument>(
                "Core", "VariableBase", "CheckDimensions",
                "GlobalArray variable " + m_Name +
                    " start and count dimensions must be defined by either "
                    "IO.DefineVariable or Variable.SetSelection, " +
                    hint + "\n");
        }
    }

    CheckDimensionsCommon(hint);
}

bool VariableBase::IsConstantDims() const noexcept { return m_ConstantDims; }
void VariableBase::SetConstantDims() noexcept { m_ConstantDims = true; }

bool VariableBase::IsValidStep(const size_t step) const noexcept
{
    if (m_AvailableStepBlockIndexOffsets.count(step) == 1)
    {
        return true;
    }

    return false;
}

void VariableBase::CheckRandomAccessConflict(const std::string hint) const
{
    if (m_RandomAccess && !m_FirstStreamingStep)
    {
        helper::Throw<std::invalid_argument>(
            "Core", "VariableBase", "CheckRandomAccessConflict",
            "can't mix streaming and "
            "random-access (call to SetStepSelection)"
            "for variable " +
                m_Name + ", " + hint);
    }
}

void VariableBase::ResetStepsSelection(const bool zeroStart) noexcept
{
    m_StepsCount = 1;

    if (zeroStart)
    {
        m_StepsStart = 0;
        return;
    }

    if (m_FirstStreamingStep)
    {
        m_StepsStart = 0;
        m_FirstStreamingStep = false;
    }
    else
    {
        ++m_StepsStart;
    }
}

std::map<std::string, Params>
VariableBase::GetAttributesInfo(core::IO &io, const std::string separator,
                                const bool fullNameKeys) const noexcept
{
    auto lf_GetAttributeInfo = [](const std::string &prefix,
                                  const std::string &attributeName,
                                  core::IO &io,
                                  std::map<std::string, Params> &attributesInfo,
                                  const bool fullNameKeys) {
        if (attributeName.compare(0, prefix.size(), prefix) != 0)
        {
            return;
        }

        auto itAttribute = io.GetAttributes().find(attributeName);

        const std::string key =
            fullNameKeys ? attributeName : attributeName.substr(prefix.size());

        if (itAttribute->second->m_Type == DataType::Struct)
        {
        }
        else
        {
            attributesInfo[key] = itAttribute->second->GetInfo();
        }
    };

    // BODY OF FUNCTION STARTS HERE
    std::map<std::string, Params> attributesInfo;
    const std::string prefix = m_Name + separator;

    if (io.m_IsPrefixedNames)
    {
        // get prefixed attributes from stored attributes
        for (const std::string &attributeName : m_PrefixedAttributes)
        {
            lf_GetAttributeInfo(prefix, attributeName, io, attributesInfo,
                                fullNameKeys);
        }
    }
    else
    { // get prefixed attributes on-the-fly (expensive)
        for (const auto &attributePair : io.GetAttributes())
        {
            const std::string &attributeName = attributePair.first;
            lf_GetAttributeInfo(prefix, attributeName, io, attributesInfo,
                                fullNameKeys);
        }
    }

    return attributesInfo;
}

// PRIVATE
void VariableBase::InitShapeType()
{
    if (m_Type == helper::GetDataType<std::string>())
    {
        if (m_Shape.empty())
        {
            if (!m_Start.empty() || !m_Count.empty())
            {
                helper::Throw<std::invalid_argument>(
                    "Core", "VariableBase", "InitShapeType",
                    "GlobalValue string variable " + m_Name +
                        " can't have Start and Count dimensions, string "
                        "variables "
                        "are always defined as a GlobalValue or LocalValue, "
                        " in call to DefineVariable");
            }
        }
        else
        {
            if (m_Shape != Dims{static_cast<size_t>(LocalValueDim)})
            {
                helper::Throw<std::invalid_argument>(
                    "Core", "VariableBase", "InitShapeType",
                    "LocalValue string variable " + m_Name +
                        " Shape must be equal to {LocalValueDim}, string "
                        "variables "
                        "are always defined as a GlobalValue or LocalValue, " +
                        " in call to DefineVariable");
            }
        }
    }

    if (!m_Shape.empty())
    {
        if (std::count(m_Shape.begin(), m_Shape.end(), JoinedDim) == 1)
        {
            if (!m_Start.empty() &&
                static_cast<size_t>(std::count(m_Start.begin(), m_Start.end(),
                                               0)) != m_Start.size())
            {
                helper::Throw<std::invalid_argument>(
                    "Core", "VariableBase", "InitShapeType",
                    "The Start array must be "
                    "empty or full-zero when defining "
                    "a Joined Array in call to "
                    "DefineVariable " +
                        m_Name);
            }
            m_ShapeID = ShapeID::JoinedArray;
        }
        else if (m_Start.empty() && m_Count.empty())
        {
            if (m_Shape.size() == 1 && m_Shape.front() == LocalValueDim)
            {
                m_ShapeID = ShapeID::LocalValue;
                m_Start.resize(1, 0); // start[0] == 0
                m_Count.resize(1, 1); // count[0] == 1
                // Count[0] == 1 makes sure the value will be written
                // into the data file (PutVariablePayload())
                m_SingleValue = true;
            }
            else
            {

                if (m_ConstantDims)
                {
                    helper::Throw<std::invalid_argument>(
                        "Core", "VariableBase", "InitShapeType",
                        "isConstantShape (true) argument is invalid "
                        "with empty start and count "
                        "arguments in call to "
                        "DefineVariable " +
                            m_Name);
                }

                m_ShapeID = ShapeID::GlobalArray;
            }
        }
        else if (m_Shape.size() == m_Start.size() &&
                 m_Shape.size() == m_Count.size())
        {
            auto lf_LargerThanError = [&](const unsigned int i,
                                          const std::string dims1,
                                          const size_t dims1Value,
                                          const std::string dims2,
                                          const size_t dims2Value) {
                const std::string iString(std::to_string(i));
                helper::Throw<std::invalid_argument>(
                    "Core", "VariableBase", "InitShapeType",
                    dims1 + "[" + iString +
                        "] = " + std::to_string(dims1Value) + " > " + dims2 +
                        "[" + iString + "], = " + std::to_string(dims2Value) +
                        " in DefineVariable " + m_Name);
            };

            for (unsigned int i = 0; i < m_Shape.size(); ++i)
            {
                if (m_Count[i] > m_Shape[i])
                {
                    lf_LargerThanError(i, "count", m_Count[i], "shape",
                                       m_Shape[i]);
                }
                if (m_Start[i] > m_Shape[i])
                {
                    lf_LargerThanError(i, "start", m_Start[i], "shape",
                                       m_Shape[i]);
                }
            }
            m_ShapeID = ShapeID::GlobalArray;
        }
        else
        {
            helper::Throw<std::invalid_argument>(
                "Core", "VariableBase", "InitShapeType",
                "the "
                "combination of shape, start and count "
                "arguments is inconsistent, in call to "
                "DefineVariable " +
                    m_Name);
        }
    }
    else
    {
        if (m_Start.empty())
        {
            if (m_Count.empty())
            {
                m_ShapeID = ShapeID::GlobalValue;
                m_SingleValue = true;
            }
            else if (m_Start.empty() && !m_Count.empty())
            {
                m_ShapeID = ShapeID::LocalArray;
            }
        }
        else
        {
            helper::Throw<std::invalid_argument>(
                "Core", "VariableBase", "InitShapeType",
                "if the "
                "shape is empty, start must be empty as well, in call to "
                "DefineVariable " +
                    m_Name);
        }
    }

    /* Extra checks for invalid settings */
    CheckDimensionsCommon(", in call to DefineVariable(\"" + m_Name + "\",...");
}

void VariableBase::CheckDimensionsCommon(const std::string hint) const
{
    if (m_ShapeID != ShapeID::LocalValue)
    {
        if ((!m_Shape.empty() &&
             std::count(m_Shape.begin(), m_Shape.end(), LocalValueDim) > 0) ||
            (!m_Start.empty() &&
             std::count(m_Start.begin(), m_Start.end(), LocalValueDim) > 0) ||
            (!m_Count.empty() &&
             std::count(m_Count.begin(), m_Count.end(), LocalValueDim) > 0))
        {
            helper::Throw<std::invalid_argument>(
                "Core", "VariableBase", "CheckDimensionsCommon",
                "LocalValueDim parameter is only "
                "allowed as {LocalValueDim} in Shape dimensions " +
                    hint);
        }
    }

    if ((!m_Shape.empty() &&
         std::count(m_Shape.begin(), m_Shape.end(), JoinedDim) > 1) ||
        (!m_Start.empty() &&
         std::count(m_Start.begin(), m_Start.end(), JoinedDim) > 0) ||
        (!m_Count.empty() &&
         std::count(m_Count.begin(), m_Count.end(), JoinedDim) > 0))
    {
        helper::Throw<std::invalid_argument>(
            "Core", "VariableBase", "CheckDimensionsCommon",
            "JoinedDim is only allowed once in "
            "Shape and cannot appear in start/count, " +
                hint);
    }
}

void VariableBase::CheckRandomAccess(const size_t step,
                                     const std::string hint) const
{
    if (!m_FirstStreamingStep && step != DefaultSizeT)
    {
        helper::Throw<std::invalid_argument>(
            "Core", "Variable", "CheckRandomAccess",
            "can't pass a step input in "
            "streaming (BeginStep/EndStep)"
            "mode for variable " +
                m_Name + ", in call to Variable<T>::" + hint);
    }
}

Dims VariableBase::Shape(const size_t step) const
{
    CheckRandomAccess(step, "Shape");

    if (m_Engine)
    {
        // see if the engine implements Variable Shape inquiry
        auto ShapePtr = m_Engine->VarShape(*this, step);
        if (ShapePtr)
        {
            return *ShapePtr;
        }
    }
    if (m_FirstStreamingStep && step == adios2::EngineCurrentStep)
    {
        return m_Shape;
    }

    if (m_Engine != nullptr && m_ShapeID == ShapeID::GlobalArray)
    {
        const size_t stepInput =
            !m_FirstStreamingStep ? m_Engine->CurrentStep() : step;

        const auto it = m_AvailableShapes.find(stepInput + 1);
        if (it != m_AvailableShapes.end())
        {
            return it->second;
        }
    }
    return m_Shape;
}

} // end namespace core
} // end namespace adios2
