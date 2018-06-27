/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * cxxx98Variable.cpp
 *
 *  Created on: Apr 10, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "cxx98Variable.h"

#include <adios2_c.h>

namespace adios2
{
namespace cxx98
{

#define declare_type(T)                                                        \
                                                                               \
    template <>                                                                \
    Variable<T>::Variable() : m_Variable(NULL)                                 \
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
        return m_Variable == NULL ? false : true;                              \
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
    }                                                                          \
                                                                               \
    template <>                                                                \
    size_t Variable<T>::SelectionSize() const                                  \
    {                                                                          \
        return adios2_selection_size(m_Variable);                              \
    }                                                                          \
                                                                               \
    template <>                                                                \
    std::string Variable<T>::Name() const                                      \
    {                                                                          \
        size_t size = 0;                                                       \
        const char *name = adios2_variable_name(m_Variable, &size);            \
        return std::string(name, size);                                        \
    }                                                                          \
                                                                               \
    template <>                                                                \
    size_t Variable<T>::Sizeof() const                                         \
    {                                                                          \
        return sizeof(T);                                                      \
    }                                                                          \
                                                                               \
    template <>                                                                \
    Dims Variable<T>::Shape() const                                            \
    {                                                                          \
        const size_t size = adios2_variable_ndims(m_Variable);                 \
        const size_t *shape = adios2_variable_shape(m_Variable);               \
        return Dims(shape, shape + size);                                      \
    }                                                                          \
                                                                               \
    template <>                                                                \
    Dims Variable<T>::Start() const                                            \
    {                                                                          \
        const size_t size = adios2_variable_ndims(m_Variable);                 \
        const size_t *start = adios2_variable_start(m_Variable);               \
        return Dims(start, start + size);                                      \
    }                                                                          \
                                                                               \
    template <>                                                                \
    Dims Variable<T>::Count() const                                            \
    {                                                                          \
        const size_t size = adios2_variable_ndims(m_Variable);                 \
        const size_t *count = adios2_variable_count(m_Variable);               \
        return Dims(count, count + size);                                      \
    }                                                                          \
                                                                               \
    template <>                                                                \
    size_t Variable<T>::Steps() const                                          \
    {                                                                          \
        return adios2_variable_steps(m_Variable);                              \
    }                                                                          \
                                                                               \
    template <>                                                                \
    size_t Variable<T>::StepsStart() const                                     \
    {                                                                          \
        return adios2_variable_steps_start(m_Variable);                        \
    }                                                                          \
                                                                               \
    template <>                                                                \
    adios2::cxx98::ShapeID Variable<T>::ShapeID() const                        \
    {                                                                          \
        const adios2_shapeid shapeid = adios2_variable_shapeid(m_Variable);    \
        switch (shapeid)                                                       \
        {                                                                      \
        case (adios2_shapeid_global_value):                                    \
            return adios2::cxx98::ShapeID::GlobalValue;                        \
        case (adios2_shapeid_global_array):                                    \
            return adios2::cxx98::ShapeID::GlobalArray;                        \
        case (adios2_shapeid_joined_array):                                    \
            return adios2::cxx98::ShapeID::JoinedArray;                        \
        case (adios2_shapeid_local_value):                                     \
            return adios2::cxx98::ShapeID::LocalValue;                         \
        case (adios2_shapeid_local_array):                                     \
            return adios2::cxx98::ShapeID::LocalArray;                         \
        }                                                                      \
    }                                                                          \
                                                                               \
    template <>                                                                \
    Variable<T>::Variable(adios2_variable *variable) : m_Variable(variable)    \
    {                                                                          \
    }

ADIOS2_FOREACH_CXX98_TYPE_1ARG(declare_type)
#undef declare_type

} // end namespace cxx98
} // end namespace adios2
