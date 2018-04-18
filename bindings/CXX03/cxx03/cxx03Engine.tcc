/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * cxx03Engine.tcc
 *
 *  Created on: Apr 18, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef BINDINGS_CXX03_CXX03_CXX03ENGINE_TCC_
#define BINDINGS_CXX03_CXX03_CXX03ENGINE_TCC_

#include "cxx03Engine.h"

namespace adios2
{
namespace cxx03
{

template <class T>
void Engine::PutSync(Variable<T> &variable, const T *values)
{
    adios2_put_sync(&m_Engine, variable.m_Variable, values);
}

template <class T>
void Engine::PutDeferred(Variable<T> &variable, const T *values)
{
    adios2_put_deferred(&m_Engine, variable.m_Variable, values);
}

template <class T>
void Engine::GetSync(Variable<T> &variable, T *values)
{
    adios2_get_sync(&m_Engine, variable.m_Variable, values);
}

template <class T>
void Engine::GetDeferred(Variable<T> &variable, T *values)
{
    adios2_get_deferred(&m_Engine, variable.m_Variable, values);
}

} // end namespace cxx03
} // end namespace adios2

#endif /* BINDINGS_CXX03_CXX03_CXX03ENGINE_TCC_ */
