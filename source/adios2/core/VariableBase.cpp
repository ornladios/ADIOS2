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

namespace adios2
{
namespace core
{

VariableBase::VariableBase(const std::string &name, const std::string type,
                           const size_t elementSize, const Dims &shape,
                           const Dims &start, const Dims &count,
                           const bool constantDims, const bool debugMode)
: m_Name(name), m_Type(type), m_ElementSize(elementSize), m_Shape(shape),
  m_Start(start), m_Count(count), m_ConstantDims(constantDims),
  m_DebugMode(debugMode)
{
    InitShapeType();
}

size_t VariableBase::TotalSize() const noexcept
{
    return helper::GetTotalSize(m_Count);
}

void VariableBase::SetShape(const adios2::Dims &shape)
{
    if (m_DebugMode)
    {
        if (m_Type == helper::GetType<std::string>())
        {
            throw std::invalid_argument("ERROR: string variable " + m_Name +
                                        " is always LocalValue, can't change "
                                        "shape, in call to SetShape\n");
        }

        if (m_SingleValue)
        {
            throw std::invalid_argument(
                "ERROR: selection is not valid for single value variable " +
                m_Name + ", in call to SetShape\n");
        }

        if (m_ConstantDims)
        {
            throw std::invalid_argument(
                "ERROR: selection is not valid for constant shape variable " +
                m_Name + ", in call to SetShape\n");
        }

        if (m_ShapeID == ShapeID::LocalArray)
        {
            throw std::invalid_argument("ERROR: can't assign shape dimensions "
                                        "to local array variable " +
                                        m_Name + ", in call to SetShape\n");
        }
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

    if (m_DebugMode)
    {
        if (m_Type == helper::GetType<std::string>() &&
            m_ShapeID != ShapeID::GlobalArray)
        {
            throw std::invalid_argument("ERROR: string variable " + m_Name +
                                        " not a GlobalArray, it can't have a "
                                        "selection, in call to SetSelection\n");
        }

        if (m_SingleValue && m_ShapeID != ShapeID::GlobalArray)
        {
            throw std::invalid_argument(
                "ERROR: selection is not valid for single value variable " +
                m_Name + ", in call to SetSelection\n");
        }

        if (m_ConstantDims)
        {
            throw std::invalid_argument(
                "ERROR: selection is not valid for constant shape variable " +
                m_Name + ", in call to SetSelection\n");
        }

        if (m_ShapeID == ShapeID::GlobalArray &&
            (m_Shape.size() != count.size() || m_Shape.size() != start.size()))
        {
            throw std::invalid_argument("ERROR: count and start must be the "
                                        "same size as shape for variable " +
                                        m_Name + ", in call to SetSelection\n");
        }

        if (m_ShapeID == ShapeID::JoinedArray && !start.empty())
        {
            throw std::invalid_argument("ERROR: start argument must be empty "
                                        "for joined array variable " +
                                        m_Name + ", in call to SetSelection\n");
        }
    }

    m_Start = start;
    m_Count = count;
    m_SelectionType = SelectionType::BoundingBox;
}

void VariableBase::SetMemorySelection(const Box<Dims> &memorySelection)
{
    const Dims &memoryStart = memorySelection.first;
    const Dims &memoryCount = memorySelection.second;

    if (m_DebugMode)
    {
        if (m_SingleValue)
        {
            throw std::invalid_argument("ERROR: memory start is not valid "
                                        "for single value variable " +
                                        m_Name +
                                        ", in call to SetMemorySelection\n");
        }

        if (m_Start.size() != memoryStart.size())
        {
            throw std::invalid_argument("ERROR: memoryStart size must be "
                                        "the same as variable " +
                                        m_Name + " start size " +
                                        std::to_string(m_Start.size()) +
                                        ", in call to SetMemorySelection\n");
        }

        if (m_Count.size() != memoryCount.size())
        {
            throw std::invalid_argument("ERROR: memoryCount size must be "
                                        "the same as variable " +
                                        m_Name + " count size " +
                                        std::to_string(m_Count.size()) +
                                        ", in call to SetMemorySelection\n");
        }

        // TODO might have to remove for reading
        for (size_t i = 0; i < memoryCount.size(); ++i)
        {
            if (memoryCount[i] < m_Count[i])
            {
                const std::string indexStr = std::to_string(i);
                const std::string memoryCountStr =
                    std::to_string(memoryCount[i]);
                const std::string countStr = std::to_string(m_Count[i]);

                throw std::invalid_argument(
                    "ERROR: memoyCount[" + indexStr + "]= " + memoryCountStr +
                    " can not be smaller than variable count[" + indexStr +
                    "]= " + countStr + " for variable " + m_Name +
                    ", in call to SetMemorySelection\n");
            }
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
    if (m_DebugMode && boxSteps.second == 0)
    {
        throw std::invalid_argument("ERROR: boxSteps.second count argument "
                                    " can't be zero, from variable " +
                                    m_Name + ", in call to SetStepSelection\n");
    }

    m_StepsStart = boxSteps.first;
    m_StepsCount = boxSteps.second;
    m_RandomAccess = true;
}

size_t VariableBase::AddOperation(Operator &op,
                                  const Params &parameters) noexcept
{
    m_Operations.push_back(Operation{&op, parameters, Params()});
    return m_Operations.size() - 1;
}

void VariableBase::SetOperationParameter(const size_t operationID,
                                         const std::string key,
                                         const std::string value)
{
    if (m_DebugMode)
    {
        if (operationID >= m_Operations.size())
        {
            throw std::invalid_argument(
                "ERROR: invalid operationID " + std::to_string(operationID) +
                ", check returned id from AddOperation, in call to "
                "SetOperationParameter\n");
        }
    }

    m_Operations[operationID].Parameters[key] = value;
}

void VariableBase::CheckDimensions(const std::string hint) const
{
    if (m_DebugMode && m_ShapeID == ShapeID::GlobalArray)
    {
        if (m_Start.empty() || m_Count.empty())
        {
            throw std::invalid_argument(
                "ERROR: GlobalArray variable " + m_Name +
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
    if (m_DebugMode && m_RandomAccess && !m_FirstStreamingStep)
    {
        throw std::invalid_argument("ERROR: can't mix streaming and "
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

        auto itAttribute = io.m_Attributes.find(attributeName);
        const std::string type = itAttribute->second.first;

        const std::string key =
            fullNameKeys ? attributeName : attributeName.substr(prefix.size());

        if (type == "compound")
        {
        }
#define declare_template_instantiation(T)                                      \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        Attribute<T> &attribute =                                              \
            io.GetAttributeMap<T>().at(itAttribute->second.second);            \
        attributesInfo[key] = attribute.GetInfo();                             \
    }
        ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
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
        for (const auto &attributePair : io.m_Attributes)
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
    if (m_DebugMode && m_Type == helper::GetType<std::string>())
    {
        if (m_Shape.empty())
        {
            if (!m_Start.empty() || !m_Count.empty())
            {
                throw std::invalid_argument(
                    "ERROR: GlobalValue string variable " + m_Name +
                    " can't have Start and Count dimensions, string variables "
                    "are always defined as a GlobalValue or LocalValue, "
                    " in call to DefineVariable\n");
            }
        }
        else
        {
            if (m_Shape != Dims{LocalValueDim})
            {
                throw std::invalid_argument(
                    "ERROR: LocalValue string variable " + m_Name +
                    " Shape must be equal to {LocalValueDim}, string variables "
                    "are always defined as a GlobalValue or LocalValue, " +
                    " in call to DefineVariable\n");
            }
        }
    }

    if (!m_Shape.empty())
    {
        if (std::count(m_Shape.begin(), m_Shape.end(), JoinedDim) == 1)
        {
            if (m_DebugMode && !m_Start.empty() &&
                std::count(m_Start.begin(), m_Start.end(), 0) != m_Start.size())
            {
                throw std::invalid_argument("ERROR: The Start array must be "
                                            "empty or full-zero when defining "
                                            "a Joined Array in call to "
                                            "DefineVariable " +
                                            m_Name + "\n");
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

                if (m_DebugMode && m_ConstantDims)
                {
                    throw std::invalid_argument(
                        "ERROR: isConstantShape (true) argument is invalid "
                        "with empty start and count "
                        "arguments in call to "
                        "DefineVariable " +
                        m_Name + "\n");
                }

                m_ShapeID = ShapeID::GlobalArray;
            }
        }
        else if (m_Shape.size() == m_Start.size() &&
                 m_Shape.size() == m_Count.size())
        {
            if (m_DebugMode)
            {
                auto lf_LargerThanError = [&](const unsigned int i,
                                              const std::string dims1,
                                              const size_t dims1Value,
                                              const std::string dims2,
                                              const size_t dims2Value) {
                    const std::string iString(std::to_string(i));
                    throw std::invalid_argument(
                        "ERROR: " + dims1 + "[" + iString +
                        "] = " + std::to_string(dims1Value) + " > " + dims2 +
                        "[" + iString + "], = " + std::to_string(dims2Value) +
                        " in DefineVariable " + m_Name + "\n");
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
            }
            m_ShapeID = ShapeID::GlobalArray;
        }
        else
        {
            throw std::invalid_argument("ERROR: the "
                                        "combination of shape, start and count "
                                        "arguments is inconsistent, in call to "
                                        "DefineVariable " +
                                        m_Name + "\n");
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
            if (m_DebugMode)
            {
                throw std::invalid_argument(
                    "ERROR: if the "
                    "shape is empty, start must be empty as well, in call to "
                    "DefineVariable " +
                    m_Name + "\n");
            }
        }
    }

    /* Extra checks for invalid settings */
    CheckDimensionsCommon(", in call to DefineVariable(\"" + m_Name + "\",...");
}

void VariableBase::CheckDimensionsCommon(const std::string hint) const
{
    if (!m_DebugMode)
    {
        return;
    }

    if (m_ShapeID != ShapeID::LocalValue)
    {
        if ((!m_Shape.empty() &&
             std::count(m_Shape.begin(), m_Shape.end(), LocalValueDim) > 0) ||
            (!m_Start.empty() &&
             std::count(m_Start.begin(), m_Start.end(), LocalValueDim) > 0) ||
            (!m_Count.empty() &&
             std::count(m_Count.begin(), m_Count.end(), LocalValueDim) > 0))
        {
            throw std::invalid_argument(
                "ERROR: LocalValueDim parameter is only "
                "allowed as {LocalValueDim} in Shape dimensions " +
                hint + "\n");
        }
    }

    if ((!m_Shape.empty() &&
         std::count(m_Shape.begin(), m_Shape.end(), JoinedDim) > 1) ||
        (!m_Start.empty() &&
         std::count(m_Start.begin(), m_Start.end(), JoinedDim) > 0) ||
        (!m_Count.empty() &&
         std::count(m_Count.begin(), m_Count.end(), JoinedDim) > 0))
    {
        throw std::invalid_argument("ERROR: JoinedDim is only allowed once in "
                                    "Shape and cannot appear in start/count, " +
                                    hint + "\n");
    }
}

Dims VariableBase::GetShape(const size_t step)
{
    if (m_Type == "compound")
    {
        // not supported
    }
#define declare_template_instantiation(T)                                      \
    else if (m_Type == helper::GetType<T>())                                   \
    {                                                                          \
        Variable<T> *variable = dynamic_cast<Variable<T> *>(this);             \
        m_Shape = variable->Shape(step);                                       \
    }
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

    return m_Shape;
}

} // end namespace core
} // end namespace adios2
