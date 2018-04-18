/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * cxx03Engine.h
 *
 *  Created on: Apr 10, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef BINDINGS_CXX03_CXX03_CXX03ENGINE_H_
#define BINDINGS_CXX03_CXX03_CXX03ENGINE_H_

#include "cxx03Variable.h"
#include "cxx03types.h"

#include <adios2_c.h>

namespace adios2
{
namespace cxx03
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
    void PutSync(Variable<T> &variable, const T *values);

    template <class T>
    void PutDeferred(Variable<T> &variable, const T *values);

    template <class T>
    void GetSync(Variable<T> &variable, T *values);

    template <class T>
    void GetDeferred(Variable<T> &variable, T *values);

    void EndStep();

    void PerformPuts();

    void PerformGets();

    void WriteStep();

    void Flush(const int transportIndex = -1);

    void Close(const int transportIndex = -1);

private:
    adios2_engine &m_Engine;
};

#define declare_template_instantiation(T)                                      \
    extern template void Engine::PutSync<T>(Variable<T> &, const T *);         \
                                                                               \
    extern template void Engine::PutDeferred<T>(Variable<T> &, const T *);     \
                                                                               \
    extern template void Engine::GetSync<T>(Variable<T> &, T *);               \
                                                                               \
    extern template void Engine::GetDeferred<T>(Variable<T> &, T *);

ADIOS2_FOREACH_CXX03_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace cxx03
} // end namespace adios2

#endif /* BINDINGS_CXX03_CXX03_CXX03ENGINE_H_ */
