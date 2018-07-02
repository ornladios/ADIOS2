/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * cxx98Variable.h
 *
 *  Created on: Apr 5, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_CXX98_CXX98_CXX98VARIABLE_H_
#define ADIOS2_BINDINGS_CXX98_CXX98_CXX98VARIABLE_H_

#include <string>

#include "cxx98types.h"

// forward declaration
struct adios2_variable;

namespace adios2
{
namespace cxx98
{

// forward declare
class IO;
class Engine;

template <class T>
class Variable
{
    friend class IO;
    friend class Engine;

public:
    Variable<T>();
    ~Variable<T>();

    operator bool() const;

    /**
     * Sets a variable selection modifying current {start, count}
     * Count is the dimension from Start point
     * @param selection input {start, count}
     */
    void SetSelection(const Dims &start, const Dims &count);

    /**
     * Sets a step selection modifying current startStep, countStep
     * countStep is the number of steps from startStep point
     * @param stepSelection input {startStep, countStep}
     */
    void SetStepSelection(const size_t stepStart, const size_t stepCount);

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
    // std::string Type() const;

    /**
     * Inspects size of the current element type, sizeof(T)
     * @return sizeof(T) for current system
     */
    size_t Sizeof() const;

    /**
     * Inspects shape id for current variable
     * @return from enum adios2::ShapeID
     */
    adios2::cxx98::ShapeID ShapeID() const;

    /**
     * Inspects current shape
     * @return shape vector
     */
    Dims Shape() const;

    /**
     * Inspects current start point
     * @return start point vector
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

private:
    Variable<T>(adios2_variable *variable);
    adios2_variable *m_Variable;
};

} // end namespace cxx98
} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX98_CXX98_CXX98VARIABLE_H_ */
