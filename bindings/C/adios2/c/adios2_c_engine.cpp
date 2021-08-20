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
#include "adios2/helper/adiosFunctions.h" //GetDataType<T>
#include "adios2_c_internal.h"

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

adios2_mode adios2_fromMode(const adios2::Mode mode, const std::string &hint)
{
    adios2_mode modeC = adios2_mode_undefined;
    switch (mode)
    {
    case (adios2::Mode::Read):
        modeC = adios2_mode_read;
        break;
    case (adios2::Mode::Write):
        modeC = adios2_mode_write;
        break;
    case (adios2::Mode::Append):
        modeC = adios2_mode_append;
        break;
    case (adios2::Mode::Deferred):
        modeC = adios2_mode_deferred;
        break;
    case (adios2::Mode::Sync):
        modeC = adios2_mode_sync;
        break;
    default:
        throw std::invalid_argument("ERROR: invalid adios2::Mode, " + hint +
                                    "\n");
    }
    return modeC;
}

adios2::StepMode ToStepMode(const adios2_step_mode mode,
                            const std::string &hint)
{
    adios2::StepMode stepModeCpp = adios2::StepMode::Read;
    switch (mode)
    {
    case (adios2_step_mode_append):
        stepModeCpp = adios2::StepMode::Append;
        break;
    case (adios2_step_mode_read):
        stepModeCpp = adios2::StepMode::Read;
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

adios2_error adios2_engine_name(char *name, size_t *size,
                                const adios2_engine *engine)
{
    try
    {
        adios2::helper::CheckForNullptr(
            engine, "for const adios2_engine, in call to adios2_engine_name");

        const adios2::core::Engine *engineCpp =
            reinterpret_cast<const adios2::core::Engine *>(engine);

        adios2::helper::CheckForNullptr(
            size, "for size_t* size, in call to adios2_engine_name");

        return String2CAPI(engineCpp->m_Name, name, size);
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_engine_name"));
    }
}

adios2_error adios2_engine_get_type(char *type, size_t *size,
                                    const adios2_engine *engine)
{
    try
    {
        adios2::helper::CheckForNullptr(
            engine,
            "for const adios2_engine, in call to adios2_engine_get_type");

        const adios2::core::Engine *engineCpp =
            reinterpret_cast<const adios2::core::Engine *>(engine);

        adios2::helper::CheckForNullptr(
            size, "for size_t* size, in call to adios2_engine_get_type");

        return String2CAPI(engineCpp->m_EngineType, type, size);
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_engine_get_type"));
    }
}

adios2_error adios2_engine_openmode(adios2_mode *mode,
                                    const adios2_engine *engine)
{
    try
    {
        adios2::helper::CheckForNullptr(
            engine,
            "for const adios2_engine, in call to adios2_engine_openmode");

        const adios2::core::Engine *engineCpp =
            reinterpret_cast<const adios2::core::Engine *>(engine);

        auto m = engineCpp->OpenMode();
        *mode = adios2_fromMode(m, "in adios2_engine_openmode()");
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_engine_openmode"));
    }
}

adios2_error adios2_begin_step(adios2_engine *engine,
                               const adios2_step_mode mode,
                               const float timeout_seconds,
                               adios2_step_status *status)
{
    try
    {
        adios2::helper::CheckForNullptr(
            engine, "for adios2_engine, in call to adios2_begin_step");

        adios2::core::Engine *engineCpp =
            reinterpret_cast<adios2::core::Engine *>(engine);

        if (engineCpp->m_EngineType == "NULL")
        {
            *status = adios2_step_status_end_of_stream;
            return adios2_error_none;
        }

        const adios2::StepStatus statusCpp = engineCpp->BeginStep(
            ToStepMode(mode, "in call to adios2_begin_step"), timeout_seconds);

        *status = ToStepStatus(statusCpp, "in call to adios2_begin_step");
        return adios2_error_none;
    }
    catch (...)
    {
        *status = adios2_step_status_other_error;
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

        const adios2::core::Engine *engineCpp =
            reinterpret_cast<const adios2::core::Engine *>(engine);

        if (engineCpp->m_EngineType == "NULL")
        {
            *current_step = adios2::MaxSizeT;
            return adios2_error_none;
        }

        *current_step = engineCpp->CurrentStep();
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_current_step"));
    }
}

adios2_error adios2_steps(size_t *steps, const adios2_engine *engine)
{
    try
    {
        adios2::helper::CheckForNullptr(
            engine, "for adios2_engine, in call to adios2_steps");

        const adios2::core::Engine *engineCpp =
            reinterpret_cast<const adios2::core::Engine *>(engine);

        if (engineCpp->m_EngineType == "NULL")
        {
            *steps = 0;
            return adios2_error_none;
        }

        *steps = engineCpp->Steps();
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_steps"));
    }
}

adios2_error adios2_put(adios2_engine *engine, adios2_variable *variable,
                        const void *data, const adios2_mode mode)
{
    try
    {
        adios2::helper::CheckForNullptr(
            engine, "for adios2_engine, in call to adios2_put");

        adios2::core::Engine *engineCpp =
            reinterpret_cast<adios2::core::Engine *>(engine);

        if (engineCpp->m_EngineType == "NULL")
        {
            return adios2_error_none;
        }

        adios2::helper::CheckForNullptr(
            variable, "for adios2_variable, in call to adios2_put");

        adios2::core::VariableBase *variableBase =
            reinterpret_cast<adios2::core::VariableBase *>(variable);
        const adios2::DataType type(variableBase->m_Type);

        const adios2::Mode modeCpp = adios2_ToMode(
            mode, "only adios2_mode_deferred or adios2_mode_sync are valid, "
                  "in call to adios2_put");

        if (type == adios2::DataType::Compound)
        {
            // not supported
        }
        else if (type == adios2::helper::GetDataType<std::string>())
        {
            const std::string dataStr(reinterpret_cast<const char *>(data));
            engineCpp->Put(*dynamic_cast<adios2::core::Variable<std::string> *>(
                               variableBase),
                           dataStr, modeCpp);
        }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::helper::GetDataType<T>())                         \
    {                                                                          \
        engineCpp->Put(                                                        \
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

        adios2::core::Engine *engineCpp =
            reinterpret_cast<adios2::core::Engine *>(engine);

        if (engineCpp->m_EngineType == "NULL")
        {
            return adios2_error_none;
        }

        adios2::helper::CheckForNullptr(
            variable_name,
            "for const char* variable_name, in call to adios2_put_by_name");

        const adios2::Mode modeCpp = adios2_ToMode(
            mode, "only adios2_mode_deferred or adios2_mode_sync are valid, "
                  "in call to adios2_put_by_name");

        const adios2::DataType type(
            engineCpp->m_IO.InquireVariableType(variable_name));

        if (type == adios2::DataType::Compound)
        {
            // not supported
        }
        else if (type == adios2::helper::GetDataType<std::string>())
        {
            const std::string dataStr(reinterpret_cast<const char *>(data));
            engineCpp->Put(variable_name, dataStr, modeCpp);
        }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::helper::GetDataType<T>())                         \
    {                                                                          \
        engineCpp->Put(variable_name, reinterpret_cast<const T *>(data),       \
                       modeCpp);                                               \
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

        adios2::core::Engine *engineCpp =
            reinterpret_cast<adios2::core::Engine *>(engine);

        if (engineCpp->m_EngineType == "NULL")
        {
            return adios2_error_none;
        }

        engineCpp->PerformPuts();
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

        adios2::core::Engine *engineCpp =
            reinterpret_cast<adios2::core::Engine *>(engine);

        if (engineCpp->m_EngineType == "NULL")
        {
            return adios2_error_none;
        }

        adios2::helper::CheckForNullptr(variable,
                                        "for adios2_variable, in call "
                                        "to adios2_get");

        adios2::core::VariableBase *variableBase =
            reinterpret_cast<adios2::core::VariableBase *>(variable);

        const adios2::DataType type(variableBase->m_Type);

        if (type == adios2::DataType::Compound)
        {
            // not supported
        }
        else if (type == adios2::helper::GetDataType<std::string>())
        {
            std::string dataStr;
            engineCpp->Get(*dynamic_cast<adios2::core::Variable<std::string> *>(
                               variableBase),
                           dataStr);
            dataStr.copy(reinterpret_cast<char *>(values), dataStr.size());
        }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::helper::GetDataType<T>())                         \
    {                                                                          \
        const adios2::Mode modeCpp = adios2_ToMode(                            \
            mode, "only adios2_mode_deferred or adios2_mode_sync are valid, "  \
                  "in call to adios2_get");                                    \
        engineCpp->Get(                                                        \
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

        adios2::core::Engine *engineCpp =
            reinterpret_cast<adios2::core::Engine *>(engine);

        if (engineCpp->m_EngineType == "NULL")
        {
            return adios2_error_none;
        }

        adios2::helper::CheckForNullptr(
            variable_name, "for const char* variable_name, in call to "
                           "adios2_get_by_name");

        const adios2::Mode modeCpp = adios2_ToMode(
            mode, "only adios2_mode_deferred or adios2_mode_sync are valid, "
                  "in call to adios2_get_by_name");
        const adios2::DataType type(
            engineCpp->m_IO.InquireVariableType(variable_name));

        if (type == adios2::DataType::Compound)
        {
            // not supported
        }
        else if (type == adios2::helper::GetDataType<std::string>())
        {
            std::string dataStr;
            engineCpp->Get(variable_name, dataStr);
            dataStr.copy(reinterpret_cast<char *>(data), dataStr.size());
        }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::helper::GetDataType<T>())                         \
    {                                                                          \
        engineCpp->Get(variable_name, reinterpret_cast<T *>(data), modeCpp);   \
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

        adios2::core::Engine *engineCpp =
            reinterpret_cast<adios2::core::Engine *>(engine);

        if (engineCpp->m_EngineType == "NULL")
        {
            return adios2_error_none;
        }

        engineCpp->PerformGets();
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

        adios2::core::Engine *engineCpp =
            reinterpret_cast<adios2::core::Engine *>(engine);

        if (engineCpp->m_EngineType == "NULL")
        {
            return adios2_error_none;
        }

        engineCpp->EndStep();
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
        adios2::core::Engine *engineCpp =
            reinterpret_cast<adios2::core::Engine *>(engine);

        if (engineCpp->m_EngineType == "NULL")
        {
            return adios2_error_none;
        }

        engineCpp->Flush(transport_index);

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

adios2_error adios2_lock_writer_definitions(adios2_engine *engine)
{
    try
    {
        adios2::helper::CheckForNullptr(
            engine,
            "for adios2_engine, in call to adios2_lock_writer_definitions");
        reinterpret_cast<adios2::core::Engine *>(engine)
            ->LockWriterDefinitions();
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_engine_type"));
    }
}

adios2_error adios2_lock_reader_selections(adios2_engine *engine)
{
    try
    {
        adios2::helper::CheckForNullptr(
            engine,
            "for adios2_engine, in call to adios2_lock_reader_selections");
        reinterpret_cast<adios2::core::Engine *>(engine)
            ->LockReaderSelections();
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_engine_type"));
    }
}

adios2_varinfo *adios2_inquire_blockinfo(adios2_engine *engine,
                                         adios2_variable *variable,
                                         const size_t step)
{
    auto lf_CopyDims = [](const std::vector<size_t> &dims) -> size_t * {
        size_t *a = nullptr;
        size_t ndims = dims.size();
        if (ndims > 0)
        {
            a = (size_t *)malloc(dims.size() * sizeof(size_t));
            std::memcpy(a, dims.data(), dims.size() * sizeof(size_t));
        }
        return a;
    };

    adios2_varinfo *varinfo = NULL;

    try
    {
        adios2::helper::CheckForNullptr(
            engine, "for adios2_engine, in call to adios2_inquire_blockinfo");

        adios2::core::Engine *engineCpp =
            reinterpret_cast<adios2::core::Engine *>(engine);

        if (engineCpp->m_EngineType == "NULL")
        {
            return NULL;
        }
        adios2::helper::CheckForNullptr(variable,
                                        "for adios2_variable, in call "
                                        "to adios2_get");

        adios2::core::VariableBase *variableBase =
            reinterpret_cast<adios2::core::VariableBase *>(variable);

        const adios2::DataType type(variableBase->m_Type);

        const auto minBlocksInfo =
            engineCpp->MinBlocksInfo(*variableBase, step);

        if (minBlocksInfo)
        {
            varinfo = (adios2_varinfo *)malloc(sizeof(adios2_varinfo));
            varinfo->nblocks = minBlocksInfo->BlocksInfo.size();
            varinfo->BlocksInfo = (adios2_blockinfo *)malloc(
                varinfo->nblocks * sizeof(adios2_blockinfo));
            auto *b = varinfo->BlocksInfo;

            varinfo->Dims = minBlocksInfo->Dims;
            varinfo->Shape = minBlocksInfo->Shape;
            varinfo->IsValue = (int)minBlocksInfo->IsValue;
            varinfo->IsReverseDims = (int)minBlocksInfo->IsReverseDims;
            for (size_t i = 0; i < varinfo->nblocks; ++i)
            {
                b[i].WriterID = minBlocksInfo->BlocksInfo[i].WriterID;
                b[i].BlockID = minBlocksInfo->BlocksInfo[i].BlockID;
                b[i].Start = minBlocksInfo->BlocksInfo[i].Start;
                b[i].Count = minBlocksInfo->BlocksInfo[i].Count;
                if (minBlocksInfo->IsValue)
                {
                    b[i].Value.uint64 = 0;
                    //  = *((T *)minBlocksInfo->BlocksInfo[i].BufferP);
                }
                else
                {
                    b[i].MinUnion.uint64 = 0;
                    //  = minBlocksInfo->BlocksInfo[i].MinUnion;
                    b[i].MaxUnion.uint64 = 0;
                    //  = minBlocksInfo->BlocksInfo[i].MaxUnion;
                }
            }
            delete minBlocksInfo;
            return varinfo;
        }

        /* Call the big gun Engine::BlocksInfo<T> */
        if (type == adios2::DataType::Compound)
        {
            return varinfo;
        }
        else if (type == adios2::helper::GetDataType<std::string>())
        {
            const auto blocksInfo = engineCpp->BlocksInfo<std::string>(
                *dynamic_cast<adios2::core::Variable<std::string> *>(
                    variableBase),
                step);
            varinfo = (adios2_varinfo *)malloc(sizeof(adios2_varinfo));
            varinfo->nblocks = blocksInfo.size();
            varinfo->BlocksInfo = (adios2_blockinfo *)malloc(
                varinfo->nblocks * sizeof(adios2_blockinfo));
            auto *b = varinfo->BlocksInfo;

            varinfo->Dims = static_cast<int>(blocksInfo[0].Shape.size());
            varinfo->Shape = lf_CopyDims(blocksInfo[0].Shape);
            varinfo->IsValue = (int)blocksInfo[0].IsValue;
            varinfo->IsReverseDims = (int)blocksInfo[0].IsReverseDims;
            for (size_t i = 0; i < varinfo->nblocks; ++i)
            {
                b[i].WriterID = blocksInfo[i].WriterID;
                b[i].BlockID = blocksInfo[i].BlockID;
                b[i].Start = lf_CopyDims(blocksInfo[i].Start);
                b[i].Count = lf_CopyDims(blocksInfo[i].Count);
                // minBlocksInfo->BlocksInfo[i].MinUnion;
                b[i].MinUnion.uint64 = 0;
                // minBlocksInfo->BlocksInfo[i].MaxUnion;
                b[i].MaxUnion.uint64 = 0;
                b[i].Value.str = (char *)malloc(blocksInfo[i].Value.size() + 1);
                std::strcpy(b[i].Value.str, blocksInfo[i].Value.data());
            };
        }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::helper::GetDataType<T>())                         \
    {                                                                          \
        const auto blocksInfo = engineCpp->BlocksInfo<T>(                      \
            *dynamic_cast<adios2::core::Variable<T> *>(variableBase), step);   \
        varinfo = (adios2_varinfo *)malloc(sizeof(adios2_varinfo));            \
        varinfo->nblocks = blocksInfo.size();                                  \
        varinfo->BlocksInfo = (adios2_blockinfo *)malloc(                      \
            varinfo->nblocks * sizeof(adios2_blockinfo));                      \
        auto *b = varinfo->BlocksInfo;                                         \
                                                                               \
        varinfo->Dims = static_cast<int>(blocksInfo[0].Shape.size());          \
        varinfo->Shape = lf_CopyDims(blocksInfo[0].Shape);                     \
        varinfo->IsValue = (int)blocksInfo[0].IsValue;                         \
        varinfo->IsReverseDims = (int)blocksInfo[0].IsReverseDims;             \
        for (size_t i = 0; i < varinfo->nblocks; ++i)                          \
        {                                                                      \
            b[i].WriterID = blocksInfo[i].WriterID;                            \
            b[i].BlockID = blocksInfo[i].BlockID;                              \
            b[i].Start = lf_CopyDims(blocksInfo[i].Start);                     \
            b[i].Count = lf_CopyDims(blocksInfo[i].Count);                     \
            b[i].MinUnion.uint64 = 0;                                          \
            b[i].MaxUnion.uint64 = 0;                                          \
            b[i].Value.uint64 = 0;                                             \
        };                                                                     \
    }
        ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
    }
    catch (...)
    {
        adios2::helper::ExceptionToError("adios2_inquire_blockinfo");
        return NULL;
    }
    return varinfo;
}

adios2_error adios2_close_by_index(adios2_engine *engine,
                                   const int transport_index)
{
    try
    {
        adios2::helper::CheckForNullptr(engine, "for adios2_engine, in call to "
                                                "adios2_close or "
                                                "adios2_close_by_index");
        adios2::core::Engine *engineCpp =
            reinterpret_cast<adios2::core::Engine *>(engine);

        if (engineCpp->m_EngineType == "NULL")
        {
            return adios2_error_none;
        }

        engineCpp->Close(transport_index);

        // erase Engine object from IO
        adios2::core::IO &io = engineCpp->GetIO();
        const std::string name = engineCpp->m_Name;
        io.RemoveEngine(name);

        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(adios2::helper::ExceptionToError(
            "adios2_close or adios2_close_by_index"));
    }
}
