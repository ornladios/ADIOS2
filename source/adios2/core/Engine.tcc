/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Engine.tcc
 *
 *  Created on: Jun 2, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_ENGINE_TCC_
#define ADIOS2_CORE_ENGINE_TCC_

#include "Engine.h"

#include <stdexcept>

#include "adios2/helper/adiosFunctions.h" // IsLvalue

namespace adios2
{

// Put
#define declare_launch_mode(L)                                                 \
                                                                               \
    template <class T>                                                         \
    void Engine::Put##L(Variable<T> &variable, const T *data)                  \
    {                                                                          \
        if (m_DebugMode)                                                       \
        {                                                                      \
            variable.CheckDimensions("Put" + std::string(#L));                 \
                                                                               \
            if (data == nullptr)                                               \
            {                                                                  \
                throw std::invalid_argument(                                   \
                    "ERROR: found null pointer for Variable " +                \
                    variable.m_Name + ", in call to Put" + std::string(#L) +   \
                    "\n");                                                     \
            }                                                                  \
        }                                                                      \
                                                                               \
        DoPut##L(variable, data);                                              \
    }                                                                          \
                                                                               \
    template <class T>                                                         \
    void Engine::Put##L(const std::string &variableName, const T *data)        \
    {                                                                          \
        Put##L(FindVariable<T>(variableName), data);                           \
    }                                                                          \
                                                                               \
    template <class T>                                                         \
    void Engine::Put##L(Variable<T> &variable)                                 \
    {                                                                          \
        Put##L(variable, variable.GetData());                                  \
    }                                                                          \
                                                                               \
    template <class T>                                                         \
    void Engine::Put##L(Variable<T> &variable, const T &value)                 \
    {                                                                          \
        const T valueLocal = value;                                            \
        Put##L(variable, &valueLocal);                                         \
    }                                                                          \
                                                                               \
    template <class T>                                                         \
    void Engine::Put##L(const std::string &variableName, const T &value)       \
    {                                                                          \
        Put##L(FindVariable<T>(variableName), value);                          \
    }
ADIOS2_FOREACH_LAUNCH_MODE(declare_launch_mode)
#undef declare_launch_mode

// Get
#define declare_launch_mode(L)                                                 \
                                                                               \
    template <class T>                                                         \
    void Engine::Get##L(Variable<T> &variable, T *data)                        \
    {                                                                          \
        if (m_DebugMode)                                                       \
        {                                                                      \
            if (&variable == nullptr)                                          \
            {                                                                  \
                throw std::invalid_argument("ERROR: variable reference is "    \
                                            "undefined, try using "            \
                                            "IO::InquireVariable(name) "       \
                                            "function first, in call to Get" + \
                                            std::string(#L) + "\n");           \
            }                                                                  \
            variable.CheckDimensions("Get" + std::string(#L));                 \
                                                                               \
            if (data == nullptr)                                               \
            {                                                                  \
                throw std::invalid_argument(                                   \
                    "ERROR: found null pointer for Variable " +                \
                    variable.m_Name + ", in call to Get" + std::string(#L) +   \
                    "\n");                                                     \
            }                                                                  \
        }                                                                      \
                                                                               \
        DoGet##L(variable, data);                                              \
    }                                                                          \
                                                                               \
    template <class T>                                                         \
    void Engine::Get##L(const std::string &variableName, T *data)              \
    {                                                                          \
        Get##L(FindVariable<T>(variableName), data);                           \
    }                                                                          \
                                                                               \
    template <class T>                                                         \
    void Engine::Get##L(Variable<T> &variable)                                 \
    {                                                                          \
        Get##L(variable, variable.GetData());                                  \
    }                                                                          \
                                                                               \
    template <class T>                                                         \
    void Engine::Get##L(Variable<T> &variable, T &value)                       \
    {                                                                          \
        Get##L(variable, &value);                                              \
    }                                                                          \
                                                                               \
    template <class T>                                                         \
    void Engine::Get##L(const std::string &variableName, T &value)             \
    {                                                                          \
        Get##L(FindVariable<T>(variableName), value);                          \
    }
ADIOS2_FOREACH_LAUNCH_MODE(declare_launch_mode)
#undef declare_launch_mode

// PRIVATE
template <class T>
Variable<T> &Engine::FindVariable(const std::string &variableName)
{
    Variable<T> *variable = m_IO.InquireVariable<T>(variableName);
    if (m_DebugMode && variable == nullptr)
    {
        throw std::invalid_argument("ERROR: Variable " + variableName +
                                    " not found in IO " + m_IO.m_Name +
                                    ", in call to Put Synch\n");
    }
    return *variable;
}

} // end namespace adios2

#endif /** ADIOS2_CORE_ENGINE_TCC_ */
