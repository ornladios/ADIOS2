/* Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS.cpp : public ADIOS class using PIMPL for C++11 bindings
 * Created on: Jun 4, 2018
 *     Author: William F Godoy
 */

#include "ADIOS.h"

#include "adios2/core/ADIOS.h"
#include "adios2/core/IO.h"
#include "adios2/helper/adiosFunctions.h" //CheckForNullptr

#ifdef ADIOS2_HAVE_MPI
#include "adios2/helper/adiosCommMPI.h"
#endif

namespace adios2
{
#ifdef ADIOS2_HAVE_MPI
ADIOS::ADIOS(const std::string &configFile, MPI_Comm comm, const bool debugMode)
: m_ADIOS(std::make_shared<core::ADIOS>(configFile, helper::CommFromMPI(comm),
                                        debugMode, "C++"))
{
}

ADIOS::ADIOS(MPI_Comm comm, const bool debugMode) : ADIOS("", comm, debugMode)
{
}

#endif
ADIOS::ADIOS(const std::string &configFile, const bool debugMode)
: m_ADIOS(std::make_shared<core::ADIOS>(configFile, debugMode, "C++"))
{
}

ADIOS::ADIOS(const bool debugMode) : ADIOS("", debugMode) {}

ADIOS::operator bool() const noexcept { return m_ADIOS ? true : false; }

IO ADIOS::DeclareIO(const std::string name)
{
    CheckPointer("for io name " + name + ", in call to ADIOS::DeclareIO");
    return IO(&m_ADIOS->DeclareIO(name));
}

IO ADIOS::AtIO(const std::string name)
{
    CheckPointer("for io name " + name + ", in call to ADIOS::AtIO");
    return IO(&m_ADIOS->AtIO(name));
}

void ADIOS::FlushAll()
{
    CheckPointer("in call to ADIOS::FlushAll");
    m_ADIOS->FlushAll();
}

Operator ADIOS::DefineOperator(const std::string name, const std::string type,
                               const Params &parameters)
{
    CheckPointer("for operator name " + name +
                 ", in call to ADIOS::DefineOperator");
    return Operator(&m_ADIOS->DefineOperator(name, type, parameters));
}

Operator ADIOS::InquireOperator(const std::string name)
{
    CheckPointer("for operator name " + name + ", in call to InquireOperator");
    return Operator(m_ADIOS->InquireOperator(name));
}

bool ADIOS::RemoveIO(const std::string name)
{
    CheckPointer("for io name " + name + ", in call to ADIOS::RemoveIO");
    return m_ADIOS->RemoveIO(name);
}

void ADIOS::RemoveAllIOs() noexcept
{
    CheckPointer("in call to ADIOS::RemoveAllIOs");
    m_ADIOS->RemoveAllIOs();
}

// PRIVATE

#define declare_type(T)                                                        \
    Operator ADIOS::DefineCallBack(                                            \
        const std::string name,                                                \
        const std::function<void(const T *, const std::string &,               \
                                 const std::string &, const std::string &,     \
                                 const size_t, const Dims &, const Dims &,     \
                                 const Dims &)> &function,                     \
        const Params &parameters)                                              \
    {                                                                          \
        using IOType = typename TypeInfo<T>::IOType;                           \
                                                                               \
        const auto &io_function = reinterpret_cast<const std::function<void(   \
            const IOType *, const std::string &, const std::string &,          \
            const std::string &, const size_t, const Dims &, const Dims &,     \
            const Dims &)> &>(function);                                       \
        return Operator(                                                       \
            &m_ADIOS->DefineCallBack(name, io_function, parameters));          \
    }
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

Operator ADIOS::DefineCallBack(
    const std::string name,
    const std::function<void(void *, const std::string &, const std::string &,
                             const std::string &, const size_t, const Dims &,
                             const Dims &, const Dims &)> &function,
    const Params &parameters)
{
    return Operator(&m_ADIOS->DefineCallBack(name, function, parameters));
}

// PRIVATE
void ADIOS::CheckPointer(const std::string hint)
{
    if (!m_ADIOS)
    {
        throw std::invalid_argument("ERROR: invalid ADIOS object, did you call "
                                    "any of the ADIOS explicit "
                                    "constructors?, " +
                                    hint + "\n");
    }
}

} // end namespace adios2
