/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * cxx98Variable.tcc
 *
 *  Created on: Apr 6, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef BINDINGS_CXX98_CXX98_CXX98VARIABLE_TCC_
#define BINDINGS_CXX98_CXX98_CXX98VARIABLE_TCC_

#include "cxx98Variable.h"
#include "cxx98types.h"

namespace adios2
{
namespace cxx98
{

#define declare_type(T)                                                        \
                                                                               \
    template <>                                                                \
    Variable<T>::Variable(adios2_variable *variable) : m_Variable(variable)    \
    {                                                                          \
    }                                                                          \
                                                                               \
    template <>                                                                \
    Variable<T>::~Variable()                                                   \
    {                                                                          \
    }                                                                          \
                                                                               \
    template <>                                                                \
    Variable<T>::operator bool() const                                         \
    {                                                                          \
        if (m_Variable == NULL)                                                \
        {                                                                      \
            return false;                                                      \
        }                                                                      \
        return true;                                                           \
    }                                                                          \
                                                                               \
    template <>                                                                \
    void Variable<T>::SetSelection(const Dims &start, const Dims &count)       \
    {                                                                          \
        adios2_set_selection(m_Variable, count.size(),                         \
                             start.empty() ? NULL : &start[0],                 \
                             count.empty() ? NULL : &count[0]);                \
    }                                                                          \
                                                                               \
    template <>                                                                \
    void Variable<T>::SetStepSelection(const size_t stepStart,                 \
                                       const size_t stepCount)                 \
    {                                                                          \
        adios2_set_step_selection(m_Variable, stepStart, stepCount);           \
    }

ADIOS2_FOREACH_CXX98_TYPE_1ARG(declare_type)
#undef declare_type

} // end namespace cxx98
} // end namespace adios2

#endif /* BINDINGS_CXX98_CXX98_CXX98VARIABLE_TCC_ */
