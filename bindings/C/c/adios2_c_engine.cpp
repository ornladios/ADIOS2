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

adios2::Mode adios2_ToMode(const adios2_mode mode, const std::string &hint)
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

adios2::StepMode ToStepMode(const adios2_step_mode mode,
                            const std::string &hint)
{
    adios2::StepMode stepModeCpp = adios2::StepMode::NextAvailable;
    switch (mode)
    {
    case (adios2_step_mode_next_available):
        stepModeCpp = adios2::StepMode::NextAvailable;
        break;
    case (adios2_step_mode_append):
        stepModeCpp = adios2::StepMode::Append;
        break;
    case (adios2_step_mode_latest_available):
        stepModeCpp = adios2::StepMode::LatestAvailable;
        break;
    case (adios2_step_mode_update):
        stepModeCpp = adios2::StepMode::Update;
        break;

    default:
        throw std::invalid_argument("ERROR: invalid adios2_step_mode, " + hint +
                                    "\n");
    }
    return stepModeCpp;
}

adios2_step_status ToStepStatus(const adios2::StepStatus statusCpp,
                                const std::string &hint)
{
    adios2_step_status status = adios2_step_status_other_error;

    switch (statusCpp)
    {
    case (adios2::StepStatus::OK):
        status = adios2_step_status_ok;
        break;
    case (adios2::StepStatus::EndOfStream):
        status = adios2_step_status_end_of_stream;
        break;
    case (adios2::StepStatus::NotReady):
        status = adios2_step_status_not_ready;
        break;
    case (adios2::StepStatus::OtherError):
        status = adios2_step_status_other_error;
        break;

    default:
        throw std::invalid_argument("ERROR: invalid adios2_step_status, " +
                                    hint + "\n");
    }
    return status;
}
} // end anonymous namespace

adios2_error adios2_begin_step(adios2_engine *engine,
                               const adios2_step_mode mode,
                               const float timeout_seconds,
                               adios2_step_status *status)
{
    *status = adios2_step_status_other_error;

    try
    {
        adios2::helper::CheckForNullptr(
            engine, "for adios2_engine, in call to adios2_begin_step");
        adios2::core::Engine *engineCpp =
            reinterpret_cast<adios2::core::Engine *>(engine);

        const adios2::StepStatus statusCpp = engineCpp->BeginStep(
            ToStepMode(mode, "in call to adios2_begin_step"), timeout_seconds);

        *status = ToStepStatus(statusCpp, "in call to adios2_begin_step");
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_begin_step"));
    }
}

adios2_error adios2_current_step(size_t *current_step,
                                 const adios2_engine *engine)
{
    try
    {
        adios2::helper::CheckForNullptr(
            engine, "for adios2_engine, in call to adios2_current_step");

        const adios2::core::Engine &engineCpp =
            *reinterpret_cast<const adios2::core::Engine *>(engine);
        *current_step = engineCpp.CurrentStep();
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_current_step"));
    }
}

adios2_error adios2_put(adios2_engine *engine, adios2_variable *variable,
                        const void *data, const adios2_mode mode)
{
    try
    {
        adios2::helper::CheckForNullptr(
            engine, "for adios2_engine, in call to adios2_put");

        adios2::helper::CheckForNullptr(
            variable, "for adios2_variable, in call to adios2_put");

        adios2::core::VariableBase *variableBase =
            reinterpret_cast<adios2::core::VariableBase *>(variable);
        const std::string type(variableBase->m_Type);

        if (type == "compound")
        {
            // not supported
        }
        else if (type == "string")
        {
            const std::string dataStr(reinterpret_cast<const char *>(data));
            adios2::core::Engine &engineCpp =
                *reinterpret_cast<adios2::core::Engine *>(engine);

            engineCpp.Put(*dynamic_cast<adios2::core::Variable<std::string> *>(
                              variableBase),
                          dataStr);
        }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::helper::GetType<T>())                             \
    {                                                                          \
        adios2::core::Engine &engineCpp =                                      \
            *reinterpret_cast<adios2::core::Engine *>(engine);                 \
                                                                               \
        const adios2::Mode modeCpp = adios2_ToMode(                            \
            mode, "only adios2_mode_deferred or adios2_mode_sync are valid, "  \
                  "in call to adios2_put");                                    \
                                                                               \
        engineCpp.Put(                                                         \
            *dynamic_cast<adios2::core::Variable<T> *>(variableBase),          \
            reinterpret_cast<const T *>(data), modeCpp);                       \
    }
        ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_put"));
    }
}

adios2_error adios2_put_by_name(adios2_engine *engine,
                                const char *variable_name, const void *data,
                                const adios2_mode mode)
{
    try
    {
        adios2::helper::CheckForNullptr(
            engine, "for adios2_engine, in call to adios2_put_by_name");

        adios2::helper::CheckForNullptr(
            variable_name,
            "for const char* variable_name, in call to adios2_put_by_name");

        auto &engineCpp = *reinterpret_cast<adios2::core::Engine *>(engine);
        const adios2::Mode modeCpp = adios2_ToMode(
            mode, "only adios2_mode_deferred or adios2_mode_sync are valid, "
                  "in call to adios2_put_by_name");

        const std::string type(
            engineCpp.m_IO.InquireVariableType(variable_name));

        if (type == "compound")
        {
            // not supported
        }
        else if (type == "string")
        {
            const std::string dataStr(reinterpret_cast<const char *>(data));
            engineCpp.Put(variable_name, dataStr);
        }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::helper::GetType<T>())                             \
    {                                                                          \
        engineCpp.Put(variable_name, reinterpret_cast<const T *>(data),        \
                      modeCpp);                                                \
    }
        ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_put_by_name"));
    }
}

adios2_error adios2_perform_puts(adios2_engine *engine)
{
    try
    {
        adios2::helper::CheckForNullptr(
            engine, "for adios2_engine, in call to adios2_perform_puts");
        auto &engineCpp = *reinterpret_cast<adios2::core::Engine *>(engine);
        engineCpp.PerformPuts();
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_perform_puts"));
    }
}

adios2_error adios2_get(adios2_engine *engine, adios2_variable *variable,
                        void *values, const adios2_mode mode)
{
    try
    {
        adios2::helper::CheckForNullptr(
            engine, "for adios2_engine, in call to adios2_get");
        adios2::helper::CheckForNullptr(variable,
                                        "for adios2_variable, in call "
                                        "to adios2_get");

        adios2::core::VariableBase *variableBase =
            reinterpret_cast<adios2::core::VariableBase *>(variable);

        const std::string type(variableBase->m_Type);

        if (type == "compound")
        {
            // not supported
        }
        else if (type == "string")
        {
            adios2::core::Engine &engineCpp =
                *reinterpret_cast<adios2::core::Engine *>(engine);

            std::string dataStr;
            engineCpp.Get(*dynamic_cast<adios2::core::Variable<std::string> *>(
                              variableBase),
                          dataStr);
            dataStr.copy(reinterpret_cast<char *>(values), dataStr.size());
        }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::helper::GetType<T>())                             \
    {                                                                          \
        adios2::core::Engine &engineCpp =                                      \
            *reinterpret_cast<adios2::core::Engine *>(engine);                 \
        const adios2::Mode modeCpp = adios2_ToMode(                            \
            mode, "only adios2_mode_deferred or adios2_mode_sync are valid, "  \
                  "in call to adios2_get");                                    \
        engineCpp.Get(                                                         \
            *dynamic_cast<adios2::core::Variable<T> *>(variableBase),          \
            reinterpret_cast<T *>(values), modeCpp);                           \
    }
        ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_get"));
    }
}

adios2_error adios2_get_by_name(adios2_engine *engine,
                                const char *variable_name, void *data,
                                const adios2_mode mode)
{
    try
    {
        adios2::helper::CheckForNullptr(
            engine, "for adios2_engine, in call to adios2_get_by_name");
        adios2::helper::CheckForNullptr(
            variable_name, "for const char* variable_name, in call to "
                           "adios2_get_by_name");

        auto &engineCpp = *reinterpret_cast<adios2::core::Engine *>(engine);
        const adios2::Mode modeCpp = adios2_ToMode(
            mode, "only adios2_mode_deferred or adios2_mode_sync are valid, "
                  "in call to adios2_get_by_name");
        const std::string type(
            engineCpp.m_IO.InquireVariableType(variable_name));

        if (type == "compound")
        {
            // not supported
        }
        else if (type == "string")
        {
            std::string dataStr;
            engineCpp.Get(variable_name, dataStr);
            dataStr.copy(reinterpret_cast<char *>(data), dataStr.size());
        }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::helper::GetType<T>())                             \
    {                                                                          \
        engineCpp.Get(variable_name, reinterpret_cast<T *>(data), modeCpp);    \
    }
        ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_get_by_name"));
    }
}

adios2_error adios2_perform_gets(adios2_engine *engine)
{
    try
    {
        adios2::helper::CheckForNullptr(
            engine, "for adios2_engine, in call to adios2_perform_gets");
        auto &engineCpp = *reinterpret_cast<adios2::core::Engine *>(engine);
        engineCpp.PerformGets();
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_perform_gets"));
    }
}

adios2_error adios2_end_step(adios2_engine *engine)
{
    try
    {
        adios2::helper::CheckForNullptr(
            engine, "for adios2_engine, in call to adios2_end_step");
        auto &engineCpp = *reinterpret_cast<adios2::core::Engine *>(engine);
        engineCpp.EndStep();
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_end_step"));
    }
}

adios2_error adios2_flush(adios2_engine *engine)
{
    return adios2_flush_by_index(engine, -1);
}

adios2_error adios2_flush_by_index(adios2_engine *engine,
                                   const int transport_index)
{
    try
    {
        adios2::helper::CheckForNullptr(engine, "for adios2_engine, in call to "
                                                "adios2_flush or "
                                                "adios2_flush_by_index");
        reinterpret_cast<adios2::core::Engine *>(engine)->Flush(
            transport_index);
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(adios2::helper::ExceptionToError(
            "adios2_flush or adios2_flush_by_index"));
    }
}

adios2_error adios2_close(adios2_engine *engine)
{
    return adios2_close_by_index(engine, -1);
}

adios2_error adios2_close_by_index(adios2_engine *engine,
                                   const int transport_index)
{
    try
    {
        adios2::helper::CheckForNullptr(engine, "for adios2_engine, in call to "
                                                "adios2_close or "
                                                "adios2_close_by_index");
        auto &engineCpp = *reinterpret_cast<adios2::core::Engine *>(engine);
        engineCpp.Close(transport_index);
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(adios2::helper::ExceptionToError(
            "adios2_close or adios2_close_by_index"));
    }
}
