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

template <class T>
class Span; // private implementation
}
/// \endcond

namespace detail
{
/**
 * Span<T> class that allows exposing buffer memory to the user
 */
template <class T>
class Span
{
    using IOType = typename TypeInfo<T>::IOType;

public:
    /** Span can only be created by an Engine */
    Span() = delete;
    /** Span can't be copied */
    Span(const Span &) = delete;
    /** Span can be moved */
    Span(Span &&) = default;
    /**
     * Memory is not owned, using RAII for members
     */
    ~Span() = default;

    /** Span can't be copied */
    Span &operator=(const Span &) = delete;
    /** Span can only be moved */
    Span &operator=(Span &&) = default;

    /**
     * size of the span based on Variable block Count
     * @return number of elements
     */
    size_t size() const noexcept;

    /**
     * Pointer to span data, can be modified if new spans are added
     * Follows rules of std::vector iterator invalidation.
     * Call again to get an updated pointer.
     * @return pointer to data
     */
    T *data() const noexcept;

    /**
     * Safe access operator that checks bounds and throws an exception
     * @param position input offset from 0 = data()
     * @return span element at input position
     * @throws std::invalid_argument if out of bounds
     */
    T &at(const size_t position);

    /**
     * Safe const access operator that checks bounds and throws an exception
     * @param position input offset from 0 = data()
     * @return span const (read-only) element at input position
     * @throws std::invalid_argument if out of bounds
     */
    const T &at(const size_t position) const;

    /**
     * Access operator (unsafe without check overhead)
     * @param position input offset from 0 = data()
     * @return span element at input position
     */
    T &operator[](const size_t position);

    /**
     * Access const operator (unsafe without check overhead)
     * @param position input offset from 0 = data()
     * @return span const (read-only) element at input position
     */
    const T &operator[](const size_t position) const;

    // engine allowed to set m_Span
    friend class adios2::Engine;

    // Custom iterator class from:
    // https://gist.github.com/jeetsukumaran/307264#file-custom_iterator-cpp-L26
    ADIOS2_CLASS_iterator;

    // Custom iterator class functions from:
    // https://gist.github.com/jeetsukumaran/307264#file-custom_iterator-cpp-L26
    ADIOS2_iterators_functions(data(), size());

private:
    using CoreSpan = core::Span<IOType>;
    Span(CoreSpan *span);
    CoreSpan *m_Span = nullptr;
};

} // end namespace detail

template <class T>
class Variable
{
    using IOType = typename TypeInfo<T>::IOType;

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
     * Inspects shape in global variables
     * @param step input for a particular Shape if changing over time. If
     * default, either return absolute or in streaming mode it returns the shape
     * for the current engine step
     * @return shape vector
     */
    adios2::Dims Shape(const size_t step = adios2::EngineCurrentStep) const;

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
     *Adds operation and parameters to current Variable object
     * @param op operator to be added
     * @param parameters key/value settings particular to the Variable, not to
     * be confused by op own parameters
     * @return operation index handler in Operations()
     */
    size_t AddOperation(const Operator op,
                        const adios2::Params &parameters = adios2::Params());

    /**
     * Inspects current operators added with AddOperator
     * @return vector of Variable<T>::OperatorInfo
     */
    std::vector<Operation> Operations() const;

    /**
     * Read mode only: return minimum and maximum values for current variable at
     * a step. For streaming mode (BeginStep/EndStep): use default (leave empty)
     * for current Engine Step
     * At random access mode (File Engines only): default = absolute MinMax
     * @param step input step
     * @return pair.first = min pair.second = max
     */
    std::pair<T, T> MinMax(const size_t step = adios2::DefaultSizeT) const;

    /**
     * Read mode only: return minimum values for current variable at
     * a step. For streaming mode (within BeginStep/EndStep): use default (leave
     * empty) for current Engine Step
     * At random access mode (File Engines only): default = absolute MinMax
     * @param step input step
     * @return variable minimum
     */
    T Min(const size_t step = adios2::DefaultSizeT) const;

    /**
     * Read mode only: return minimum values for current variable at
     * a step. For streaming mode (within BeginStep/EndStep): use default
     * (leave empty) for current Engine Step
     * At random access mode (File Engines only): default = absolute MinMax
     * @param step input step
     * @return variable minimum
     */
    T Max(const size_t step = adios2::DefaultSizeT) const;

    /** Contains block information for a particular Variable<T> */
    struct Info
    {
        /** block start */
        adios2::Dims Start;
        /** block count */
        adios2::Dims Count;
        /** block Min, if IsValue is false */
        IOType Min = IOType();
        /** block Max, if IsValue is false */
        IOType Max = IOType();
        /** block Value, if IsValue is true */
        IOType Value = IOType();
        /** true: value, false: array */
        bool IsValue = false;
        /** blockID for Block Selection */
        size_t BlockID = 0;
        /** block corresponding step */
        size_t Step = 0;
        /** reference to internal block data (used by inline Engine) */
        const T *Data() const;

        // allow Engine to set m_Info
        friend class Engine;

    private:
        class CoreInfo;
        const CoreInfo *m_Info;
    };

    /**
     * Read mode only and random-access (no BeginStep/EndStep) with file engines
     * only. Allows inspection of variable info on a per relative step (returned
     * vector index)
     * basis
     * @return first vector: relative steps, second vector: blocks info within a
     * step
     */
    std::vector<std::vector<typename Variable<T>::Info>> AllStepsBlocksInfo();

    using Span = adios2::detail::Span<T>;

private:
    Variable<T>(core::Variable<IOType> *variable);
    core::Variable<IOType> *m_Variable = nullptr;

    std::vector<std::vector<typename Variable<T>::Info>> DoAllStepsBlocksInfo();
};

template <typename T>
std::string ToString(const Variable<T> &variable);

} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX11_CXX11_VARIABLE_H_ */
