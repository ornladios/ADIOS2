/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Variable.h :
 *
 *  Created on: Jun 4, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_CXX11_CXX11_VARIABLE_H_
#define ADIOS2_BINDINGS_CXX11_CXX11_VARIABLE_H_

#include "Operator.h"

#include "adios2/ADIOSTypes.h"
#include "adios2/core/Variable.h"

namespace adios2
{

/// \cond EXCLUDE_FROM_DOXYGEN
// forward declare
class IO;     // friend
class Engine; // friend

namespace core
{

template <class T>
class Variable; // private implementation
}
/// \endcond

template <class T>
class Variable
{

    friend class IO;
    friend class Engine;

public:
    /**
     * Empty (default) constructor, use it as a placeholder for future
     * variables from IO:DefineVariable<T> or IO:InquireVariable<T>.
     * Can be used with STL containers.
     */
    Variable<T>() = default;

    /** Default, using RAII STL containers */
    ~Variable<T>() = default;

    /** Checks if object is valid, e.g. if( variable ) { //..valid } */
    explicit operator bool() const noexcept;

    /**
     * Set new shape, care must be taken when reading back the variable for
     * different steps. Only applies to Global arrays.
     * @param shape new shape dimensions array
     */
    void SetShape(const adios2::Dims &shape);

    /**
     * Read mode only. Required for reading local variables, ShapeID() =
     * ShapeID::LocalArray or ShapeID::LocalValue. For Global Arrays it will Set
     * the appropriate Start and Count Selection for the global array
     * coordinates.
     * @param blockID: variable block index defined at write time. Blocks can be
     * inspected with bpls -D variableName
     */
    void SetBlockSelection(const size_t blockID);

    /**
     * Sets a variable selection modifying current {start, count}
     * Count is the dimension from Start point
     * @param selection input {start, count}
     */
    void SetSelection(const adios2::Box<adios2::Dims> &selection);

    /**
     * Set the local start (offset) point to the memory pointer passed at Put
     * and the memory local dimensions (count). Used for non-contiguous memory
     * writes and reads (e.g. multidimensional ghost-cells).
     * Currently not working for calls to Get.
     * @param memorySelection {memoryStart, memoryCount}
     * <pre>
     * 		memoryStart: relative local offset of variable.start to the
     * contiguous memory pointer passed at Put from which data starts. e.g. if
     * variable.Start() = {rank*Ny,0} and there is 1 ghost cell per dimension,
     * then memoryStart = {1,1}
     * 		memoryCount: local dimensions for the contiguous memory pointer
     * passed at Put, e.g. if there is 1 ghost cell per dimension and
     * variable.Count() = {Ny,Nx}, then memoryCount = {Ny+2,Nx+2}
     * </pre>
     */
    void SetMemorySelection(const adios2::Box<adios2::Dims> &memorySelection);

    /**
     * Sets a step selection modifying current startStep, countStep
     * countStep is the number of steps from startStep point
     * @param stepSelection input {startStep, countStep}
     */
    void SetStepSelection(const adios2::Box<size_t> &stepSelection);

    /**
     * Returns the number of elements required for pre-allocation based on
     * current count and stepsCount
     * @return elements of type T required for pre-allocation
     */
    size_t SelectionSize() const;

    /**
     * Inspects Variable name
     * @return name
     */
    std::string Name() const;

    /**
     * Inspects Variable type
     * @return type string literal containing the type: double, float, unsigned
     * int, etc.
     */
    std::string Type() const;

    /**
     * Inspects size of the current element type, sizeof(T)
     * @return sizeof(T) for current system
     */
    size_t Sizeof() const;

    /**
     * Inspects shape id for current variable
     * @return from enum adios2::ShapeID
     */
    adios2::ShapeID ShapeID() const;

    /**
     * Inspects current shape
     * @return shape vector
     */
    adios2::Dims Shape() const;

    /**
     * Inspects current start point
     * @return start vector
     */
    adios2::Dims Start() const;

    /**
     * Inspects current count from start
     * @return count vector
     */
    adios2::Dims Count() const;

    /**
     * For read mode, inspect the number of available steps
     * @return available steps
     */
    size_t Steps() const;

    /**
     * For read mode, inspect the start step for available steps
     * @return available start step
     */
    size_t StepsStart() const;

    /**
     * For read mode, retrieve current BlockID, default = 0 if not set with
     * SetBlockID
     * @return current block id
     */
    size_t BlockID() const;

    /**
     * EXPERIMENTAL: carries information about an Operation added with
     * AddOperation
     */
    struct Operation
    {
        /** Operator associated with this operation */
        const adios2::Operator Op;
        /** Parameters settings for this operation */
        const adios2::Params Parameters;
        /** Information associated with this operation */
        const adios2::Params Info;
    };

    /**
     * EXPERIMENTAL: Adds operation and parameters to current Variable object
     * @param op operator to be added
     * @param parameters key/value settings particular to the Variable, not to
     * be confused by op own parameters
     * @return operation index handler in Operations()
     */
    size_t AddOperation(const Operator op,
                        const adios2::Params &parameters = adios2::Params());

    /**
     * EXPERIMENTAL: inspects current operators added with AddOperator
     * @return vector of Variable<T>::OperatorInfo
     */
    std::vector<Operation> Operations() const;

    /**
     * Read mode only: return the absolute minimum for current variable
     * @return minimum
     */
    T Min() const;

    /**
     * Read mode only: return the absolute maximum for current variable
     * @return maximum
     */
    T Max() const;

    /** Contains sub-block information for a particular Variable<T> */
    struct Info
    {
        adios2::Dims Start;   ///< block start
        adios2::Dims Count;   ///< block count
        T Min = T();          ///< block Min, if IsValue is false
        T Max = T();          ///< block Max, if IsValue is false
        T Value = T();        ///< block Value, if IsValue is true
        bool IsValue = false; ///< true: value, false: array
        size_t BlockID = -1;  ///< block ID for block selections
        const T *Data() const
        {
            return m_Info ? (m_Info->BufferP ? m_Info->BufferP
                                             : m_Info->BufferV.data())
                          : nullptr;
        }; ///< block data for block selections. Provided by core Info.
        // allow Engine to set m_Info
        friend class Engine;

    private:
        const typename core::Variable<T>::Info *m_Info;
    };

private:
    Variable<T>(core::Variable<T> *variable);
    core::Variable<T> *m_Variable = nullptr;
};

} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX11_CXX11_VARIABLE_H_ */
