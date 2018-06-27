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

    void SetSelection(const Dims &start, const Dims &count);

    void SetStepSelection(const size_t stepStart, const size_t stepCount);

    size_t SelectionSize() const;
    std::string Name() const;
    // std::string Type() const;
    size_t Sizeof() const;
    adios2::cxx98::ShapeID ShapeID() const;
    Dims Shape() const;
    Dims Start() const;
    Dims Count() const;
    size_t Steps() const;
    size_t StepsStart() const;

private:
    Variable<T>(adios2_variable *variable);
    adios2_variable *m_Variable;
};

} // end namespace cxx98
} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX98_CXX98_CXX98VARIABLE_H_ */
