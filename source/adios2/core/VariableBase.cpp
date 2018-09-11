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
#include <stdexcept> //std::invalid_argument
/// \endcond

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

size_t VariableBase::PayloadSize() const noexcept
{
    return helper::GetTotalSize(m_Count) * m_ElementSize;
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

void VariableBase::SetSelection(const Box<Dims> &boxDims)
{
    const Dims &start = boxDims.first;
    const Dims &count = boxDims.second;

    if (m_DebugMode)
    {
        if (m_Type == helper::GetType<std::string>())
        {
            throw std::invalid_argument(
                "ERROR: string variable " + m_Name +
                " is always LocalValue, it can't have a "
                "selection, in call to SetSelection\n");
        }

        if (m_SingleValue)
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

        if (m_ShapeID == ShapeID::LocalArray && !start.empty())
        {
            throw std::invalid_argument("ERROR: start argument must be empty "
                                        "for local array variable " +
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
}

void VariableBase::SetMemorySelection(const std::pair<Dims, Dims> &boxDims)
{
    const Dims &start = boxDims.first;
    const Dims &count = boxDims.second;

    if (m_DebugMode)
    {
        if (m_SingleValue)
        {
            throw std::invalid_argument("ERROR: memory selection is not valid "
                                        "for single value variable " +
                                        m_Name +
                                        ", in call to SetMemorySelection\n");
        }

        if (m_Shape.size() != start.size() || m_Shape.size() != count.size())
        {
            throw std::invalid_argument(
                "ERROR: selection Dims start, count sizes must be "
                "the same as variable " +
                m_Name + " m_Shape, in call to SetMemorySelction\n");
        }
    }

    m_MemoryStart = start;
    m_MemoryCount = count;
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

size_t VariableBase::SelectionSize() const noexcept
{
    return helper::GetTotalSize(m_Count) * m_StepsCount;
}

bool VariableBase::IsConstantDims() const noexcept { return m_ConstantDims; };
void VariableBase::SetConstantDims() noexcept { m_ConstantDims = true; };

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
    if (m_DebugMode && m_RandomAccess)
    {
        throw std::invalid_argument("ERROR: can't mix streaming and "
                                    "random-access (call to StepStepSelection)"
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

// PRIVATE
void VariableBase::InitShapeType()
{
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
                auto lf_LargerThanError =
                    [&](const unsigned int i, const std::string dims1,
                        const size_t dims1Value, const std::string dims2,
                        const size_t dims2Value) {

                        const std::string iString(std::to_string(i));
                        throw std::invalid_argument(
                            "ERROR: " + dims1 + "[" + iString + "] = " +
                            std::to_string(dims1Value) + " > " + dims2 + "[" +
                            iString + "], = " + std::to_string(dims2Value) +
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
    if (m_DebugMode)
    {
        CheckDimensionsCommon(", in call to DefineVariable(\"" + m_Name +
                              "\",...");
    }
}

void VariableBase::CheckDimensionsCommon(const std::string hint) const
{
    if (!m_DebugMode)
    {
        return;
    }

    if (m_Type == "string")
    {
        if (!(m_Shape.empty() && m_Start.empty() && m_Count.empty()))
        {
            throw std::invalid_argument("ERROR: string variable " + m_Name +
                                        " can't have dimensions (shape, start, "
                                        "count must be empty), string is "
                                        "always defined as a LocalValue, " +
                                        hint + "\n");
        }
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
                "allowed in a {LocalValueDim} "
                "shape, " +
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

} // end namespace core
} // end namespace adios2
