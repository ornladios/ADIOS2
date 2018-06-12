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

namespace
{

adios2::Mode ToMode(adios2_mode mode, const std::string hint)
{
    adios2::Mode modeCpp = adios2::Mode::Undefined;
    switch (mode)
    {
    case (adios2_mode_deferred):
        modeCpp = adios2::Mode::Deferred;
        break;
    case (adios2_mode_sync):
        modeCpp = adios2::Mode::Sync;
        break;
    default:
        throw std::invalid_argument("ERROR: invalid adios2_mode, " + hint +
                                    "\n");
    }
    return modeCpp;
}
}

adios2_step_status adios2_begin_step(adios2_engine *engine,
                                     const adios2_step_mode mode,
                                     const float timeout_seconds)
{
    adios2::helper::CheckForNullptr(
        engine, "for adios2_engine, in call to adios2_begin_step");
    auto &engineCpp = *reinterpret_cast<adios2::core::Engine *>(engine);

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

size_t adios2_current_step(const adios2_engine *engine)
{
    adios2::helper::CheckForNullptr(
        engine, "for adios2_engine, in call to adios2_current_step");

    const adios2::core::Engine &engineCpp =
        *reinterpret_cast<const adios2::core::Engine *>(engine);
    return engineCpp.CurrentStep();
}

void adios2_put(adios2_engine *engine, adios2_variable *variable,
                const void *data, const adios2_mode mode)
{
    adios2::helper::CheckForNullptr(engine,
                                    "for adios2_engine, in call to adios2_put");

    adios2::helper::CheckForNullptr(
        variable, "for adios2_variable, in call to adios2_put");

    adios2::core::VariableBase *variableBase =
        reinterpret_cast<adios2::core::VariableBase *>(variable);
    const std::string type(variableBase->m_Type);

    if (type == "compound")
    {
        // not supported
    }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::helper::GetType<T>())                             \
    {                                                                          \
        adios2::core::Engine &engineCpp =                                      \
            *reinterpret_cast<adios2::core::Engine *>(engine);                 \
                                                                               \
        const adios2::Mode modeCpp = ToMode(                                   \
            mode, "only adios2_mode_deferred or adios2_mode_sync are valid, "  \
                  "in call to adios2_put");                                    \
                                                                               \
        engineCpp.Put(                                                         \
            *dynamic_cast<adios2::core::Variable<T> *>(variableBase),          \
            reinterpret_cast<const T *>(data), modeCpp);                       \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
}

void adios2_put_by_name(adios2_engine *engine, const char *variable_name,
                        const void *data, const adios2_mode mode)
{
    adios2::helper::CheckForNullptr(
        engine, "for adios2_engine, in call to adios2_put_by_name");

    adios2::helper::CheckForNullptr(
        variable_name,
        "for const char* variable_name, in call to adios2_put_by_name");

    auto &engineCpp = *reinterpret_cast<adios2::core::Engine *>(engine);
    const adios2::Mode modeCpp =
        ToMode(mode, "only adios2_mode_deferred or adios2_mode_sync are valid, "
                     "in call to adios2_put_by_name");

    const std::string type(
        engineCpp.GetIO().InquireVariableType(variable_name));

    if (type == "compound")
    {
        // not supported
    }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::helper::GetType<T>())                             \
    {                                                                          \
        engineCpp.Put(variable_name, reinterpret_cast<const T *>(data),        \
                      modeCpp);                                                \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
}

void adios2_perform_puts(adios2_engine *engine)
{
    adios2::helper::CheckForNullptr(
        engine, "for adios2_engine, in call to adios2_perform_puts");
    auto &engineCpp = *reinterpret_cast<adios2::core::Engine *>(engine);
    engineCpp.PerformPuts();
}

void adios2_get(adios2_engine *engine, adios2_variable *variable, void *values,
                const adios2_mode mode)
{
    adios2::helper::CheckForNullptr(engine,
                                    "for adios2_engine, in call to adios2_get");
    adios2::helper::CheckForNullptr(variable, "for adios2_variable, in call "
                                              "to adios2_get");

    adios2::core::VariableBase *variableBase =
        reinterpret_cast<adios2::core::VariableBase *>(variable);
    const std::string type(variableBase->m_Type);

    if (type == "compound")
    {
        // not supported
    }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::helper::GetType<T>())                             \
    {                                                                          \
        adios2::core::Engine &engineCpp =                                      \
            *reinterpret_cast<adios2::core::Engine *>(engine);                 \
        const adios2::Mode modeCpp = ToMode(                                   \
            mode, "only adios2_mode_deferred or adios2_mode_sync are valid, "  \
                  "in call to adios2_get");                                    \
        engineCpp.Get(                                                         \
            *dynamic_cast<adios2::core::Variable<T> *>(variableBase),          \
            reinterpret_cast<T *>(values), modeCpp);                           \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
}

void adios2_get_by_name(adios2_engine *engine, const char *variable_name,
                        void *data, const adios2_mode mode)
{
    adios2::helper::CheckForNullptr(
        engine, "for adios2_engine, in call to adios2_get_by_name");
    adios2::helper::CheckForNullptr(variable_name,
                                    "for const char* variable_name, in call to "
                                    "adios2_get_by_name");

    auto &engineCpp = *reinterpret_cast<adios2::core::Engine *>(engine);
    const adios2::Mode modeCpp =
        ToMode(mode, "only adios2_mode_deferred or adios2_mode_sync are valid, "
                     "in call to adios2_get_by_name");
    const std::string type(
        engineCpp.GetIO().InquireVariableType(variable_name));

    if (type == "compound")
    {
        // not supported
    }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::helper::GetType<T>())                             \
    {                                                                          \
        engineCpp.Get(variable_name, reinterpret_cast<T *>(data), modeCpp);    \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
}

void adios2_perform_gets(adios2_engine *engine)
{
    adios2::helper::CheckForNullptr(
        engine, "for adios2_engine, in call to adios2_perform_gets");
    auto &engineCpp = *reinterpret_cast<adios2::core::Engine *>(engine);
    engineCpp.PerformGets();
}

void adios2_end_step(adios2_engine *engine)
{
    adios2::helper::CheckForNullptr(
        engine, "for adios2_engine, in call to adios2_end_step");
    auto &engineCpp = *reinterpret_cast<adios2::core::Engine *>(engine);
    engineCpp.EndStep();
}

void adios2_flush(adios2_engine *engine) { adios2_flush_by_index(engine, -1); }

void adios2_flush_by_index(adios2_engine *engine, const int transport_index)
{
    adios2::helper::CheckForNullptr(
        engine, "for adios2_engine, in call to adios2_flush_by_index");
    auto &engineCpp = *reinterpret_cast<adios2::core::Engine *>(engine);
    engineCpp.Flush(transport_index);
}

void adios2_close(adios2_engine *engine) { adios2_close_by_index(engine, -1); }

void adios2_close_by_index(adios2_engine *engine, const int transport_index)
{
    adios2::helper::CheckForNullptr(
        engine, "for adios2_engine, in call to adios2_close_by_index");
    auto &engineCpp = *reinterpret_cast<adios2::core::Engine *>(engine);
    engineCpp.Close(transport_index);
}
