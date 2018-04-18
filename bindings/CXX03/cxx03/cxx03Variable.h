/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * cxx03Variable.h
 *
 *  Created on: Apr 5, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef BINDINGS_CXX03_CXX03_CXX03VARIABLE_H_
#define BINDINGS_CXX03_CXX03_CXX03VARIABLE_H_

#include <string>

#include <adios2_c.h>

#include "cxx03types.h"

namespace adios2
{
namespace cxx03
{

template <class T>
class Variable
{
public:
    adios2_variable *m_Variable;

    Variable<T>(adios2_variable *variable);

    ~Variable<T>();

    operator bool() const;

    void SetSelection(const Dims &start, const Dims &count);

    void SetStepSelection(const size_t stepStart, const size_t stepCount);
};

} // end namespace cxx03
} // end namespace adios2

#endif /* BINDINGS_CXX03_CXX03_CXX03VARIABLE_H_ */
