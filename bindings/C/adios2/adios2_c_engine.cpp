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

adios2_step_status adios2_begin_step(adios2_engine *engine,
                                     const adios2_step_mode mode,
                                     const float timeout_seconds)
{
    adios2::CheckForNullptr(engine,
                            "for adios2_engine, in call to adios2_begin_step");
    auto &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);

    adios2::StepStatus statusCpp = adios2::StepStatus::OK;

    switch (mode)
    {
    case (adios2_step_mode_append):
        statusCpp =
            engineCpp.BeginStep(adios2::StepMode::Append, timeout_seconds);
        break;

    case (adios2_step_mode_update):
        statusCpp =
            engineCpp.BeginStep(adios2::StepMode::Update, timeout_seconds);
        break;

    case (adios2_step_mode_next_available):
        statusCpp = engineCpp.BeginStep(adios2::StepMode::NextAvailable,
                                        timeout_seconds);
        break;

    case (adios2_step_mode_latest_available):
        statusCpp = engineCpp.BeginStep(adios2::StepMode::LatestAvailable,
                                        timeout_seconds);
        break;
    }

    adios2_step_status status = adios2_step_status_ok;
    switch (statusCpp)
    {
    case (adios2::StepStatus::OK):
        status = adios2_step_status_ok;
        break;

    case (adios2::StepStatus::NotReady):
        status = adios2_step_status_not_ready;
        break;

    case (adios2::StepStatus::EndOfStream):
        status = adios2_step_status_end_of_stream;
        break;

    case (adios2::StepStatus::OtherError):
        status = adios2_step_status_other_error;
        break;
    }

    return status;
}

void adios2_put_sync(adios2_engine *engine, adios2_variable *variable,
                     const void *values)
{
    adios2::CheckForNullptr(engine,
                            "for adios2_engine, in call to adios2_put_sync");

    adios2::CheckForNullptr(variable,
                            "for adios2_variable, in call to adios2_put_sync");

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

void adios2_put_sync_self(adios2_engine *engine, adios2_variable *variable)
{
    adios2::CheckForNullptr(
        engine, "for adios2_engine, in call to adios2_put_sync_self");

    adios2::CheckForNullptr(
        variable, "for adios2_variable, in call to adios2_put_sync_self");

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

void adios2_put_sync_by_name(adios2_engine *engine, const char *variable_name,
                             const void *values)
{
    adios2::CheckForNullptr(
        engine, "for adios2_engine, in call to adios2_put_sync_by_name");

    adios2::CheckForNullptr(
        variable_name,
        "for const char* variable_name, in call to adios2_put_sync_by_name");

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

void adios2_put_deferred(adios2_engine *engine, adios2_variable *variable,
                         const void *values)
{
    adios2::CheckForNullptr(
        engine, "for adios2_engine, in call to adios2_put_deferred");

    adios2::CheckForNullptr(
        variable, "for adios2_variable, in call to adios2_put_deferred");

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

void adios2_put_deferred_self(adios2_engine *engine, adios2_variable *variable)
{
    adios2::CheckForNullptr(
        engine, "for adios2_engine, in call to adios2_put_deferred_self");

    adios2::CheckForNullptr(
        variable, "for adios2_variable, in call to adios2_put_deferred_self");

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

void adios2_put_deferred_by_name(adios2_engine *engine,
                                 const char *variable_name, const void *values)
{
    adios2::CheckForNullptr(
        engine, "for adios2_engine, in call to adios2_put_deferred_by_name");

    adios2::CheckForNullptr(variable_name,
                            "for const char* variable_name, in call "
                            "to adios2_put_deferred_by_name");

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

void adios2_perform_puts(adios2_engine *engine)
{
    adios2::CheckForNullptr(
        engine, "for adios2_engine, in call to adios2_perform_puts");
    auto &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);
    engineCpp.PerformPuts();
}

void adios2_get_sync(adios2_engine *engine, adios2_variable *variable,
                     void *values)
{
    adios2::CheckForNullptr(engine,
                            "for adios2_engine, in call to adios2_get_sync");
    adios2::CheckForNullptr(variable, "for adios2_variable, in call "
                                      "to adios2_get_sync");

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

void adios2_get_sync_self(adios2_engine *engine, adios2_variable *variable)
{
    adios2::CheckForNullptr(
        engine, "for adios2_engine, in call to adios2_get_sync_self");
    adios2::CheckForNullptr(variable, "for adios2_variable, in call "
                                      "to adios2_get_sync_self");

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

void adios2_get_sync_by_name(adios2_engine *engine, const char *variable_name,
                             void *values)
{
    adios2::CheckForNullptr(
        engine, "for adios2_engine, in call to adios2_get_sync_by_name");
    adios2::CheckForNullptr(variable_name,
                            "for const char* variable_name, in call to "
                            "adios2_get_sync_by_name");

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

void adios2_get_deferred(adios2_engine *engine, adios2_variable *variable,
                         void *values)
{
    adios2::CheckForNullptr(
        engine, "for adios2_engine, in call to adios2_get_deferred");
    adios2::CheckForNullptr(variable, "for adios2_variable, in call "
                                      "to adios2_get_deferred");

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

void adios2_get_deferred_self(adios2_engine *engine, adios2_variable *variable)
{
    adios2::CheckForNullptr(
        engine, "for adios2_engine, in call to adios2_get_deferred_self");
    adios2::CheckForNullptr(variable, "for adios2_variable, in call "
                                      "to adios2_get_deferred_self");

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

void adios2_get_deferred_by_name(adios2_engine *engine,
                                 const char *variable_name, void *values)
{
    adios2::CheckForNullptr(
        engine, "for adios2_engine, in call to adios2_get_deferred_by_name");
    adios2::CheckForNullptr(variable_name,
                            "for const char* variable_name, in call "
                            "to adios2_get_deferred_by_name");

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

void adios2_perform_gets(adios2_engine *engine)
{
    adios2::CheckForNullptr(
        engine, "for adios2_engine, in call to adios2_perform_gets");
    auto &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);
    engineCpp.PerformGets();
}

void adios2_end_step(adios2_engine *engine)
{
    adios2::CheckForNullptr(engine,
                            "for adios2_engine, in call to adios2_end_step");
    auto &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);
    engineCpp.EndStep();
}

void adios2_write_step(adios2_engine *engine)
{
    adios2::CheckForNullptr(engine,
                            "for adios2_engine, in call to adios2_write_step");
    auto &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);
    engineCpp.WriteStep();
}

void adios2_flush(adios2_engine *engine)
{
    adios2::CheckForNullptr(engine,
                            "for adios2_engine, in call to adios2_flush");
    auto &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);
    engineCpp.Flush();
}

void adios2_close(adios2_engine *engine)
{
    adios2::CheckForNullptr(engine,
                            "for adios2_engine, in call to adios2_close");
    auto &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);
    engineCpp.Close();
}

void adios2_close_by_index(adios2_engine *engine,
                           const unsigned int transport_index)
{
    adios2::CheckForNullptr(
        engine, "for adios2_engine, in call to adios2_close_by_index");
    auto &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);
    engineCpp.Close(transport_index);
}
