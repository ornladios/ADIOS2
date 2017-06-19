/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * VariableBase.h Base class for Variable and VariableCompound types. Contains
 * common elements.
 *
 *  Created on: Feb 20, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_VARIABLEBASE_H_
#define ADIOS2_CORE_VARIABLEBASE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <sstream>
#include <string>
#include <vector>
/// \endcond

#include "adios2/ADIOSConfig.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/core/SelectionBoundingBox.h"
#include "adios2/core/Transform.h"

namespace adios2
{
/** Base class for Variable<T> (primitives) and VariableCompound classes */
class VariableBase
{

public:
    /** unique identifier inside Method that creates a Variable */
    const std::string m_Name;

    /** primitive from <T> or compound from struct */
    const std::string m_Type;

    /** Variable -> sizeof(T),
     *  VariableCompound -> from constructor sizeof(struct) */
    const size_t m_ElementSize;

    ShapeID m_ShapeID;                 ///< see shape types in ADIOSTypes.h
    bool m_SingleValue = false;        ///< true: single value, false: array
    const bool m_ConstantDims = false; ///< true: fix m_Shape, m_Start, m_Count
    Dims m_Shape;                      ///< total dimensions across MPI
    Dims m_Start; ///< starting point (offsets) in global shape
    Dims m_Count; ///< dimensions from m_Start in global shape

    Dims m_MemoryStart; ///< offset of memory selection
    Dims m_MemoryCount; ///< subset of m_Shape (e.g. remove ghost points)

    /** Read from this step (must be 0 in staging) */
    unsigned int m_ReadFromStep = 0;
    /** Read this many steps at once (must be 1 in staging) */
    unsigned int m_ReadNSteps = 1;
    /** Global array was written as Joined array, so read accordingly */
    bool m_ReadAsJoined = false;
    /** Global array was written as Local value, so read accordingly */
    bool m_ReadAsLocalValue = false;
    /** number of steps available in a file (or 1 in staging) filled by
     * InquireVariable*/
    unsigned int m_AvailableSteps = 1;

    VariableBase(const std::string &name, const std::string type,
                 const size_t elementSize, const Dims shape, const Dims start,
                 const Dims count, const bool constantShape,
                 const bool debugMode);

    virtual ~VariableBase() = default;

    /**
     * Returns the payload size in bytes
     * @return TotalSize * m_ElementSize
     */
    size_t PayLoadSize() const noexcept;

    /**
     * Returns the total number of elements
     * @return number of elements
     */
    size_t TotalSize() const noexcept;

    /** Set the local dimension and global offset of the variable */
    void SetSelection(const Dims start, const Dims count);

    /** Overloaded version of SetSelection using a SelectionBoundingBox */
    void SetSelection(const SelectionBoundingBox &selection);

    /**
     * Set the local dimension and global offset of the variable using a
     * selection
     * Only bounding boxes are allowed
     */
    void SetMemorySelection(const SelectionBoundingBox &selection);

    /**
     * Set the steps for the variable to read from. The pointer passed at
     * reading must be able to hold enough memory to store multiple steps in a
     * single read.
     * @param startStep  The first step to read. Steps start from 0
     * @param countStep    Number of consecutive steps to read at once.
     *
     */
    void SetStepSelection(const unsigned int startStep,
                          const unsigned int countStep);

    void AddTransform(Transform &transform,
                      const std::vector<std::string> &parametersVector);

    void AddTransform(Transform &transform,
                      const Params &parametersVector = Params());

    /** Apply current sequence of transforms defined by AddTransform */
    virtual void ApplyTransforms() = 0;

    /** Clears out the transform sequence defined by AddTransform */
    void ClearTransforms();

    /** Self-check dims according to type, called from Engine before Write
     * @param hint extra debugging info for the exception */
    void CheckDims(const std::string hint) const;

private:
    const bool m_DebugMode = false;

    void InitShapeType();

    /** Transforms metadata info */
    struct TransformInfo
    {
        /** reference to object derived from Transform class */
        Transform &Object;
        /** parameters from AddTransform */
        Params Parameters;
        /** resulting sizes from transformation */
        Dims Sizes;
    };

    /**
     * Sequence determines application order, e.g.
     * first Transforms[0] then Transforms[1]. Pointer used as
     * reference (no memory management).
     */
    std::vector<TransformInfo> m_TransformsInfo;
};

} // end namespace

#endif /* ADIOS2_CORE_VARIABLEBASE_H_ */
