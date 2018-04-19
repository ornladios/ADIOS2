/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * cxx98Engine.tcc
 *
 *  Created on: Apr 18, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef BINDINGS_CXX98_CXX98_CXX98ENGINE_TCC_
#define BINDINGS_CXX98_CXX98_CXX98ENGINE_TCC_

#include "cxx98Engine.h"

namespace adios2
{
namespace cxx98
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

} // end namespace cxx98
} // end namespace adios2

#endif /* BINDINGS_CXX98_CXX98_CXX98ENGINE_TCC_ */
