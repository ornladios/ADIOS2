/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_c_FILE.cpp
 *
 *  Created on: Jan 8, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adios2_c_FILE.h"

#include "adios2/core/Stream.h"
#include "adios2/core/Variable.h"
#include "adios2/helper/adiosFunctions.h" //CheckForNullptr

#ifdef _WIN32
#pragma warning(disable : 4297) // Windows noexcept default functions
#endif

namespace
{
template <class T>
void adios2_fread_common(adios2::core::Stream &stream, const std::string &name,
                         T *data, const adios2::Box<adios2::Dims> &selection)
{
    if (selection.first.empty() && selection.second.empty())
    {
        stream.Read(name, data);
        return;
    }

    stream.Read(name, data, selection);
}

template <class T>
void adios2_fread_steps_common(adios2::core::Stream &stream,
                               const std::string &name, T *data,
                               const adios2::Box<adios2::Dims> &selection,
                               const adios2::Box<size_t> &stepSelection,
                               const bool stepOnly)
{
    if (stepOnly)
    {
        stream.Read(name, data, stepSelection);
    }
    else
    {
        stream.Read(name, data, selection, stepSelection);
    }
}

} // end empty namespace

#ifdef __cplusplus
extern "C" {
#endif

namespace
{

adios2::Mode adios2_ToMode(const char *modeC)
{
    adios2::Mode mode = adios2::Mode::Undefined;
    const std::string modeStr(modeC);

    if (modeStr == "w")
    {
        mode = adios2::Mode::Write;
    }
    else if (modeStr == "r")
    {
        mode = adios2::Mode::Read;
    }
    else if (modeStr == "a")
    {
        mode = adios2::Mode::Append;
    }
    else
    {
        throw std::invalid_argument("ERROR: invalid mode " + modeStr +
                                    ", in call to adios2_fopen\n");
    }

    return mode;
}
} // end anonymous namespace

#ifdef ADIOS2_HAVE_MPI
adios2_FILE *adios2_fopen_glue(const char *name, const char *mode,
                               MPI_Comm comm, const char *host_language)
{
    adios2_FILE *stream = nullptr;

    try
    {
        adios2::helper::CheckForNullptr(
            name, "for name, in adios2_fopen or adios2_fopen_config");

        adios2::core::Stream *streamCpp = new adios2::core::Stream(
            name, adios2_ToMode(mode), comm, "BPFile", host_language);
        stream = reinterpret_cast<adios2_FILE *>(streamCpp);
    }
    catch (...)
    {
        adios2::helper::ExceptionToError("adios2_fopen");
    }
    return stream;
}

adios2_FILE *adios2_fopen_config_glue(const char *name, const char *mode,
                                      MPI_Comm comm, const char *config_file,
                                      const char *io_in_config_file,
                                      const char *host_language)
{
    adios2_FILE *stream = nullptr;

    try
    {
        adios2::helper::CheckForNullptr(
            name, "for name, in adios2_fopen or adios2_fopen_config");

        adios2::core::Stream *streamCpp = new adios2::core::Stream(
            name, adios2_ToMode(mode), comm, config_file, io_in_config_file,
            host_language);
        stream = reinterpret_cast<adios2_FILE *>(streamCpp);
    }
    catch (...)
    {
        adios2::helper::ExceptionToError("adios2_fopen_config");
    }
    return stream;
}

adios2_FILE *adios2_fopen(const char *name, const char *mode, MPI_Comm comm)
{
    return adios2_fopen_glue(name, mode, comm, "C");
}

adios2_FILE *adios2_fopen_config(const char *name, const char *mode,
                                 MPI_Comm comm, const char *config_file,
                                 const char *io_in_config_file)
{
    return adios2_fopen_config_glue(name, mode, comm, config_file,
                                    io_in_config_file, "C");
}

#else
adios2_FILE *adios2_fopen_glue(const char *name, const char *mode,
                               const char *host_language)
{
    adios2_FILE *stream = nullptr;

    try
    {
        adios2::helper::CheckForNullptr(
            name, "for name, in adios2_fopen or adios2_fopen_config");

        adios2::core::Stream *streamCpp = new adios2::core::Stream(
            name, adios2_ToMode(mode), "BPFile", host_language);
        stream = reinterpret_cast<adios2_FILE *>(streamCpp);
    }
    catch (...)
    {
        adios2::helper::ExceptionToError("adios2_fopen");
    }
    return stream;
}

adios2_FILE *adios2_fopen_config_glue(const char *name, const char *mode,
                                      const char *config_file,
                                      const char *io_in_config_file,
                                      const char *host_language)
{
    adios2_FILE *stream = nullptr;

    try
    {
        adios2::helper::CheckForNullptr(
            name, "for name, in adios2_fopen or adios2_fopen_config");

        adios2::core::Stream *streamCpp =
            new adios2::core::Stream(name, adios2_ToMode(mode), config_file,
                                     io_in_config_file, host_language);
        stream = reinterpret_cast<adios2_FILE *>(streamCpp);
    }
    catch (...)
    {
        adios2::helper::ExceptionToError("adios2_fopen_config");
    }
    return stream;
}

adios2_FILE *adios2_fopen(const char *name, const char *mode)
{
    return adios2_fopen_glue(name, mode, "C");
}

adios2_FILE *adios2_fopen_config(const char *name, const char *mode,
                                 const char *config_file,
                                 const char *io_in_config_file)
{
    return adios2_fopen_config_glue(name, mode, config_file, io_in_config_file,
                                    "C");
}
#endif

adios2_error adios2_fwrite(adios2_FILE *stream, const char *name,
                           const adios2_type type, const void *data,
                           const size_t ndims, const size_t *shape,
                           const size_t *start, const size_t *count,
                           const adios2_bool end_step)
{
    try
    {
        adios2::helper::CheckForNullptr(
            name, "null name, in call to adios2_fwrite\n");

        adios2::helper::CheckForNullptr(
            stream, "null adios2_FILE for variable " + std::string(name) +
                        ", in call to adios2_fwrite\n");

        adios2::helper::CheckForNullptr(
            data, "null data for variable " + std::string(name) +
                      ", in call to adios2_fwrite\n");

        auto lf_ConvertDims = [](const size_t ndims,
                                 const size_t *in) -> adios2::Dims {

            if (in != nullptr)
            {
                return adios2::Dims(in, in + ndims);
            }
            return adios2::Dims();
        };

        const adios2::Dims shapeV = lf_ConvertDims(ndims, shape);
        const adios2::Dims startV = lf_ConvertDims(ndims, start);
        const adios2::Dims countV = lf_ConvertDims(ndims, count);

        adios2::core::Stream &streamCpp =
            *reinterpret_cast<adios2::core::Stream *>(stream);
        const bool endStep = (end_step == adios2_true) ? true : false;

        switch (type)
        {

        case (adios2_type_string):
        {
            const std::string stringCpp(reinterpret_cast<const char *>(data));
            streamCpp.Write(name, stringCpp);
            break;
        }
        case (adios2_type_char):
        {
            streamCpp.Write(name, reinterpret_cast<const char *>(data), shapeV,
                            startV, countV);

            break;
        }
        case (adios2_type_signed_char):
        {
            streamCpp.Write(name, reinterpret_cast<const signed char *>(data),
                            shapeV, startV, countV);
            break;
        }
        case (adios2_type_short):
        {
            streamCpp.Write(name, reinterpret_cast<const short *>(data), shapeV,
                            startV, countV);
            break;
        }
        case (adios2_type_int):
        {
            streamCpp.Write(name, reinterpret_cast<const int *>(data), shapeV,
                            startV, countV);
            break;
        }
        case (adios2_type_long_int):
        {
            streamCpp.Write(name, reinterpret_cast<const long int *>(data),
                            shapeV, startV, countV);
            break;
        }
        case (adios2_type_long_long_int):
        {
            streamCpp.Write(name, reinterpret_cast<const long long int *>(data),
                            shapeV, startV, countV);
            break;
        }
        case (adios2_type_unsigned_char):
        {
            streamCpp.Write(name, reinterpret_cast<const unsigned char *>(data),
                            shapeV, startV, countV);
            break;
        }
        case (adios2_type_unsigned_short):
        {
            streamCpp.Write(name,
                            reinterpret_cast<const unsigned short *>(data),
                            shapeV, startV, countV);
            break;
        }
        case (adios2_type_unsigned_int):
        {
            streamCpp.Write(name, reinterpret_cast<const unsigned int *>(data),
                            shapeV, startV, countV);
            break;
        }
        case (adios2_type_unsigned_long_int):
        {
            streamCpp.Write(name,
                            reinterpret_cast<const unsigned long int *>(data),
                            shapeV, startV, countV);
            break;
        }
        case (adios2_type_unsigned_long_long_int):
        {
            streamCpp.Write(
                name, reinterpret_cast<const unsigned long long int *>(data),
                shapeV, startV, countV);
            break;
        }
        case (adios2_type_float):
        {
            streamCpp.Write(name, reinterpret_cast<const float *>(data), shapeV,
                            startV, countV);
            break;
        }
        case (adios2_type_double):
        {
            streamCpp.Write(name, reinterpret_cast<const double *>(data),
                            shapeV, startV, countV);
            break;
        }
        case (adios2_type_float_complex):
        {
            streamCpp.Write(name,
                            reinterpret_cast<const std::complex<float> *>(data),
                            shapeV, startV, countV);
            break;
        }
        case (adios2_type_double_complex):
        {
            streamCpp.Write(
                name, reinterpret_cast<const std::complex<double> *>(data),
                shapeV, startV, countV);
            break;
        }
        case (adios2_type_int8_t):
        {
            streamCpp.Write(name, reinterpret_cast<const int8_t *>(data),
                            shapeV, startV, countV);
            break;
        }
        case (adios2_type_int16_t):
        {
            streamCpp.Write(name, reinterpret_cast<const int16_t *>(data),
                            shapeV, startV, countV);
            break;
        }
        case (adios2_type_int32_t):
        {
            streamCpp.Write(name, reinterpret_cast<const int32_t *>(data),
                            shapeV, startV, countV);
            break;
        }
        case (adios2_type_int64_t):
        {
            streamCpp.Write(name, reinterpret_cast<const int64_t *>(data),
                            shapeV, startV, countV);
            break;
        }
        case (adios2_type_uint8_t):
        {
            streamCpp.Write(name, reinterpret_cast<const uint8_t *>(data),
                            shapeV, startV, countV);
            break;
        }
        case (adios2_type_uint16_t):
        {
            streamCpp.Write(name, reinterpret_cast<const uint16_t *>(data),
                            shapeV, startV, countV);
            break;
        }
        case (adios2_type_uint32_t):
        {
            streamCpp.Write(name, reinterpret_cast<const uint32_t *>(data),
                            shapeV, startV, countV);
            break;
        }
        case (adios2_type_uint64_t):
        {
            streamCpp.Write(name, reinterpret_cast<const uint64_t *>(data),
                            shapeV, startV, countV);
            break;
        }
        default:
        {
            throw std::invalid_argument(
                "ERROR: type not supported for variable " + std::string(name) +
                ", in call to adios2_fwrite\n");
        }
        }
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_fwrite"));
    }
}

adios2_step *adios2_fgets(adios2_step *step, adios2_FILE *stream)
{
    step = nullptr;
    try
    {
        adios2::helper::CheckForNullptr(
            stream, "null adios2_FILE, in call to adios2_fgets\n");

        adios2::core::Stream &streamCpp =
            *reinterpret_cast<adios2::core::Stream *>(stream);

        if (streamCpp.GetStep())
        {
            step = stream;
        }
    }
    catch (...)
    {
        adios2::helper::ExceptionToError("adios2_fgets");
    }
    return step;
}

adios2_error adios2_fread(adios2_FILE *stream, const char *name,
                          const adios2_type type, void *data,
                          const size_t ndims, const size_t *start,
                          const size_t *count)
{
    try
    {
        adios2::helper::CheckForNullptr(
            stream, "null adios2_FILE for variable " + std::string(name) +
                        ", in call to adios2_fread\n");

        auto lf_ConvertDims = [](const size_t ndims,
                                 const size_t *in) -> adios2::Dims {

            if (in != nullptr)
            {
                return adios2::Dims(in, in + ndims);
            }
            return adios2::Dims();
        };

        const adios2::Dims startV = lf_ConvertDims(ndims, start);
        const adios2::Dims countV = lf_ConvertDims(ndims, count);

        adios2::core::Stream &streamCpp =
            *reinterpret_cast<adios2::core::Stream *>(stream);
        const adios2::Box<adios2::Dims> selection(startV, countV);

        switch (type)
        {

        case (adios2_type_string):
        {
            std::vector<std::string> dataString =
                streamCpp.Read<std::string>(name);

            if (dataString.empty())
            {
                data = nullptr;
            }
            else
            {
                const std::string firstStr = dataString.front();
                firstStr.copy(reinterpret_cast<char *>(data), firstStr.size());
            }
            break;
        }
        case (adios2_type_char):
        {
            adios2_fread_common(streamCpp, name, reinterpret_cast<char *>(data),
                                selection);
            break;
        }
        case (adios2_type_signed_char):
        {
            adios2_fread_common(streamCpp, name,
                                reinterpret_cast<signed char *>(data),
                                selection);
            break;
        }
        case (adios2_type_short):
        {
            adios2_fread_common(streamCpp, name,
                                reinterpret_cast<short *>(data), selection);
            break;
        }
        case (adios2_type_int):
        {
            adios2_fread_common(streamCpp, name, reinterpret_cast<int *>(data),
                                selection);
            break;
        }
        case (adios2_type_long_int):
        {
            adios2_fread_common(streamCpp, name,
                                reinterpret_cast<long int *>(data), selection);
            break;
        }
        case (adios2_type_long_long_int):
        {
            adios2_fread_common(streamCpp, name,
                                reinterpret_cast<long long int *>(data),
                                selection);
            break;
        }
        case (adios2_type_unsigned_char):
        {
            adios2_fread_common(streamCpp, name,
                                reinterpret_cast<unsigned char *>(data),
                                selection);
            break;
        }
        case (adios2_type_unsigned_short):
        {
            adios2_fread_common(streamCpp, name,
                                reinterpret_cast<unsigned short *>(data),
                                selection);
            break;
        }
        case (adios2_type_unsigned_int):
        {
            adios2_fread_common(streamCpp, name,
                                reinterpret_cast<unsigned int *>(data),
                                selection);
            break;
        }
        case (adios2_type_unsigned_long_int):
        {
            adios2_fread_common(streamCpp, name,
                                reinterpret_cast<unsigned long int *>(data),
                                selection);
            break;
        }
        case (adios2_type_unsigned_long_long_int):
        {
            adios2_fread_common(
                streamCpp, name,
                reinterpret_cast<unsigned long long int *>(data), selection);
            break;
        }
        case (adios2_type_float):
        {
            adios2_fread_common(streamCpp, name,
                                reinterpret_cast<float *>(data), selection);
            break;
        }
        case (adios2_type_double):
        {
            adios2_fread_common(streamCpp, name,
                                reinterpret_cast<double *>(data), selection);
            break;
        }
        case (adios2_type_float_complex):
        {
            adios2_fread_common(streamCpp, name,
                                reinterpret_cast<std::complex<float> *>(data),
                                selection);
            break;
        }
        case (adios2_type_double_complex):
        {
            adios2_fread_common(streamCpp, name,
                                reinterpret_cast<std::complex<double> *>(data),
                                selection);
            break;
        }
        case (adios2_type_int8_t):
        {
            adios2_fread_common(streamCpp, name,
                                reinterpret_cast<int8_t *>(data), selection);
            break;
        }
        case (adios2_type_int16_t):
        {
            adios2_fread_common(streamCpp, name,
                                reinterpret_cast<int16_t *>(data), selection);
            break;
        }
        case (adios2_type_int32_t):
        {
            adios2_fread_common(streamCpp, name,
                                reinterpret_cast<int32_t *>(data), selection);
            break;
        }
        case (adios2_type_int64_t):
        {
            adios2_fread_common(streamCpp, name,
                                reinterpret_cast<int64_t *>(data), selection);
            break;
        }
        case (adios2_type_uint8_t):
        {
            adios2_fread_common(streamCpp, name,
                                reinterpret_cast<uint8_t *>(data), selection);
            break;
        }
        case (adios2_type_uint16_t):
        {
            adios2_fread_common(streamCpp, name,
                                reinterpret_cast<uint16_t *>(data), selection);
            break;
        }
        case (adios2_type_uint32_t):
        {
            adios2_fread_common(streamCpp, name,
                                reinterpret_cast<uint32_t *>(data), selection);
            break;
        }
        case (adios2_type_uint64_t):
        {
            adios2_fread_common(streamCpp, name,
                                reinterpret_cast<uint64_t *>(data), selection);
            break;
        }
        default:
        {
            throw std::invalid_argument(
                "ERROR: type not yet supported for variable " +
                std::string(name) + ", in call to adios2_fread\n");
        }
        }
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_fread"));
    }
}

adios2_error adios2_fread_steps(adios2_FILE *stream, const char *name,
                                const adios2_type type, void *data,
                                const size_t ndims, const size_t *start,
                                const size_t *count, const size_t step_start,
                                const size_t step_count)
{
    try
    {
        adios2::helper::CheckForNullptr(
            stream, "null adios2_FILE for variable " + std::string(name) +
                        ", in call to adios2_fread_steps\n");

        if (start == nullptr && count != nullptr)
        {
            throw std::invalid_argument(
                "ERROR: when reading variable " + std::string(name) +
                " start is NULL, in call to adios2_fread_steps\n");
        }
        else if (start != nullptr && count == nullptr)
        {
            throw std::invalid_argument(
                "ERROR: when reading variable " + std::string(name) +
                " count is NULL, in call to adios2_fread_steps\n");
        }

        auto lf_ConvertDims = [](const size_t ndims,
                                 const size_t *in) -> adios2::Dims {

            if (in != nullptr)
            {
                return adios2::Dims(in, in + ndims);
            }
            return adios2::Dims();
        };

        const adios2::Dims startV = lf_ConvertDims(ndims, start);
        const adios2::Dims countV = lf_ConvertDims(ndims, count);

        const bool stepOnly =
            (start == nullptr && count == nullptr && step_count == 1) ? true
                                                                      : false;
        adios2::core::Stream &streamCpp =
            *reinterpret_cast<adios2::core::Stream *>(stream);
        const adios2::Box<adios2::Dims> selection(startV, countV);
        const adios2::Box<size_t> stepSelection(step_start, step_count);

        switch (type)
        {

        case (adios2_type_string):
        {
            if (!stepOnly)
            {
                throw std::invalid_argument(
                    "ERROR: when reading variable " + std::string(name) +
                    " no selection is allowed (turn to NULL)");
            }

            std::string dataString;
            streamCpp.Read(name, &dataString,
                           adios2::Box<size_t>(step_start, 1));

            if (dataString.empty())
            {
                data = nullptr;
            }
            else
            {
                strcpy(reinterpret_cast<char *>(data), dataString.c_str());
            }
            break;
        }
        case (adios2_type_char):
        {
            adios2_fread_steps_common(streamCpp, name,
                                      reinterpret_cast<char *>(data), selection,
                                      stepSelection, stepOnly);
            break;
        }
        case (adios2_type_double):
        {
            adios2_fread_steps_common(streamCpp, name,
                                      reinterpret_cast<double *>(data),
                                      selection, stepSelection, stepOnly);
            break;
        }
        case (adios2_type_double_complex):
        {
            adios2_fread_steps_common(
                streamCpp, name, reinterpret_cast<std::complex<double> *>(data),
                selection, stepSelection, stepOnly);
            break;
        }
        case (adios2_type_float):
        {
            adios2_fread_steps_common(streamCpp, name,
                                      reinterpret_cast<float *>(data),
                                      selection, stepSelection, stepOnly);
            break;
        }
        case (adios2_type_float_complex):
        {
            adios2_fread_steps_common(
                streamCpp, name, reinterpret_cast<std::complex<float> *>(data),
                selection, stepSelection, stepOnly);
            break;
        }
        case (adios2_type_int):
        {
            adios2_fread_steps_common(streamCpp, name,
                                      reinterpret_cast<int *>(data), selection,
                                      stepSelection, stepOnly);
            break;
        }
        case (adios2_type_int16_t):
        {
            adios2_fread_steps_common(streamCpp, name,
                                      reinterpret_cast<int16_t *>(data),
                                      selection, stepSelection, stepOnly);
            break;
        }
        case (adios2_type_int32_t):
        {
            adios2_fread_steps_common(streamCpp, name,
                                      reinterpret_cast<int32_t *>(data),
                                      selection, stepSelection, stepOnly);
            break;
        }
        case (adios2_type_int64_t):
        {
            adios2_fread_steps_common(streamCpp, name,
                                      reinterpret_cast<int64_t *>(data),
                                      selection, stepSelection, stepOnly);
            break;
        }
        case (adios2_type_int8_t):
        {
            adios2_fread_steps_common(streamCpp, name,
                                      reinterpret_cast<int8_t *>(data),
                                      selection, stepSelection, stepOnly);
            break;
        }
        case (adios2_type_long_int):
        {
            adios2_fread_steps_common(streamCpp, name,
                                      reinterpret_cast<long int *>(data),
                                      selection, stepSelection, stepOnly);
            break;
        }
        case (adios2_type_long_long_int):
        {
            adios2_fread_steps_common(streamCpp, name,
                                      reinterpret_cast<long long int *>(data),
                                      selection, stepSelection, stepOnly);
            break;
        }
        case (adios2_type_short):
        {
            adios2_fread_steps_common(streamCpp, name,
                                      reinterpret_cast<short *>(data),
                                      selection, stepSelection, stepOnly);
            break;
        }
        case (adios2_type_signed_char):
        {
            adios2_fread_steps_common(streamCpp, name,
                                      reinterpret_cast<signed char *>(data),
                                      selection, stepSelection, stepOnly);
            break;
        }
        case (adios2_type_uint16_t):
        {
            adios2_fread_steps_common(streamCpp, name,
                                      reinterpret_cast<uint16_t *>(data),
                                      selection, stepSelection, stepOnly);
            break;
        }
        case (adios2_type_uint32_t):
        {
            adios2_fread_steps_common(streamCpp, name,
                                      reinterpret_cast<uint32_t *>(data),
                                      selection, stepSelection, stepOnly);
            break;
        }
        case (adios2_type_uint64_t):
        {
            adios2_fread_steps_common(streamCpp, name,
                                      reinterpret_cast<uint64_t *>(data),
                                      selection, stepSelection, stepOnly);
            break;
        }
        case (adios2_type_uint8_t):
        {
            adios2_fread_steps_common(streamCpp, name,
                                      reinterpret_cast<uint8_t *>(data),
                                      selection, stepSelection, stepOnly);
            break;
        }
        case (adios2_type_unsigned_char):
        {
            adios2_fread_steps_common(streamCpp, name,
                                      reinterpret_cast<unsigned char *>(data),
                                      selection, stepSelection, stepOnly);
            break;
        }
        case (adios2_type_unsigned_int):
        {
            adios2_fread_steps_common(streamCpp, name,
                                      reinterpret_cast<unsigned int *>(data),
                                      selection, stepSelection, stepOnly);
            break;
        }
        case (adios2_type_unsigned_long_int):
        {
            adios2_fread_steps_common(
                streamCpp, name, reinterpret_cast<unsigned long int *>(data),
                selection, stepSelection, stepOnly);
            break;
        }
        case (adios2_type_unsigned_long_long_int):
        {
            adios2_fread_steps_common(
                streamCpp, name,
                reinterpret_cast<unsigned long long int *>(data), selection,
                stepSelection, stepOnly);
            break;
        }
        case (adios2_type_unsigned_short):
        {
            adios2_fread_steps_common(streamCpp, name,
                                      reinterpret_cast<unsigned short *>(data),
                                      selection, stepSelection, stepOnly);
            break;
        }
        default:
        {
            throw std::invalid_argument(
                "ERROR: type not yet supported for variable " +
                std::string(name) + ", in call to adios2_fread\n");
        }
        }
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_fread_steps"));
    }
}

adios2_error adios2_fclose(adios2_FILE *stream)
{
    try
    {
        adios2::helper::CheckForNullptr(
            stream, "null adios2_FILE, in call to adios2_fclose\n");

        adios2::core::Stream &streamCpp =
            *reinterpret_cast<adios2::core::Stream *>(stream);
        streamCpp.Close();
        delete reinterpret_cast<adios2::core::Stream *>(stream);
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_fclose"));
    }
}

#ifdef __cplusplus
} // end extern C
#endif
