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
               const std::string engineType, const std::string hostLanguage)
: m_Name(name), m_ADIOS(std::make_shared<ADIOS>(comm, DebugON, hostLanguage)),
  m_IO(&m_ADIOS->DeclareIO(name)), m_Mode(mode), m_EngineType(engineType)
{
    if (mode == adios2::Mode::Read)
    {
        CheckOpen();
    }
}

Stream::Stream(const std::string &name, const Mode mode,
               const std::string engineType, const std::string hostLanguage)
: Stream(name, mode, MPI_COMM_SELF, engineType, hostLanguage)
{
}

Stream::Stream(const std::string &name, const Mode mode, MPI_Comm comm,
               const std::string configFile, const std::string ioInConfigFile,
               const std::string hostLanguage)
: m_Name(name),
  m_ADIOS(std::make_shared<ADIOS>(configFile, comm, DebugON, hostLanguage)),
  m_IO(&m_ADIOS->DeclareIO(ioInConfigFile)), m_Mode(mode)
{
    if (mode == adios2::Mode::Read)
    {
        CheckOpen();
    }
}

Stream::Stream(const std::string &name, const Mode mode,
               const std::string configFile, const std::string ioInConfigFile,
               const std::string hostLanguage)
: Stream(name, mode, MPI_COMM_SELF, configFile, ioInConfigFile, hostLanguage)
{
}

bool Stream::GetStep()
{
    if (!m_FirstStep)
    {
        if (m_StepStatus)
        {
            m_Engine->EndStep();
        }
    }
    else
    {
        m_FirstStep = false;
    }

    if (m_Engine->BeginStep() != StepStatus::OK)
    {
        m_StepStatus = false;
        return false;
    }

    m_StepStatus = true;
    return true;
}

void Stream::EndStep()
{
    if (m_StepStatus)
    {
        m_Engine->EndStep();
        m_StepStatus = false;
    }
    else
    {
        throw std::invalid_argument("ERROR: stream " + m_Name +
                                    " calling end step function twice (check "
                                    "if a write function calls it) or "
                                    "invalid stream\n");
    }
}

void Stream::Close()
{
    if (m_Engine != nullptr)
    {
        m_Engine->Close();
        m_StepStatus = false;
        m_Engine = nullptr;
    }
}

size_t Stream::CurrentStep() const
{
    if (m_FirstStep)
    {
        return 0;
    }

    if (m_Engine == nullptr)
    {
        throw std::invalid_argument("ERROR: stream with name " + m_Name +
                                    "is invalid or closed, in call "
                                    "to CurrentStep");
    }

    return m_Engine->CurrentStep();
}

// PRIVATE
void Stream::CheckOpen()
{
    if (m_Engine == nullptr)
    {
        if (!m_EngineType.empty())
        {
            m_IO->SetEngine(m_EngineType);
        }
        m_Engine = &m_IO->Open(m_Name, m_Mode);
        if (m_Mode == adios2::Mode::Write)
        {
            m_Engine->BeginStep();
            m_StepStatus = true;
        }
    }
}

#define declare_template_instantiation(T)                                      \
    template void Stream::WriteAttribute(const std::string &, const T &,       \
                                         const std::string &,                  \
                                         const std::string, const bool);       \
                                                                               \
    template void Stream::WriteAttribute(const std::string &, const T *,       \
                                         const size_t, const std::string &,    \
                                         const std::string, const bool);       \
                                                                               \
    template void Stream::ReadAttribute(                                       \
        const std::string &, T *, const std::string &, const std::string);

ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

#define declare_template_instantiation(T)                                      \
    template void Stream::Write<T>(const std::string &, const T *,             \
                                   const Dims &, const Dims &, const Dims &,   \
                                   const bool);                                \
                                                                               \
    template void Stream::Write<T>(const std::string &, const T &,             \
                                   const bool);                                \
                                                                               \
    template void Stream::Read<T>(const std::string &, T *);                   \
                                                                               \
    template void Stream::Read<T>(const std::string &, T *,                    \
                                  const Box<size_t> &);                        \
                                                                               \
    template void Stream::Read<T>(const std::string &, T *,                    \
                                  const Box<Dims> &);                          \
                                                                               \
    template void Stream::Read<T>(const std::string &, T *, const Box<Dims> &, \
                                  const Box<size_t> &);                        \
                                                                               \
    template std::vector<T> Stream::Read<T>(const std::string &);              \
                                                                               \
    template std::vector<T> Stream::Read<T>(const std::string &,               \
                                            const Box<size_t> &);              \
                                                                               \
    template std::vector<T> Stream::Read<T>(                                   \
        const std::string &, const Box<Dims> &, const Box<size_t> &);          \
                                                                               \
    template std::vector<T> Stream::Read<T>(const std::string &,               \
                                            const Box<Dims> &);

ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace core
} // end namespace adios2
