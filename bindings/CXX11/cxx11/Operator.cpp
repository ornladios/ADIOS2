/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Operator.cpp :
 *
 *  Created on: Jun 7, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "Operator.h"
#include "Operator.tcc"

#include "adios2/ADIOSMacros.h"
#include "adios2/core/Operator.h"
#include "adios2/helper/adiosFunctions.h"

namespace adios2
{

Operator::operator bool() const noexcept
{
    return (m_Operator == nullptr) ? false : true;
}

std::string Operator::Type() const noexcept
{
    if (m_Operator == nullptr)
    {
        return ""; // empty string
    }

    return m_Operator->m_Type;
}

void Operator::SetParameter(const std::string key, const std::string value)
{
    helper::CheckForNullptr(m_Operator, "in call to Operator::SetParameter");
    m_Operator->SetParameter(key, value);
}

const Params &Operator::GetParameters()
{
    helper::CheckForNullptr(m_Operator, "in call to Operator::GetParameters");
    return m_Operator->GetParameters();
}

void Operator::RunCallback2(void *arg0, const std::string &arg1,
                            const std::string &arg2, const std::string &arg3,
                            const Dims &arg4)
{
    helper::CheckForNullptr(m_Operator, "in call to Operator::RunCallback2");
    m_Operator->RunCallback2(arg0, arg1, arg2, arg3, arg4);
}

size_t Operator::BufferMaxSize(const size_t sizeIn) const
{
    helper::CheckForNullptr(m_Operator, "in call to Operator::BufferMaxSize");
    return m_Operator->BufferMaxSize(sizeIn);
}

// PRIVATE
Operator::Operator(core::Operator *op) : m_Operator(op) {}

// TEMPLATES
#define declare_template_instantiation(T)                                      \
    template void Operator::RunCallback1(const T *, const std::string &,       \
                                         const std::string &,                  \
                                         const std::string &, const Dims &);   \
                                                                               \
    template size_t Operator::Compress(const T *, const Dims &, const size_t,  \
                                       const std::string, char *,              \
                                       const Params &) const;                  \
                                                                               \
    template size_t Operator::Decompress(const char *, const size_t, T *,      \
                                         const size_t) const;

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

#define declare_template_instantiation(T)                                      \
    template size_t Operator::BufferMaxSize(const T *, const Dims &,           \
                                            const Params &) const;             \
                                                                               \
    template size_t Operator::Decompress(const char *, const size_t, T *,      \
                                         const Dims &, const std::string,      \
                                         const Params &) const;

ADIOS2_FOREACH_ZFP_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace adios2
