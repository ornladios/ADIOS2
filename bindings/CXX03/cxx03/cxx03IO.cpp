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

namespace
{

adios2_mode ToMode(const Mode mode)
{
    adios2_mode modeC = adios2_mode_undefined;

    switch (mode)
    {
    case Write:
        modeC = adios2_mode_write;
        break;
    case Read:
        modeC = adios2_mode_read;
        break;
    case Append:
        modeC = adios2_mode_append;
        break;
    default:;
    }

    return modeC;
}
}

IO::IO(adios2_io &io) : m_IO(io) {}

IO::~IO() {}

#ifdef ADIOS2_HAVE_MPI
Engine IO::Open(const std::string &name, const Mode mode, MPI_Comm comm)
{
    return Engine(
        *adios2_open_new_comm(&m_IO, name.c_str(), ToMode(mode), comm));
}
#endif

Engine IO::Open(const std::string &name, const enum Mode mode)
{
    return Engine(*adios2_open(&m_IO, name.c_str(), ToMode(mode)));
}

#define declare_template_instantiation(T)                                      \
    template Variable<T> IO::InquireVariable<T>(const std::string &);

ADIOS2_FOREACH_CXX03_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace cxx03
} // end namespace adios2
