/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * VariableBase.h
 *
 *  Created on: Feb 20, 2017
 *      Author: wfg
 */

#ifndef ADIOS2_CORE_VARIABLEBASE_H_
#define ADIOS2_CORE_VARIABLEBASE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <exception>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>
/// \endcond

#include "adios2/ADIOSConfig.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/core/SelectionBoundingBox.h"
#include "adios2/core/adiosFunctions.h"
#include "adios2/core/adiosTemplates.h"

namespace adios
{

using Dims = std::vector<std::size_t>;

class VariableBase
{

public:
    const std::string m_Name;   ///< variable name
    const std::string m_Type;   ///< variable type
    const bool m_ConstantShape; ///< dimensions and offsets cannot change after
                                /// declaration

    /**
     * Variable -> sizeof(T),
     * VariableCompound -> from constructor sizeof(struct)
     */
    const std::size_t m_ElementSize;

    Dims m_Shape;            ///< total dimensions across MPI
    Dims m_Start;            ///< offsets of local writer in global shape
    Dims m_Count;            ///< dimensions of the local writer in global shape
    Dims m_MemoryDimensions; ///< array of memory dimensions
    Dims m_MemoryOffsets;    ///< array of memory offsets

    VarClass m_VarClass;
    bool m_IsScalar = false; /// Global value or Loval value
    const bool m_IsDimension = false;
    const bool m_DebugMode = false;

    VariableBase(const std::string &name, const std::string type,
                 const std::size_t elementSize, const Dims shape,
                 const Dims start, const Dims count, const bool constantShape,
                 const bool debugMode)
    : m_Name{name}, m_Type{type}, m_ConstantShape{constantShape},
      m_ElementSize{elementSize}, m_Count{count}, m_Shape{shape},
      m_Start{start}, m_DebugMode{debugMode}
    {
        if (shape.empty() && start.empty())
        {
            if (count.empty())
            {
                m_VarClass = VarClass::GlobalValue;
                m_IsScalar = true;
            }
            else
            {
                m_VarClass = VarClass::LocalArray;
            }
        }
        else if (shape.size() == 1 && shape[0] == LocalValueDim)
        {
            m_VarClass = VarClass::LocalValue;
            m_IsScalar = true;
        }
        else if (shape.size() > 0 && shape[0] == JoinedDim)
        {
            m_VarClass = VarClass::JoinedArray;
        }
        else
        {
            if ((start.empty() && count.empty()) ||
                (shape.size() == start.size() && shape.size() == count.size()))
            {
                m_VarClass = VarClass::GlobalArray;
            }
            else
            {
                throw std::invalid_argument("DefineVariable() is invalid. The "
                                            "combination of dimension "
                                            "specifications cannot be "
                                            "interpreted\n");
            }
        }
    }

    virtual ~VariableBase() {}

    std::size_t DimensionsSize() const noexcept { return m_Count.size(); }

    /**
     * Returns the payload size in bytes
     * @return TotalSize * sizeof(T)
     */
    std::size_t PayLoadSize() const noexcept
    {
        return GetTotalSize(m_Count) * m_ElementSize;
    }

    /**
     * Returns the total size
     * @return number of elements
     */
    std::size_t TotalSize() const noexcept { return GetTotalSize(m_Count); }

    /**
     * Set the local dimension and global offset of the variable
     */
    void SetSelection(const Dims start, const Dims count)
    {
        if (m_IsScalar)
        {
            throw std::invalid_argument("Variable.SetSelection() is an invalid "
                                        "call for single value variable '" +
                                        m_Name + "'\n");
        }
        if (m_ConstantShape)
        {
            throw std::invalid_argument(
                "Variable.SetSelection() is not allowed "
                "for constant shape variable '" +
                m_Name + "'\n");
        }
        if (m_VarClass == VarClass::GlobalArray &&
            m_Shape.size() != count.size())
        {
            throw std::invalid_argument("Variable.SetSelection() selection "
                                        "dimension must equal the global "
                                        "dimension of the variable '" +
                                        m_Name + "'\n");
        }
        if ((m_VarClass == VarClass::LocalArray ||
             m_VarClass == VarClass::JoinedArray) &&
            !start.empty())
        {
            throw std::invalid_argument(
                "Variable.SetSelection() for local or joined array '" + m_Name +
                "' should pass an empty 'start' argument\n");
        }
//        ConvertUint64VectorToSizetVector(count, m_Count);
//        ConvertUint64VectorToSizetVector(start, m_Start);
        m_Count = count;
        m_Start = start;
    }

    /**
     * Set the local dimension and global offset of the variable using a
     * selection
     */
    void SetSelection(const SelectionBoundingBox &sel)
    {
        Dims start, count;
        ConvertUint64VectorToSizetVector(sel.m_Start, start);
        ConvertUint64VectorToSizetVector(sel.m_Count, count);
        SetSelection(start, count);
    }

    /**
     * Set the local dimension and global offset of the variable using a
     * selection
     * Only bounding boxes are allowed
     */
    void SetMemorySelection(const SelectionBoundingBox &sel)
    {
        if (m_Shape.size() == 0)
        {
            throw std::invalid_argument(
                "Variable.SetMemorySelection() is an invalid "
                "call for single value variables\n");
        }
        if (m_Shape.size() != sel.m_Count.size())
        {
            throw std::invalid_argument(
                "Variable.SetMemorySelection() bounding box "
                "dimension must equal the global "
                "dimension of the variable\n");
        }

        ConvertUint64VectorToSizetVector(sel.m_Count, m_MemoryDimensions);
        ConvertUint64VectorToSizetVector(sel.m_Start, m_MemoryOffsets);
    }

    /**
     * Set the steps for the variable to read from. The pointer passed at
     * reading must be able to hold enough memory to store multiple steps in a
     * single read.
     * @param fromStep  The first step to read. Steps start from 0
     * @param nSteps    Number of consecutive steps to read at once.
     *
     */
    void SetStepSelection(const unsigned int fromStep,
                          const unsigned int nSteps)
    {
        m_ReadFromStep = fromStep;
        m_ReadNSteps = nSteps;
    }

    /** Return the number of steps available for the variable
     *  @return Number of steps
     */
    unsigned int GetNSteps() { return m_NStepsAvailable; }

    ///< Should only be called by read engines
    void SetNSteps(unsigned int steps) { m_NStepsAvailable = steps; }
    unsigned int GetReadFromStep() { return m_ReadFromStep; }
    unsigned int GetReadNSteps() { return m_ReadNSteps; }
    void SetReadAsJoinedArray() { m_ReadAsJoined = true; }
    void SetReadAsLocalValue() { m_ReadAsLocalValue = true; }
    bool ReadAsJoinedArray() { return m_ReadAsJoined; }
    bool ReadAsLocalValue() { return m_ReadAsLocalValue; }

private:
    ///< Read from this step (must be 0 in staging)
    unsigned int m_ReadFromStep = 0;
    ///< Read this many steps at once (must be 1 in staging)
    unsigned int m_ReadNSteps = 1;
    ///< Global array was written as Joined array, so read accordingly
    bool m_ReadAsJoined = false;
    ///< Global array was written as Local value, so read accordingly
    bool m_ReadAsLocalValue = false;

    /* Values filled by InquireVariable() */
    ///< number of steps available in a file (or 1 in staging),
    unsigned int m_NStepsAvailable = 1;
};

} // end namespace

#endif /* ADIOS2_CORE_VARIABLEBASE_H_ */
