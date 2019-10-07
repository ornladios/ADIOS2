/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_c_adios.cpp
 *
 *  Created on: Nov 8, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adios2_c_adios.h"

#include "adios2/core/ADIOS.h"
#include "adios2/helper/adiosFunctions.h"

#ifdef ADIOS2_HAVE_MPI
#include "adios2/helper/adiosCommMPI.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ADIOS2_HAVE_MPI

// to be called from other languages, hidden from the public apis
adios2_adios *adios2_init_config_glue(const char *config_file, MPI_Comm comm,
                                      const adios2_debug_mode debug_mode,
                                      const char *host_language)
{
    adios2_adios *adios = nullptr;

    try
    {
        adios2::helper::CheckForNullptr(
            config_file,
            "for config_file, in call to adios2_init or adios2_init_config");
        const bool debugBool =
            (debug_mode == adios2_debug_mode_on) ? true : false;
        adios = reinterpret_cast<adios2_adios *>(new adios2::core::ADIOS(
            config_file, adios2::helper::CommFromMPI(comm), debugBool,
            host_language));
    }
    catch (...)
    {
        adios2::helper::ExceptionToError("adios2_init or adios2_init_config");
    }
    return adios;
}

adios2_adios *adios2_init(MPI_Comm comm, const adios2_debug_mode debug_mode)
{
    return adios2_init_config("", comm, debug_mode);
}

adios2_adios *adios2_init_config(const char *config_file, MPI_Comm comm,
                                 const adios2_debug_mode debug_mode)
{
    return adios2_init_config_glue(config_file, comm, debug_mode, "C");
}

#else

adios2_adios *adios2_init_config_glue(const char *config_file,
                                      const adios2_debug_mode debug_mode,
                                      const char *host_language)
{
    adios2_adios *adios = nullptr;
    try
    {
        adios2::helper::CheckForNullptr(
            config_file,
            "for config_file, in call to adios2_init or adios2_init_config");
        const bool debugBool =
            (debug_mode == adios2_debug_mode_on) ? true : false;
        adios = reinterpret_cast<adios2_adios *>(
            new adios2::core::ADIOS(config_file, debugBool, host_language));
    }
    catch (...)
    {
        adios2::helper::ExceptionToError("adios2_init or adios2_init_config");
    }
    return adios;
}

adios2_adios *adios2_init(const adios2_debug_mode debug_mode)
{
    return adios2_init_config("", debug_mode);
}

adios2_adios *adios2_init_config(const char *config_file,
                                 const adios2_debug_mode debug_mode)
{
    return adios2_init_config_glue("", debug_mode, "C");
}
#endif

adios2_io *adios2_declare_io(adios2_adios *adios, const char *name)
{
    adios2_io *io = nullptr;
    try
    {
        adios2::helper::CheckForNullptr(
            adios, "for adios2_adios, in call to adios2_declare_io");
        io = reinterpret_cast<adios2_io *>(
            &reinterpret_cast<adios2::core::ADIOS *>(adios)->DeclareIO(name));
    }
    catch (...)
    {
        adios2::helper::ExceptionToError("adios2_declare_io");
    }
    return io;
}

adios2_io *adios2_at_io(adios2_adios *adios, const char *name)
{
    adios2_io *io = nullptr;
    try
    {
        adios2::helper::CheckForNullptr(
            adios, "for adios2_adios, in call to adios2_at_io");
        io = reinterpret_cast<adios2_io *>(
            &reinterpret_cast<adios2::core::ADIOS *>(adios)->AtIO(name));
    }
    catch (...)
    {
        adios2::helper::ExceptionToError("adios2_at_io");
    }
    return io;
}

adios2_operator *adios2_define_operator(adios2_adios *adios, const char *name,
                                        const char *type)
{
    adios2_operator *op = nullptr;
    try
    {
        adios2::helper::CheckForNullptr(
            adios, "for adios2_adios, in call to adios2_define_operator");
        op = reinterpret_cast<adios2_operator *>(
            &reinterpret_cast<adios2::core::ADIOS *>(adios)->DefineOperator(
                name, type));
    }
    catch (...)
    {
        adios2::helper::ExceptionToError("adios2_define_operator");
    }
    return op;
}

adios2_operator *adios2_inquire_operator(adios2_adios *adios, const char *name)
{
    adios2_operator *op = nullptr;
    try
    {
        adios2::helper::CheckForNullptr(
            adios, "for adios2_adios, in call to adios2_inquire_operator");
        op = reinterpret_cast<adios2_operator *>(
            reinterpret_cast<adios2::core::ADIOS *>(adios)->InquireOperator(
                name));
    }
    catch (...)
    {
        adios2::helper::ExceptionToError("adios2_inquire_operator");
    }
    return op;
}

adios2_error adios2_flush_all(adios2_adios *adios)
{
    try
    {
        adios2::helper::CheckForNullptr(
            adios, "for adios2_adios, in call to adios2_flush_all");
        reinterpret_cast<adios2::core::ADIOS *>(adios)->FlushAll();
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_flush_all"));
    }
}

adios2_error adios2_remove_io(adios2_bool *result, adios2_adios *adios,
                              const char *name)
{
    try
    {
        adios2::helper::CheckForNullptr(
            adios, "for adios2_adios, in call to adios2_remove_io");
        const bool resultCpp =
            reinterpret_cast<adios2::core::ADIOS *>(adios)->RemoveIO(name);
        *result = resultCpp ? adios2_true : adios2_false;
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_remove_io"));
    }
}

adios2_error adios2_remove_all_ios(adios2_adios *adios)
{
    try
    {
        adios2::helper::CheckForNullptr(
            adios, "for adios2_adios, in call to adios2_remove_all_ios");
        reinterpret_cast<adios2::core::ADIOS *>(adios)->RemoveAllIOs();
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_remove_all_ios"));
    }
}

adios2_error adios2_finalize(adios2_adios *adios)
{
    try
    {
        adios2::helper::CheckForNullptr(
            adios, "for adios2_adios, in call to adios2_finalize");
        delete reinterpret_cast<adios2::core::ADIOS *>(adios);
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_finalize"));
    }
}

#ifdef __cplusplus
}
// end extern C
#endif
