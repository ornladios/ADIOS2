/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * cxx98Engine.h
 *
 *  Created on: Apr 10, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef BINDINGS_CXX98_CXX98_CXX98ENGINE_H_
#define BINDINGS_CXX98_CXX98_CXX98ENGINE_H_

#include <adios2_c.h>

#include "cxx98Variable.h"
#include "cxx98types.h"

namespace adios2
{
namespace cxx98
{

class Engine
{
public:
    Engine(adios2_engine &engine);

    ~Engine();

    StepStatus BeginStep();

    StepStatus BeginStep(const StepMode mode, const float timeoutSeconds = 0.f);

    size_t CurrentStep() const;

    template <class T>
    void Put(Variable<T> &variable, const T *data,
             const Mode launch = Deferred);

    void PerformPuts();

    template <class T>
    void Get(Variable<T> &variable, T *data, const Mode launch = Deferred);

    void PerformGets();

    void EndStep();

    void Flush(const int transportIndex = -1);

    void Close(const int transportIndex = -1);

private:
    adios2_engine &m_Engine;
};

#define declare_template_instantiation(T)                                      \
    extern template void Engine::Put<T>(Variable<T> &, const T *, const Mode); \
                                                                               \
    extern template void Engine::Get<T>(Variable<T> &, T *, const Mode);

ADIOS2_FOREACH_CXX98_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace cxx98
} // end namespace adios2

#endif /* BINDINGS_CXX98_CXX98_CXX98ENGINE_H_ */
