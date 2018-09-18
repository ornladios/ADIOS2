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
#include "adios2/core/Operator.h"

namespace adios2
{
namespace core
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

    ShapeID m_ShapeID;          ///< see shape types in ADIOSTypes.h
    bool m_SingleValue = false; ///< true: single value, false: array
    Dims m_Shape;               ///< total dimensions across MPI
    Dims m_Start;               ///< starting point (offsets) in global shape
    Dims m_Count;               ///< dimensions from m_Start in global shape

    /** Global array was written as Joined array, so read accordingly */
    bool m_ReadAsJoined = false;
    /** Global array was written as Local value, so read accordingly */
    bool m_ReadAsLocalValue = false;

    /** For read mode. true: SetStepSelection was used, only valid in File based
     * engines, false: streaming */
    bool m_RandomAccess = false;

    /** used in streaming mode, true: first variable encounter, false: variable
     * already encountered in previous step */
    bool m_FirstStreamingStep = true;

    /** Operators metadata info */
    struct Operation
    {
        /** reference to object derived from Operator class,
         *  needs a pointer to enable assignment operator (C++ class) */
        core::Operator *Op;
        /** Variable specific parameters */
        Params Parameters;
        /** resulting information from executing Operation (e.g. buffer size) */
        Params Info;
    };

    /** Registered transforms */
    std::vector<Operation> m_Operations;

    size_t m_AvailableStepsStart = 0;
    size_t m_AvailableStepsCount = 0;

    size_t m_StepsStart = 0;
    size_t m_StepsCount = 1;

    /** Index Metadata Position in a serial metadata buffer */
    size_t m_IndexStart = 0;

    /** Index to Step and blocks' (inside a step) characteristics position in a
     * serial metadata buffer
     * <pre>
     * key: step number (time_index in bp3 format)
     * value:  vector of block starts for that step
     * </pre>
     * */
    std::map<size_t, std::vector<size_t>> m_AvailableStepBlockIndexOffsets;

    /** wildcard memory space used for contiguous memory read */
    std::map<size_t, std::vector<char>> m_RawMemory;

    VariableBase(const std::string &name, const std::string type,
                 const size_t elementSize, const Dims &shape, const Dims &start,
                 const Dims &count, const bool constantShape,
                 const bool debugMode);

    virtual ~VariableBase() = default;

    /**
     * Returns the payload size in bytes
     * @return TotalSize * m_ElementSize
     */
    size_t PayloadSize() const noexcept;

    /**
     * Returns the total number of elements
     * @return number of elements
     */
    size_t TotalSize() const noexcept;

    /**
     * Set new shape
     * @param shape input shape to be applied to this variable
     */
    void SetShape(const adios2::Dims &shape);

    /**
     * Set new start and count dimensions
     * @param boxDims = {start, count}
     */
    void SetSelection(const Box<Dims> &boxDims);

    /**
     * Set the steps for the variable. The pointer passed at
     * reading must be able to hold enough memory to store multiple steps in a
     * single read. For writing it changes the time step
     * @param boxSteps {startStep, countStep}
     */
    void SetStepSelection(const Box<size_t> &boxSteps);

    /**
     * Set the local dimension and global offset of the variable using a
     * selection
     * Only bounding boxes are allowed
     */
    void SetMemorySelection(const Box<Dims> &boxDims);

    size_t GetAvailableStepsStart() const;

    size_t GetAvailableStepsCount() const;

    /**
     * Adds an operation to this variable.
     * @param op reference to an Operator object
     * @param parameters operation specific parameters
     * @return operator handler
     */
    size_t AddOperation(core::Operator &op,
                        const Params &parameters = Params()) noexcept;

    /**
     * Sets a parameter by key/value in an existing operation from AddOperation
     * @param operationID returned handler form AddOperation
     * @param key input parameter key
     * @param value input parameter value
     */
    void SetOperationParameter(const size_t operationID, const std::string key,
                               const std::string value);

    /** Self-check dims according to type, called from Engine before Write
     * @param hint extra debugging info for the exception */
    void CheckDimensions(const std::string hint) const;

    /**
     * Returns the minimum required allocation for the current selection
     * @return memory size to be allocated by a pointer/vector to read this
     * variable
     */
    size_t SelectionSize() const noexcept;

    bool IsConstantDims() const noexcept;
    void SetConstantDims() noexcept;

    bool IsValidStep(const size_t step) const noexcept;

protected:
    const bool m_DebugMode = false;
    bool m_ConstantDims = false; ///< true: fix m_Shape, m_Start, m_Count

    Dims m_MemoryStart; ///< offset of memory selection
    Dims m_MemoryCount; ///< subset of m_Shape (e.g. remove ghost points)

    unsigned int m_DeferredCounter = 0;

    void InitShapeType();

    /** Self-check dims according to type, called right after DefineVariable and
     *  SetSelection.
     * @param hint extra debugging info for the exception */
    void CheckDimensionsCommon(const std::string hint) const;
};

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_VARIABLEBASE_H_ */
