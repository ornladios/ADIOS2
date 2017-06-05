/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * IO.inl inline template functions implementation of IO class
 *
 *  Created on: May 15, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_IO_INL_
#define ADIOS2_CORE_IO_INL_
#ifndef ADIOS2_CORE_IO_H_
#error "Inline file should only be included from it's header, never on it's own"
#endif

#include "adios2/ADIOSMPI.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h" //BuildParametersMap

namespace adios
{

template <class... Args>
void IO::SetParameters(Args... args)
{
    std::vector<std::string> parameters = {args...};
    m_Parameters = BuildParametersMap(parameters, m_DebugMode);
}

template <class... Args>
unsigned int IO::AddTransport(const std::string type, Args... args)
{
    std::vector<std::string> parameters = {args...};
    return AddTransportParameters(type, parameters);
}

template <class T>
VariableCompound &IO::DefineVariableCompound(const std::string &name,
                                             const Dims shape, const Dims start,
                                             const Dims count,
                                             const bool constantShape)
{
    if (m_DebugMode == true)
    {
        if (VariableExists(name) == true)
        {
            std::invalid_argument("ERROR: variable " + name +
                                  " exists in IO object " + m_Name +
                                  ", in call to DefineVariable\n");
        }
    }
    const unsigned int size = m_Compound.size();
    m_Compound.emplace(size,
                       VariableCompound(name, sizeof(T), shape, start, count,
                                        constantShape, m_DebugMode));
    m_Variables.emplace(name, std::make_pair(GetType<T>(), size));
    return m_Compound.at(size);
}

} // end namespace adios

#endif /* ADIOS2_CORE_IO_INL_ */
