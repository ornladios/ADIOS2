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

    /** Global array was written as Joined array, so read accordingly */
    bool m_ReadAsJoined = false;
    /** Global array was written as Local value, so read accordingly */
    bool m_ReadAsLocalValue = false;

    /** Transforms metadata info */
    struct TransformInfo
    {
        /** reference to object derived from Transform class */
        Transform &Operator;
        /** parameters from AddTransform */
        Params Parameters;
        /** resulting sizes from transformation */
        Dims Sizes;
    };

    /** Registered transforms */
    std::vector<TransformInfo> m_TransformsInfo;

    size_t m_AvailableStepsStart = 1;
    size_t m_AvailableStepsCount = 0;

    size_t m_StepStart = 1;
    size_t m_StepCount = 1;

    /** Index Metadata Position in a serial metadata buffer */
    size_t m_IndexStart;

    /** Index to Step and Subsets inside a step characteristics position in a
     * serial metadata buffer
     * <pre>
     * key: step number (time_index in bp3 format)
     * value:  vector of block starts for that step
     * </pre>
     * */
    std::map<size_t, std::vector<size_t>> m_IndexStepBlockStarts;

    VariableBase(const std::string &name, const std::string type,
                 const size_t elementSize, const Dims &shape, const Dims &start,
                 const Dims &count, const bool constantShape,
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

    /** Set Dims and Time start and count */
    void SetSelection(const std::pair<Dims, Dims> &boxDims,
                      const std::pair<size_t, size_t> &boxSteps =
                          std::pair<size_t, size_t>(0, 1));

    /**
     * Set the steps for the variable. The pointer passed at
     * reading must be able to hold enough memory to store multiple steps in a
     * single read. For writing it changes the time step
     * @param boxSteps {startStep, countStep}
     */
    void SetStepSelection(const std::pair<size_t, size_t> &boxSteps);

    /**
     * Set the local dimension and global offset of the variable using a
     * selection
     * Only bounding boxes are allowed
     */
    void SetMemorySelection(const std::pair<Dims, Dims> &boxDims);

    size_t GetAvailableStepsStart() const;

    size_t GetAvailableStepsCount() const;

    /**
     * Pushed a new transform to a sequence of transports
     * @param transform reference to an object derived from the Transform class
     * @param parameters transform specific parameters
     * @return transformID handler
     */
    unsigned int AddTransform(Transform &transform,
                              const Params &parameters = Params()) noexcept;

    void ResetTransformParameters(const unsigned int transformIndex,
                                  const Params &parameters = Params());

    /** Clears out the transform sequence defined by AddTransform */
    void ClearTransforms() noexcept;

    /** Self-check dims according to type, called from Engine before Write
     * @param hint extra debugging info for the exception */
    void CheckDimensions(const std::string hint) const;

protected:
    const bool m_DebugMode = false;

    Dims m_MemoryStart; ///< offset of memory selection
    Dims m_MemoryCount; ///< subset of m_Shape (e.g. remove ghost points)

    unsigned int m_DeferredCounter = 0;

    void InitShapeType();

    /** Self-check dims according to type, called right after DefineVariable and
     *  SetSelection.
     * @param hint extra debugging info for the exception */
    void CheckDimensionsCommon(const std::string hint) const;
};

} // end namespace adios2

#endif /* ADIOS2_CORE_VARIABLEBASE_H_ */
