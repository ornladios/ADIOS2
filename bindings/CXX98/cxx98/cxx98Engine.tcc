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
void Engine::Put(Variable<T> &variable, const T *values, const Mode launch)
{
    adios2_put(&m_Engine, variable.m_Variable, values,
               static_cast<adios2_mode>(launch));
}

template <class T>
void Engine::Get(Variable<T> &variable, T *values, const Mode launch)
{
    adios2_get(&m_Engine, variable.m_Variable, values,
               static_cast<adios2_mode>(launch));
}

} // end namespace cxx98
} // end namespace adios2

#endif /* BINDINGS_CXX98_CXX98_CXX98ENGINE_TCC_ */
