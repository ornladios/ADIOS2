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
                 const std::string engineType, const Params &parameters,
                 const vParams &transportParameters)
{
    if (mode == openmode::out)
    {
        m_Stream = std::make_shared<core::Stream>(name, Mode::Write, comm,
                                                  engineType, parameters,
                                                  transportParameters, "C++");
    }
    else if (mode == openmode::app)
    {
        m_Stream = std::make_shared<core::Stream>(name, Mode::Append, comm,
                                                  engineType, parameters,
                                                  transportParameters, "C++");
    }
    else if (mode == openmode::in)
    {
        m_Stream = std::make_shared<core::Stream>(name, Mode::Read, comm,
                                                  engineType, parameters,
                                                  transportParameters, "C++");
    }
}

fstream::fstream(const std::string &name, const openmode mode, MPI_Comm comm,
                 const std::string configFile, const std::string ioInConfigFile)
{
    if (mode == openmode::out)
    {
        m_Stream = std::make_shared<core::Stream>(
            name, Mode::Write, comm, configFile, ioInConfigFile, "C++");
    }
    else if (mode == openmode::app)
    {
        m_Stream = std::make_shared<core::Stream>(
            name, Mode::Append, comm, configFile, ioInConfigFile, "C++");
    }
    else if (mode == openmode::in)
    {
        m_Stream = std::make_shared<core::Stream>(
            name, Mode::Read, comm, configFile, ioInConfigFile, "C++");
    }
}

fstream::fstream(const std::string &name, const openmode mode,
                 const std::string engineType, const Params &parameters,
                 const vParams &transportParameters)
{
    if (mode == openmode::out)
    {
        m_Stream = std::make_shared<core::Stream>(name, Mode::Write, engineType,
                                                  parameters,
                                                  transportParameters, "C++");
    }
    else if (mode == openmode::app)
    {
        m_Stream = std::make_shared<core::Stream>(name, Mode::Append,
                                                  engineType, parameters,
                                                  transportParameters, "C++");
    }
    else if (mode == openmode::in)
    {
        m_Stream = std::make_shared<core::Stream>(name, Mode::Read, engineType,
                                                  parameters,
                                                  transportParameters, "C++");
    }
}

fstream::fstream(const std::string &name, const openmode mode,
                 const std::string configFile, const std::string ioInConfigFile)
{
    if (mode == openmode::out)
    {
        m_Stream = std::make_shared<core::Stream>(name, Mode::Write, configFile,
                                                  ioInConfigFile, "C++");
    }
    else if (mode == openmode::app)
    {
        m_Stream = std::make_shared<core::Stream>(
            name, Mode::Append, configFile, ioInConfigFile, "C++");
    }
    else if (mode == openmode::in)
    {
        m_Stream = std::make_shared<core::Stream>(name, Mode::Read, configFile,
                                                  ioInConfigFile, "C++");
    }
}

fstream::fstream() : m_Stream(std::make_shared<core::Stream>("C++")) {}

void fstream::open(const std::string &name, const openmode mode, MPI_Comm comm,
                   const std::string engineType, const Params &parameters,
                   const vParams &transportParameters)
{
    m_Stream->Open(name, static_cast<Mode>(mode), comm, engineType, parameters,
                   transportParameters);
}

void fstream::open(const std::string &name, const openmode mode, MPI_Comm comm,
                   const std::string configFile,
                   const std::string ioInConfigFile)
{
    m_Stream->Open(name, static_cast<Mode>(mode), comm, configFile,
                   ioInConfigFile);
}

void fstream::open(const std::string &name, const openmode mode,
                   const std::string engineType, const Params &parameters,
                   const vParams &transportParameters)
{
    open(name, mode, MPI_COMM_SELF, engineType, parameters,
         transportParameters);
}

void fstream::open(const std::string &name, const openmode mode,
                   const std::string configFile,
                   const std::string ioInConfigFile)
{
    open(name, mode, MPI_COMM_SELF, configFile, ioInConfigFile);
}

bool fstream::is_open() const noexcept { return m_Stream->m_IsOpen; }

bool fstream::eof() const noexcept
{
    bool eof = false;

    if (m_Stream->m_Status == StepStatus::EndOfStream)
    {
        eof = true;
    }

    return eof;
}

fstream::operator bool() const noexcept { return eof(); }

void fstream::close() { m_Stream->Close(); }

#define declare_template_instantiation(T)                                      \
    template void fstream::write<T>(const std::string &, const T *,            \
                                    const Dims &, const Dims &, const Dims &,  \
                                    const bool);                               \
                                                                               \
    template void fstream::write<T>(const std::string &, const T &,            \
                                    const bool);                               \
                                                                               \
    template std::vector<T> fstream::read<T>(const std::string &, const bool); \
                                                                               \
    template std::vector<T> fstream::read<T>(                                  \
        const std::string &, const Dims &, const Dims &, const bool);          \
                                                                               \
    template std::vector<T> fstream::read<T>(const std::string &,              \
                                             const Dims &, const Dims &,       \
                                             const size_t, const size_t);      \
                                                                               \
    template void fstream::read<T>(const std::string &, T *, const bool);      \
                                                                               \
    template void fstream::read<T>(const std::string &name, T &, const bool);  \
                                                                               \
    template void fstream::read<T>(const std::string &name, T &,               \
                                   const size_t);                              \
                                                                               \
    template void fstream::read<T>(const std::string &, T *, const Dims &,     \
                                   const Dims &, const bool);                  \
                                                                               \
    template void fstream::read<T>(const std::string &, T *, const Dims &,     \
                                   const Dims &, const size_t, const size_t);

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace adios2
