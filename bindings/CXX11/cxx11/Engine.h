/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Engine.h :
 *
 *  Created on: Jun 4, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_CXX11_CXX11_ENGINE_H_
#define ADIOS2_BINDINGS_CXX11_CXX11_ENGINE_H_

#include "Variable.h"

#include "adios2/ADIOSMacros.h"
#include "adios2/ADIOSTypes.h"

namespace adios2
{

// forward declare
class IO; // friend

namespace core
{
class Engine; // private implementation
}

class Engine
{
    friend class IO;

public:
    Engine() = default;

    ~Engine() = default;

    explicit operator bool() const noexcept;

    StepStatus BeginStep();

    StepStatus BeginStep(const StepMode mode, const float timeoutSeconds = 0.f);

    size_t CurrentStep() const;

    void FixedSchedule();

    template <class T>
    void Put(Variable<T> variable, const T *data,
             const Mode launch = Mode::Deferred);

    template <class T>
    void Put(const std::string &variableName, const T *data,
             const Mode launch = Mode::Deferred);

    template <class T>
    void Put(Variable<T> variable, const T &datum,
             const Mode launch = Mode::Deferred);

    template <class T>
    void Put(const std::string &variableName, const T &datum,
             const Mode launch = Mode::Deferred);

    void PerformPuts();

    template <class T>
    void Get(Variable<T> variable, T *data, const Mode launch = Mode::Deferred);

    template <class T>
    void Get(const std::string &variableName, T *data,
             const Mode launch = Mode::Deferred);

    template <class T>
    void Get(Variable<T> variable, T &datum,
             const Mode launch = Mode::Deferred);

    template <class T>
    void Get(const std::string &variableName, T &datum,
             const Mode launch = Mode::Deferred);

    void PerformGets();

    void EndStep();

    void Flush(const int transportIndex = -1);

    void Close(const int transportIndex = -1);

private:
    Engine(core::Engine *engine);
    core::Engine *m_Engine = nullptr;
};

#define declare_template_instantiation(T)                                      \
    extern template void Engine::Put<T>(Variable<T>, const T *, const Mode);   \
    extern template void Engine::Put<T>(const std::string &, const T *,        \
                                        const Mode);                           \
    extern template void Engine::Put<T>(Variable<T>, const T &, const Mode);   \
    extern template void Engine::Put<T>(const std::string &, const T &,        \
                                        const Mode);                           \
                                                                               \
    extern template void Engine::Get<T>(Variable<T>, T *, const Mode);         \
    extern template void Engine::Get<T>(const std::string &, T *, const Mode); \
    extern template void Engine::Get<T>(Variable<T>, T &, const Mode);         \
    extern template void Engine::Get<T>(const std::string &, T &, const Mode);

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX11_CXX11_ENGINE_H_ */
