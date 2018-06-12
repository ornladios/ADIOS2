/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Stream.cpp
 *
 *  Created on: Jan 5, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "Stream.h"
#include "Stream.tcc"

#include "adios2/ADIOSMPI.h"

namespace adios2
{
namespace core
{

Stream::Stream(const std::string &name, const Mode mode, MPI_Comm comm,
               const std::string engineType, const Params &parameters,
               const vParams &transportParameters,
               const std::string hostLanguage)
: m_HostLanguage(hostLanguage)
{
    Open(name, mode, comm, engineType, parameters, transportParameters);
}

Stream::Stream(const std::string &name, const Mode mode, MPI_Comm comm,
               const std::string configFile, const std::string ioInConfigFile,
               const std::string hostLanguage)
: m_HostLanguage(hostLanguage)
{
    Open(name, mode, comm, configFile, ioInConfigFile);
}

Stream::Stream(const std::string &name, const Mode mode,
               const std::string engineType, const Params &parameters,
               const vParams &transportParameters,
               const std::string hostLanguage)
: Stream(name, mode, MPI_COMM_SELF, engineType, parameters, transportParameters,
         hostLanguage)
{
}

Stream::Stream(const std::string &name, const Mode mode,
               const std::string configFile, const std::string ioInConfigFile,
               const std::string hostLanguage)
: Stream(name, mode, MPI_COMM_SELF, configFile, ioInConfigFile, hostLanguage)
{
}

Stream::Stream(const std::string hostLanguage) : m_HostLanguage(hostLanguage) {}

void Stream::Open(const std::string &name, const Mode mode, MPI_Comm comm,
                  const std::string engineType, const Params &parameters,
                  const vParams &transportParameters)
{
    ThrowIfOpen(name + ", in call to constructor or open\n");
    m_Name = name;
    m_ADIOS = std::make_shared<ADIOS>(comm, DebugON, m_HostLanguage);
    m_IO = &m_ADIOS->DeclareIO(name);
    m_IO->SetEngine(engineType);
    m_Engine = &m_IO->Open(name, mode, comm);
    m_Status = m_Engine->BeginStep();
    m_IsOpen = true;
}

void Stream::Open(const std::string &name, const Mode mode, MPI_Comm comm,
                  const std::string configFile,
                  const std::string ioInConfigFile)
{
    ThrowIfOpen(name + ", in call to constructor or open with config file\n");
    m_Name = name;
    m_ADIOS =
        std::make_shared<ADIOS>(configFile, comm, DebugON, m_HostLanguage);
    m_IO = &m_ADIOS->DeclareIO(ioInConfigFile);
    m_Engine = &m_IO->Open(name, mode, comm);
    m_Status = m_Engine->BeginStep();
    m_IsOpen = true;
}

void Stream::Open(const std::string &name, const Mode mode,
                  const std::string engineType, const Params &parameters,
                  const vParams &transportParameters)
{
    Open(name, mode, MPI_COMM_SELF, engineType, parameters,
         transportParameters);
}

void Stream::Open(const std::string &name, const Mode mode,
                  const std::string configFile,
                  const std::string ioInConfigFile)
{
    Open(name, mode, MPI_COMM_SELF, configFile, ioInConfigFile);
}

void Stream::Close()
{
    ThrowIfNotOpen(m_Name + ", in call to Close");
    m_Engine->Close();
    m_IsOpen = false;
    m_Status = StepStatus::NotReady;
}

// PRIVATE
void Stream::ThrowIfNotOpen(const std::string hint) const
{
    if (!m_IsOpen)
    {
        throw std::invalid_argument("ERROR: adios2 stream is not opened, " +
                                    hint + "\n");
    }
}

void Stream::ThrowIfOpen(const std::string hint) const
{
    if (m_IsOpen)
    {
        throw std::invalid_argument("ERROR: adios2 stream is already opened, " +
                                    hint + "\n");
    }
}

#define declare_template_instantiation(T)                                      \
    template void Stream::Write<T>(const std::string &, const T *,             \
                                   const Dims &, const Dims &, const Dims &,   \
                                   const bool);                                \
                                                                               \
    template void Stream::Write<T>(const std::string &, const T &,             \
                                   const bool);                                \
                                                                               \
    template void Stream::Read<T>(const std::string &, T *, const bool);       \
                                                                               \
    template void Stream::Read<T>(const std::string &, T *,                    \
                                  const Box<size_t> &);                        \
                                                                               \
    template void Stream::Read<T>(const std::string &, T *, const Box<Dims> &, \
                                  const bool);                                 \
                                                                               \
    template void Stream::Read<T>(const std::string &, T *, const Box<Dims> &, \
                                  const Box<size_t> &);                        \
                                                                               \
    template std::vector<T> Stream::Read<T>(const std::string &, const bool);  \
                                                                               \
    template std::vector<T> Stream::Read<T>(                                   \
        const std::string &, const Box<Dims> &, const Box<size_t> &);          \
                                                                               \
    template std::vector<T> Stream::Read<T>(const std::string &,               \
                                            const Box<Dims> &, const bool);

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace core
} // end namespace adios2
