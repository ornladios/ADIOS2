/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Engine.tcc :
 *
 *  Created on: Jun 5, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_CXX11_CXX11_ENGINE_TCC_
#define ADIOS2_BINDINGS_CXX11_CXX11_ENGINE_TCC_

#include "Engine.h"

#include "adios2/core/Engine.h"
#include "adios2/helper/adiosFunctions.h"

namespace adios2
{

template <class T>
void Engine::Put(Variable<T> variable, const T *data, const Mode launch)
{
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Put");
    m_Engine->Put<T>(*variable.m_Variable, data, launch);
}

template <class T>
void Engine::Put(const std::string &variableName, const T *data,
                 const Mode launch)
{
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Put");
    m_Engine->Put<T>(variableName, data, launch);
}

template <class T>
void Engine::Put(Variable<T> variable, const T &datum, const Mode /*launch*/)
{
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Put");
    m_Engine->Put(*variable.m_Variable, datum);
}

template <class T>
void Engine::Put(const std::string &variableName, const T &datum,
                 const Mode /*launch*/)
{
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Put");
    m_Engine->Put<T>(variableName, datum);
}

template <class T>
void Engine::Get(Variable<T> variable, T *data, const Mode launch)
{
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Get");
    m_Engine->Get<T>(*variable.m_Variable, data, launch);
}

template <class T>
void Engine::Get(const std::string &variableName, T *data, const Mode launch)
{
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Get");
    m_Engine->Get<T>(variableName, data, launch);
}

template <class T>
void Engine::Get(Variable<T> variable, T &datum, const Mode /*launch*/)
{
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Get");
    m_Engine->Get<T>(*variable.m_Variable, datum);
}

template <class T>
void Engine::Get(const std::string &variableName, T &datum,
                 const Mode /*launch*/)
{
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Get");
    m_Engine->Get<T>(variableName, datum);
}

template <class T>
void Engine::Get(Variable<T> variable, std::vector<T> &dataV, const Mode launch)
{
    adios2::helper::CheckForNullptr(
        m_Engine, "in call to Engine::Get with std::vector argument");
    m_Engine->Get<T>(*variable.m_Variable, dataV, launch);
}

template <class T>
void Engine::Get(const std::string &variableName, std::vector<T> &dataV,
                 const Mode launch)
{
    adios2::helper::CheckForNullptr(
        m_Engine, "in call to Engine::Get with std::vector argument");
    m_Engine->Get<T>(variableName, dataV, launch);
}

} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX11_CXX11_ENGINE_TCC_ */
