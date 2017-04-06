/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS.tcc
 *   This contains the template implementations for the ADIOS class
 */

#ifndef ADIOS_TCC_
#define ADIOS_TCC_

#include "ADIOS.h"

namespace adios
{

template <class T>
VariableCompound &ADIOS::DefineVariableCompound(const std::string &name,
                                                const Dims dimensions,
                                                const Dims globalDimensions,
                                                const Dims globalOffsets)
{
    CheckVariableInput(name, dimensions);
    const unsigned int size = m_Compound.size();
    m_Compound.emplace(size, VariableCompound(name, sizeof(T), dimensions,
                                              globalDimensions, globalOffsets,
                                              m_DebugMode));
    m_Variables.emplace(name, std::make_pair(GetType<T>(), size));
    return m_Compound.at(size);
}

} // end namespace adios

#endif /* ADIOS_TCC_ */
