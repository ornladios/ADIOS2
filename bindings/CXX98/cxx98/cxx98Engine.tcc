/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * cxx98Engine.tcc
 *
 *  Created on: Apr 18, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_CXX98_CXX98_CXX98ENGINE_TCC_
#define ADIOS2_BINDINGS_CXX98_CXX98_CXX98ENGINE_TCC_

#include "cxx98Engine.h"

#include <adios2_c.h>

namespace adios2
{
namespace cxx98
{

template <class T>
void Engine::Put(Variable<T> variable, const T *data, const Mode launch)
{
    adios2_put(m_Engine, variable.m_Variable, data, (adios2_mode)launch);
}

template <class T>
void Engine::Put(const std::string &variableName, const T *data,
                 const Mode launch)
{
    adios2_put_by_name(m_Engine, variableName.c_str(), data,
                       (adios2_mode)launch);
}

template <class T>
void Engine::Put(Variable<T> variable, const T &datum, const Mode /** launch */)
{
    const T datumLocal = datum;
    adios2_put(m_Engine, variable.m_Variable, &datumLocal, adios2_mode_sync);
}

template <class T>
void Engine::Put(const std::string &variableName, const T &datum,
                 const Mode /** launch */)
{
    const T datumLocal = datum;
    adios2_put_by_name(m_Engine, variableName.c_str(), &datumLocal,
                       adios2_mode_sync);
}

template <class T>
void Engine::Get(Variable<T> variable, T *data, const Mode launch)
{
    adios2_get(m_Engine, variable.m_Variable, data, (adios2_mode)launch);
}

template <class T>
void Engine::Get(const std::string &variableName, T *data, const Mode launch)
{
    adios2_get_by_name(m_Engine, variableName.c_str(), data,
                       (adios2_mode)launch);
}

template <class T>
void Engine::Get(Variable<T> variable, T &datum, const Mode launch)
{
    adios2_get(m_Engine, variable.m_Variable, &datum, (adios2_mode)launch);
}

template <class T>
void Engine::Get(const std::string &variableName, T &datum, const Mode launch)
{
    adios2_get_by_name(m_Engine, variableName.c_str(), &datum,
                       (adios2_mode)launch);
}

} // end namespace cxx98
} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX98_CXX98_CXX98ENGINE_TCC_ */
