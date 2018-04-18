/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * cxx03IO.cpp
 *
 *  Created on: Apr 5, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "cxx03IO.h"
#include "cxx03IO.tcc"

namespace adios2
{
namespace cxx03
{

IO::IO(adios2_io &io) : m_IO(io) {}

IO::~IO() {}

#define declare_template_instantiation(T)                                      \
    template Variable<T> IO::DefineVariable<T>(const std::string &,            \
                                               const Dims &, const Dims &,     \
                                               const Dims &, const bool, T *); \
    template Variable<T> IO::InquireVariable<T>(const std::string &);

ADIOS2_FOREACH_CXX03_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

#define declare_template_instantiation(T)                                      \
    template Attribute<T> IO::DefineAttribute<T>(const std::string &,          \
                                                 const T *, const size_t);     \
    template Attribute<T> IO::DefineAttribute<T>(const std::string &,          \
                                                 const T &);

ADIOS2_FOREACH_CXX03_ATTRIBUTE_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace cxx03
} // end namespace adios2
