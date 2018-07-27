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

// forward declare
class IO;     // friend
class Engine; // friend

namespace core
{

template <class T>
class Variable; // private implementation
}

template <class T>
class Variable
{

    friend class IO;
    friend class Engine;

public:
    Variable<T>() = default;
    ~Variable<T>() = default;

    explicit operator bool() const noexcept;

    /**
     * Sets a variable selection modifying current {start, count}
     * Count is the dimension from Start point
     * @param selection input {start, count}
     */
    void SetSelection(const Box<Dims> &selection);

    /**
     * Sets a step selection modifying current startStep, countStep
     * countStep is the number of steps from startStep point
     * @param stepSelection input {startStep, countStep}
     */
    void SetStepSelection(const Box<size_t> &stepSelection);

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
    Dims Shape() const;

    /**
     * Inspects current start point
     * @return start vector
     */
    Dims Start() const;

    /**
     * Inspects current count from start
     * @return count vector
     */
    Dims Count() const;

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
     * EXPERIMENTAL: carries information about an Operation added with
     * AddOperation
     */
    struct Operation
    {
        const Operator Op;
        const Params Parameters;
        const Params Info;
    };

    /**
     * EXPERIMENTAL: Adds operation and parameters to current Variable object
     * @param op operator to be added
     * @param parameters key/value settings particular to the Variable, not to
     * be confused by op own parameters
     * @return operation index handler in Operations()
     */
    size_t AddOperation(const Operator op, const Params &parameters = Params());

    /**
     * EXPERIMENTAL: inspects current operators added with AddOperator
     * @return vector of Variable<T>::OperatorInfo
     */
    std::vector<Operation> Operations() const;

    struct Info
    {
        Dims Start;
        Dims Count;
        T Min = T();
        T Max = T();
        T Value = T();
        bool IsValue;
    };

private:
    Variable<T>(core::Variable<T> *variable);
    core::Variable<T> *m_Variable = nullptr;
};

} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX11_CXX11_VARIABLE_H_ */
