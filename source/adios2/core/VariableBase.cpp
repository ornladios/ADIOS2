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

#include "adios2/helper/adiosFunctions.h" //GetTotalSize

namespace adios
{

VariableBase::VariableBase(const std::string &name, const std::string type,
                           const size_t elementSize, const Dims shape,
                           const Dims start, const Dims count,
                           const bool constantDims, const bool debugMode)
: m_Name(name), m_Type(type), m_ElementSize(elementSize), m_Shape(shape),
  m_Start(start), m_Count(count), m_ConstantDims(constantDims),
  m_DebugMode(debugMode)
{
    InitShapeType();
}

size_t VariableBase::PayLoadSize() const noexcept
{
    return GetTotalSize(m_Count) * m_ElementSize;
}

size_t VariableBase::TotalSize() const noexcept
{
    return GetTotalSize(m_Count);
}

void VariableBase::SetSelection(const Dims start, const Dims count)
{
    if (m_DebugMode)
    {
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
            (m_Shape.size() != m_Count.size() ||
             m_Shape.size() != m_Start.size()))
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

    m_Count = count;
    m_Start = start;
}

void VariableBase::SetSelection(const SelectionBoundingBox &selection)
{
    SetSelection(selection.m_Start, selection.m_Count);
}

void VariableBase::SetMemorySelection(const SelectionBoundingBox &selection)
{
    if (m_DebugMode)
    {
        if (m_SingleValue)
        {
            throw std::invalid_argument("ERROR: memory selection is not valid "
                                        "for single value variable " +
                                        m_Name +
                                        ", in call to SetMemorySelection\n");
        }
        if (m_Shape.size() != selection.m_Count.size() ||
            m_Shape.size() != selection.m_Start.size())
        {
            throw std::invalid_argument(
                "ERROR: selection argument m_Count and m_Start sizes must be "
                "the "
                "same as variable " +
                m_Name + " m_Shape, in call to SetMemorySelction\n");
        }
    }

    m_MemoryCount = selection.m_Count;
    m_MemoryStart = selection.m_Start;
}

void VariableBase::SetStepSelection(const unsigned int startStep,
                                    const unsigned int countStep)
{
    m_ReadFromStep = startStep;
    m_ReadNSteps = countStep;
}

void VariableBase::AddTransform(
    Transform &transform, const std::vector<std::string> &parametersVector)
{
}

void VariableBase::AddTransform(Transform &transform,
                                const Params &parametersVector)
{
}

// transforms related functions
void VariableBase::ClearTransforms() { m_TransformsInfo.clear(); }

// PRIVATE
void VariableBase::InitShapeType()
{
    if (!m_Shape.empty() && m_Start.empty() && m_Count.empty())
    {
        if (m_DebugMode)
        {
            if (m_ConstantDims)
            {
                throw std::invalid_argument(
                    "ERROR: isConstantShape (true) argument is invalid "
                    "with empty start and count "
                    "arguments\n");
            }
        }

        m_ShapeID = ShapeID::GlobalArray;
    }
    else if (!m_Shape.empty() && m_Shape.size() == m_Start.size() &&
             m_Shape.size() == m_Count.size())
    {
        if (m_DebugMode)
        {
            auto lf_LargerThanError = [&](const unsigned int i,
                                          const std::string dims1,
                                          const std::string dims2) {

                const std::string iString(std::to_string(i));
                throw std::invalid_argument(
                    "ERROR: " + dims1 + "[" + iString + "] > " + dims2 + "[" +
                    iString + "], in DefineVariable " + m_Name + "\n");
            };

            for (unsigned int i = 0; i < m_Shape.size(); ++i)
            {
                if (m_Count[i] > m_Shape[i])
                {
                    lf_LargerThanError(i, "count", "shape");
                }
                if (m_Start[i] > m_Shape[i])
                {
                    lf_LargerThanError(i, "start", "shape");
                }
            }
        }

        m_ShapeID = ShapeID::GlobalArray;
    }
    else if (m_Shape.empty() && m_Start.empty() && m_Count.empty())
    {
        m_ShapeID = ShapeID::GlobalValue;
        m_SingleValue = true;
    }
    else if (m_Shape.empty() && m_Start.empty() && !m_Count.empty())
    {
        m_ShapeID = ShapeID::LocalArray;
    }
    else if (m_Shape.size() == 1 && m_Shape.front() == LocalValueDim)
    {
        m_ShapeID = ShapeID::LocalValue;
        m_SingleValue = true;
    }
    else if (!m_Shape.empty() &&
             std::count(m_Shape.begin(), m_Shape.end(), JoinedDim) == 1)
    {
        m_ShapeID = ShapeID::JoinedArray;
    }
    else if (!m_Shape.empty() &&
             std::count(m_Shape.begin(), m_Shape.end(), JoinedDim) > 1)
    {
        throw std::invalid_argument("ERROR: variable can't have more than one "
                                    "JoinedDim in shape argument, in call to "
                                    "DefineVariable " +
                                    m_Name + "\n");
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

void VariableBase::CheckDims(const std::string hint) const
{
    if (m_ShapeID == ShapeID::GlobalArray)
    {
        if (m_Start.empty() || m_Count.empty())
        {
            throw std::invalid_argument(
                "ERROR: GlobalArray variable " + m_Name +
                " start and count dimensions must be defined by either "
                "DefineVariable or a Selection " +
                hint + "\n");
        }
    }
    // TODO need to think more exceptions here
}

} // end namespace adios
