/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_c_engine.cpp
 *
 *  Created on: Nov 8, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adios2_c_engine.h"

#include "adios2/core/Engine.h"
#include "adios2/helper/adiosFunctions.h" //GetType<T>

adios2_step_status adios2_begin_step(adios2_Engine *engine,
                                     const adios2_step_mode mode,
                                     const float timeout_seconds)
{
    auto &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);
    return static_cast<adios2_step_status>(engineCpp.BeginStep(
        static_cast<adios2::StepMode>(mode), timeout_seconds));
}

void adios2_put_sync(adios2_Engine *engine, adios2_Variable *variable,
                     const void *values)
{
    adios2::VariableBase *variableBase =
        reinterpret_cast<adios2::VariableBase *>(variable);
    const std::string type(variableBase->m_Type);

    adios2::Engine &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);

    if (type == "compound")
    {
        // not supported
    }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::GetType<T>())                                     \
    {                                                                          \
        engineCpp.PutSync(*dynamic_cast<adios2::Variable<T> *>(variableBase),  \
                          reinterpret_cast<const T *>(values));                \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
}

void adios2_put_sync_self(adios2_Engine *engine, adios2_Variable *variable)
{
    adios2::VariableBase *variableBase =
        reinterpret_cast<adios2::VariableBase *>(variable);
    const std::string type(variableBase->m_Type);

    adios2::Engine &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);

    if (type == "compound")
    {
        // not supported
    }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::GetType<T>())                                     \
    {                                                                          \
        engineCpp.PutSync(*dynamic_cast<adios2::Variable<T> *>(variableBase)); \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
}

void adios2_put_sync_by_name(adios2_Engine *engine, const char *variable_name,
                             const void *values)
{
    auto &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);
    const std::string type(
        engineCpp.GetIO().InquireVariableType(variable_name));

    if (type == "compound")
    {
        // not supported
    }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::GetType<T>())                                     \
    {                                                                          \
        engineCpp.PutSync(variable_name, reinterpret_cast<const T *>(values)); \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
}

void adios2_put_deferred(adios2_Engine *engine, adios2_Variable *variable,
                         const void *values)
{
    adios2::VariableBase *variableBase =
        reinterpret_cast<adios2::VariableBase *>(variable);
    const std::string type(variableBase->m_Type);

    adios2::Engine &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);

    if (type == "compound")
    {
        // not supported
    }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::GetType<T>())                                     \
    {                                                                          \
        engineCpp.PutDeferred(                                                 \
            *dynamic_cast<adios2::Variable<T> *>(variableBase),                \
            reinterpret_cast<const T *>(values));                              \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
}

void adios2_put_deferred_self(adios2_Engine *engine, adios2_Variable *variable)
{
    adios2::VariableBase *variableBase =
        reinterpret_cast<adios2::VariableBase *>(variable);
    const std::string type(variableBase->m_Type);

    adios2::Engine &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);

    if (type == "compound")
    {
        // not supported
    }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::GetType<T>())                                     \
    {                                                                          \
        engineCpp.PutDeferred(                                                 \
            *dynamic_cast<adios2::Variable<T> *>(variableBase));               \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
}

void adios2_put_deferred_by_name(adios2_Engine *engine,
                                 const char *variable_name, const void *values)
{
    auto &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);
    const std::string type(
        engineCpp.GetIO().InquireVariableType(variable_name));

    if (type == "compound")
    {
        // not supported
    }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::GetType<T>())                                     \
    {                                                                          \
        engineCpp.PutDeferred(variable_name,                                   \
                              reinterpret_cast<const T *>(values));            \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
}

void adios2_perform_puts(adios2_Engine *engine)
{
    auto &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);
    engineCpp.PerformPuts();
}

void adios2_get_sync(adios2_Engine *engine, adios2_Variable *variable,
                     void *values)
{
    adios2::VariableBase *variableBase =
        reinterpret_cast<adios2::VariableBase *>(variable);
    const std::string type(variableBase->m_Type);

    adios2::Engine &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);

    if (type == "compound")
    {
        // not supported
    }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::GetType<T>())                                     \
    {                                                                          \
        engineCpp.GetSync(*dynamic_cast<adios2::Variable<T> *>(variableBase),  \
                          reinterpret_cast<T *>(values));                      \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
}

void adios2_get_sync_self(adios2_Engine *engine, adios2_Variable *variable)
{
    adios2::VariableBase *variableBase =
        reinterpret_cast<adios2::VariableBase *>(variable);
    const std::string type(variableBase->m_Type);

    adios2::Engine &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);

    if (type == "compound")
    {
        // not supported
    }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::GetType<T>())                                     \
    {                                                                          \
        engineCpp.GetSync(*dynamic_cast<adios2::Variable<T> *>(variableBase)); \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
}

void adios2_get_sync_by_name(adios2_Engine *engine, const char *variable_name,
                             void *values)
{
    auto &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);
    const std::string type(
        engineCpp.GetIO().InquireVariableType(variable_name));

    if (type == "compound")
    {
        // not supported
    }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::GetType<T>())                                     \
    {                                                                          \
        engineCpp.GetSync(variable_name, reinterpret_cast<T *>(values));       \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
}

void adios2_get_deferred(adios2_Engine *engine, adios2_Variable *variable,
                         void *values)
{
    adios2::VariableBase *variableBase =
        reinterpret_cast<adios2::VariableBase *>(variable);
    const std::string type(variableBase->m_Type);

    adios2::Engine &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);

    if (type == "compound")
    {
        // not supported
    }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::GetType<T>())                                     \
    {                                                                          \
        engineCpp.GetDeferred(                                                 \
            *dynamic_cast<adios2::Variable<T> *>(variableBase),                \
            reinterpret_cast<T *>(values));                                    \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
}

void adios2_get_deferred_self(adios2_Engine *engine, adios2_Variable *variable)
{
    adios2::VariableBase *variableBase =
        reinterpret_cast<adios2::VariableBase *>(variable);
    const std::string type(variableBase->m_Type);

    adios2::Engine &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);

    if (type == "compound")
    {
        // not supported
    }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::GetType<T>())                                     \
    {                                                                          \
        engineCpp.GetDeferred(                                                 \
            *dynamic_cast<adios2::Variable<T> *>(variableBase));               \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
}

void adios2_get_deferred_by_name(adios2_Engine *engine,
                                 const char *variable_name, void *values)
{
    auto &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);
    const std::string type(
        engineCpp.GetIO().InquireVariableType(variable_name));

    if (type == "compound")
    {
        // not supported
    }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::GetType<T>())                                     \
    {                                                                          \
        engineCpp.GetDeferred(variable_name, reinterpret_cast<T *>(values));   \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
}

void adios2_perform_gets(adios2_Engine *engine)
{
    auto &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);
    engineCpp.PerformGets();
}

void adios2_end_step(adios2_Engine *engine)
{
    auto &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);
    engineCpp.EndStep();
}

void adios2_write_step(adios2_Engine *engine)
{
    auto &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);
    engineCpp.WriteStep();
}

void adios2_close(adios2_Engine *engine)
{
    auto &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);
    engineCpp.Close();
}

void adios2_close_by_index(adios2_Engine *engine,
                           const unsigned int transport_index)
{
    auto &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);
    engineCpp.Close(transport_index);
}
