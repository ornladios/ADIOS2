/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2fstream.cpp
 *
 *  Created on: Mar 5, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "ADIOS2fstream.h"
#include "ADIOS2fstream.tcc"

#include "adios2/ADIOSMPI.h"

namespace adios2
{

fstream::fstream(const std::string &name, const openmode mode, MPI_Comm comm,
                 const std::string engineType)
: m_Stream(new core::Stream(name, ToMode(mode), comm, engineType, "C++"))
{
}

fstream::fstream(const std::string &name, const openmode mode,
                 const std::string engineType)
: fstream(name, mode, MPI_COMM_SELF, engineType)
{
}

fstream::fstream(const std::string &name, const openmode mode, MPI_Comm comm,
                 const std::string &configFile,
                 const std::string ioInConfigFile)
: m_Stream(new core::Stream(name, ToMode(mode), comm, configFile,
                            ioInConfigFile, "C++"))
{
}

fstream::fstream(const std::string &name, const openmode mode,
                 const std::string &configFile,
                 const std::string ioInConfigFile)
: fstream(name, mode, MPI_COMM_SELF, configFile, ioInConfigFile)
{
}

fstream::~fstream() = default;

void fstream::open(const std::string &name, const openmode mode, MPI_Comm comm,
                   const std::string engineType)
{
    if (m_Stream)
    {
        throw std::invalid_argument("ERROR: adios2::fstream with name " + name +
                                    " is already opened, in call to open");
    }

    m_Stream = std::unique_ptr<core::Stream>(
        new core::Stream(name, ToMode(mode), comm, engineType, "C++"));
}

void fstream::open(const std::string &name, const openmode mode,
                   const std::string engineType)
{
    open(name, mode, MPI_COMM_SELF, engineType);
}

void fstream::open(const std::string &name, const openmode mode, MPI_Comm comm,
                   const std::string configFile,
                   const std::string ioInConfigFile)
{
    if (m_Stream)
    {
        throw std::invalid_argument("ERROR: adios2::fstream with name " + name +
                                    " is already opened, in call to open");
    }

    m_Stream = std::unique_ptr<core::Stream>(new core::Stream(
        name, ToMode(mode), comm, configFile, ioInConfigFile, "C++"));
}

void fstream::open(const std::string &name, const openmode mode,
                   const std::string configFile,
                   const std::string ioInConfigFile)
{
    open(name, mode, MPI_COMM_SELF, configFile, ioInConfigFile);
}

fstream::operator bool() const noexcept
{
    if (!m_Stream)
    {
        return false;
    }

    return true;
}

void fstream::close()
{
    m_Stream->Close();
    m_Stream.reset();
}

// bool getstep(adios2::fstream &stream,
//             std::map<std::string, Params> &availableVariables,
//             std::map<std::string, Params> &availableAttributes)
//{
//    return stream.m_Stream->GetStep();
//}
//
// bool getstep(adios2::fstream &stream,
//             std::map<std::string, Params> &availableVariables)
//{
//    return stream.m_Stream->GetStep();
//}

bool getstep(adios2::fstream &stream) { return stream.m_Stream->GetStep(); }

size_t fstream::currentstep() const noexcept { return m_Stream->CurrentStep(); }

adios2::Mode fstream::ToMode(const openmode mode) const noexcept
{
    adios2::Mode modeCpp = adios2::Mode::Undefined;
    switch (mode)
    {
    case (openmode::out):
        modeCpp = adios2::Mode::Write;
        break;
    case (openmode::in):
        modeCpp = adios2::Mode::Read;
        break;
    case (openmode::app):
        modeCpp = adios2::Mode::Append;
        break;
    }
    return modeCpp;
}

#define declare_template_instantiation(T)                                      \
    template void fstream::write<T>(const std::string &, const T *,            \
                                    const Dims &, const Dims &, const Dims &,  \
                                    const bool);                               \
                                                                               \
    template void fstream::write<T>(const std::string &, const T &,            \
                                    const bool);                               \
                                                                               \
    template std::vector<T> fstream::read<T>(const std::string &);             \
                                                                               \
    template std::vector<T> fstream::read<T>(const std::string &,              \
                                             const Dims &, const Dims &);      \
                                                                               \
    template std::vector<T> fstream::read<T>(const std::string &,              \
                                             const Dims &, const Dims &,       \
                                             const size_t, const size_t);      \
                                                                               \
    template void fstream::read<T>(const std::string &, T *);                  \
                                                                               \
    template void fstream::read<T>(const std::string &name, T &);              \
                                                                               \
    template void fstream::read<T>(const std::string &name, T &,               \
                                   const size_t);                              \
                                                                               \
    template void fstream::read<T>(const std::string &, T *, const Dims &,     \
                                   const Dims &);                              \
                                                                               \
    template void fstream::read<T>(const std::string &, T *, const Dims &,     \
                                   const Dims &, const size_t, const size_t);

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace adios2
