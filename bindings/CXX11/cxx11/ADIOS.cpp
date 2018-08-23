/* Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS.cpp : public ADIOS class using PIMPL for C++11 bindings
 * Created on: Jun 4, 2018
 *     Author: William F Godoy
 */

#include "ADIOS.h"

#include "adios2/ADIOSMPI.h"
#include "adios2/core/ADIOS.h"

namespace adios2
{

ADIOS::ADIOS(const std::string &configFile, MPI_Comm comm, const bool debugMode)
: m_ADIOS(std::make_shared<core::ADIOS>(configFile, comm, debugMode))
{
}

ADIOS::ADIOS(MPI_Comm comm, const bool debugMode) : ADIOS("", comm, debugMode)
{
}

ADIOS::ADIOS(const std::string &configFile, const bool debugMode)
: ADIOS(configFile, MPI_COMM_SELF, debugMode)
{
}

ADIOS::ADIOS(const bool debugMode) : ADIOS("", MPI_COMM_SELF, debugMode) {}

ADIOS::operator bool() const noexcept { return m_ADIOS ? true : false; }

IO ADIOS::DeclareIO(const std::string name)
{
    return IO(m_ADIOS->DeclareIO(name));
}

IO ADIOS::AtIO(const std::string name) { return IO(m_ADIOS->AtIO(name)); }

void ADIOS::FlushAll() { m_ADIOS->FlushAll(); }

Operator ADIOS::DefineOperator(const std::string name, const std::string type,
                               const Params &parameters)
{
    return Operator(&m_ADIOS->DefineOperator(name, type, parameters));
}

Operator ADIOS::InquireOperator(const std::string name) noexcept
{
    return Operator(m_ADIOS->InquireOperator(name));
}

// PRIVATE

#define declare_type(T)                                                        \
    Operator ADIOS::DefineCallBack(                                            \
        const std::string name,                                                \
        const std::function<void(const T *, const std::string &,                 \
                                 const std::string &, const std::string &,         \
                                 const size_t, const Dims &,\
                                 const Dims &, const Dims &)> &function,                     \
        const Params &parameters)                                              \
    {                                                                          \
        return Operator(&m_ADIOS->DefineCallBack(name, function, parameters)); \
    }
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

Operator ADIOS::DefineCallBack(
    const std::string name,
    const std::function<void(void *, const std::string &, const std::string &,
                             const std::string &, const size_t, const Dims &, const Dims &, const Dims &)> &function,
    const Params &parameters)
{
    return Operator(&m_ADIOS->DefineCallBack(name, function, parameters));
}

} // end namespace adios2
