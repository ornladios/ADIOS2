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

    ShapeID m_ShapeID = ShapeID::Unknown; ///< see shape types in ADIOSTypes.h
    size_t m_BlockID = 0; ///< current block ID for local variables, global = 0
    SelectionType m_SelectionType = SelectionType::BoundingBox;

    bool m_SingleValue = false; ///< true: single value, false: array
    Dims m_Shape;               ///< total dimensions across MPI
    Dims m_Start;               ///< starting point (offsets) in global shape
    Dims m_Count;               ///< dimensions from m_Start in global shape

    Dims m_MemoryStart; ///< start offset
    Dims m_MemoryCount; ///< local dimensions

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

    std::map<size_t, Dims> m_AvailableShapes;

    VariableBase(const std::string &name, const std::string type,
                 const size_t elementSize, const Dims &shape, const Dims &start,
                 const Dims &count, const bool constantShape,
                 const bool debugMode);

    virtual ~VariableBase() = default;

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
     * Use at read only for local variables
     * @param blockID
     */
    void SetBlockSelection(const size_t blockID);

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
     * Set local offset and dimensions to memory passed at Put
     */
    void SetMemorySelection(const Box<Dims> &memorySelection);

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

    /**
     * Resets m_StepsStart and m_StepsCount. Must be called in BeginStep
     */
    void ResetStepsSelection(const bool zeroStart) noexcept;

    /**
     * Checks if variable has a conflict to be accessed as a stream and
     * random-access (SetStepSelection has been called)
     * @param hint improve exception error message
     * @throws std::invalid_argument if random access and streaming are called
     */
    void CheckRandomAccessConflict(const std::string hint) const;

    /**
     * Dynamically update the shape of a global variable
     * @return
     */
    Dims Shape() const;

protected:
    const bool m_DebugMode = false;
    bool m_ConstantDims = false; ///< true: fix m_Shape, m_Start, m_Count

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
