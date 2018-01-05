/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_c_bpstream.cpp
 *
 *  Created on: Jan 8, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adios2_c_FILE.h"

#include <adios2/highlevelapi/fstream/Stream.h>
#include <cstring> //strcpy

#include "adios2/ADIOSMPI.h"
#include "adios2/adios2_c_glue.h"
#include "adios2/adios2_c_io.h"
#include "adios2/core/Variable.h"

adios2_FILE *adios2_fopen(const char *name, const adios2_mode mode,
                          MPI_Comm comm)
{
    return adios2_fopen_glue(name, mode, comm, "C");
}

adios2_FILE *adios2_fopen_nompi(const char *name, const adios2_mode mode)
{
    return adios2_fopen_glue(name, mode, MPI_COMM_SELF, "C");
}

adios2_FILE *adios2_fopen_config(const char *name, const adios2_mode mode,
                                 MPI_Comm comm, const char *config_file,
                                 const char *io_in_config_file)
{
    return adios2_fopen_config_glue(name, mode, comm, config_file,
                                    io_in_config_file, "C");
}

adios2_FILE *adios2_fopen_config_nompi(const char *name, const adios2_mode mode,
                                       const char *config_file,
                                       const char *io_in_config_file)
{
    return adios2_fopen_config_glue(name, mode, MPI_COMM_SELF, config_file,
                                    io_in_config_file, "C");
}

void adios2_fwrite(adios2_FILE *stream, const char *name,
                   const adios2_type type, const void *data, const size_t ndims,
                   const size_t *shape, const size_t *start,
                   const size_t *count, const int end_step)
{
    std::vector<size_t> shapeV, startV, countV;

    if (shape != nullptr)
    {
        shapeV.assign(shape, shape + ndims);
    }

    if (start != nullptr)
    {
        startV.assign(start, start + ndims);
    }

    if (count != nullptr)
    {
        countV.assign(count, count + ndims);
    }

    adios2::Stream &streamCpp = *reinterpret_cast<adios2::Stream *>(stream);
    const bool endStep = (end_step == adios2_advance_step) ? true : false;

    switch (type)
    {

    case (adios2_type_string):
    {
        const std::string stringCpp(reinterpret_cast<const char *>(data));
        streamCpp.Write(name, stringCpp, endStep);
        break;
    }
    case (adios2_type_char):
    {
        streamCpp.Write(name, reinterpret_cast<const char *>(data), shapeV,
                        startV, countV, endStep);

        break;
    }
    case (adios2_type_signed_char):
    {
        streamCpp.Write(name, reinterpret_cast<const signed char *>(data),
                        shapeV, startV, countV, endStep);
        break;
    }
    case (adios2_type_short):
    {
        streamCpp.Write(name, reinterpret_cast<const short *>(data), shapeV,
                        startV, countV, endStep);
        break;
    }
    case (adios2_type_int):
    {
        streamCpp.Write(name, reinterpret_cast<const int *>(data), shapeV,
                        startV, countV, endStep);
        break;
    }
    case (adios2_type_long_int):
    {
        streamCpp.Write(name, reinterpret_cast<const long int *>(data), shapeV,
                        startV, countV, endStep);
        break;
    }
    case (adios2_type_long_long_int):
    {
        streamCpp.Write(name, reinterpret_cast<const long long int *>(data),
                        shapeV, startV, countV, endStep);
        break;
    }
    case (adios2_type_unsigned_char):
    {
        streamCpp.Write(name, reinterpret_cast<const unsigned char *>(data),
                        shapeV, startV, countV, endStep);
        break;
    }
    case (adios2_type_unsigned_short):
    {
        streamCpp.Write(name, reinterpret_cast<const unsigned short *>(data),
                        shapeV, startV, countV, endStep);
        break;
    }
    case (adios2_type_unsigned_int):
    {
        streamCpp.Write(name, reinterpret_cast<const unsigned int *>(data),
                        shapeV, startV, countV, endStep);
        break;
    }
    case (adios2_type_unsigned_long_int):
    {
        streamCpp.Write(name, reinterpret_cast<const unsigned long int *>(data),
                        shapeV, startV, countV, endStep);
        break;
    }
    case (adios2_type_unsigned_long_long_int):
    {
        streamCpp.Write(name,
                        reinterpret_cast<const unsigned long long int *>(data),
                        shapeV, startV, countV, endStep);
        break;
    }
    case (adios2_type_float):
    {
        streamCpp.Write(name, reinterpret_cast<const float *>(data), shapeV,
                        startV, countV, endStep);
        break;
    }
    case (adios2_type_double):
    {
        streamCpp.Write(name, reinterpret_cast<const double *>(data), shapeV,
                        startV, countV, endStep);
        break;
    }
    case (adios2_type_float_complex):
    {
        streamCpp.Write(name,
                        reinterpret_cast<const std::complex<float> *>(data),
                        shapeV, startV, countV, endStep);
        break;
    }
    case (adios2_type_double_complex):
    {
        streamCpp.Write(name,
                        reinterpret_cast<const std::complex<double> *>(data),
                        shapeV, startV, countV, endStep);
        break;
    }
    case (adios2_type_int8_t):
    {
        streamCpp.Write(name, reinterpret_cast<const int8_t *>(data), shapeV,
                        startV, countV, endStep);
        break;
    }
    case (adios2_type_int16_t):
    {
        streamCpp.Write(name, reinterpret_cast<const int16_t *>(data), shapeV,
                        startV, countV, endStep);
        break;
    }
    case (adios2_type_int32_t):
    {
        streamCpp.Write(name, reinterpret_cast<const int32_t *>(data), shapeV,
                        startV, countV, endStep);
        break;
    }
    case (adios2_type_int64_t):
    {
        streamCpp.Write(name, reinterpret_cast<const int64_t *>(data), shapeV,
                        startV, countV, endStep);
        break;
    }
    case (adios2_type_uint8_t):
    {
        streamCpp.Write(name, reinterpret_cast<const uint8_t *>(data), shapeV,
                        startV, countV, endStep);
        break;
    }
    case (adios2_type_uint16_t):
    {
        streamCpp.Write(name, reinterpret_cast<const uint16_t *>(data), shapeV,
                        startV, countV, endStep);
        break;
    }
    case (adios2_type_uint32_t):
    {
        streamCpp.Write(name, reinterpret_cast<const uint32_t *>(data), shapeV,
                        startV, countV, endStep);
        break;
    }
    case (adios2_type_uint64_t):
    {
        streamCpp.Write(name, reinterpret_cast<const uint64_t *>(data), shapeV,
                        startV, countV, endStep);
        break;
    }
    default:
    {
        throw std::invalid_argument(
            "ERROR: type not yet supported for variable " + std::string(name) +
            ", in call to adios2_swrite\n");
    }
    }
}

namespace
{
template <class T>
void adios2_fread_common(adios2::Stream &stream, const std::string &name,
                         T *data, const adios2::Box<adios2::Dims> &selection,
                         const bool endStep)
{
    if (selection.first.empty() && selection.second.empty())
    {
        stream.Read(name, data, endStep);
        return;
    }

    stream.Read(name, data, selection, endStep);
}

template <class T>
void adios2_fread_steps_common(adios2::Stream &stream, const std::string &name,
                               T *data,
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

void adios2_fread(adios2_FILE *stream, const char *name, const adios2_type type,
                  void *data, const size_t ndims, const size_t *selection_start,
                  const size_t *selection_count, const int end_step)
{
    if (stream == nullptr)
    {
        throw std::invalid_argument("ERROR: null adios2_FILE for variable " +
                                    std::string(name) +
                                    ", in call to adios2_fread\n");
    }

    std::vector<size_t> startV, countV;

    if (selection_start != nullptr)
    {
        startV.assign(selection_start, selection_start + ndims);
    }

    if (selection_count != nullptr)
    {
        countV.assign(selection_count, selection_count + ndims);
    }

    adios2::Stream &streamCpp = *reinterpret_cast<adios2::Stream *>(stream);
    const adios2::Box<adios2::Dims> selection(startV, countV);
    const bool endStep = (end_step == adios2_advance_step) ? true : false;

    switch (type)
    {

    case (adios2_type_string):
    {
        std::vector<std::string> dataString =
            streamCpp.Read<std::string>(name, endStep);

        if (dataString.empty())
        {
            data = nullptr;
        }
        else
        {
            strcpy(reinterpret_cast<char *>(data), dataString.front().c_str());
        }
        break;
    }
    case (adios2_type_char):
    {
        adios2_fread_common(streamCpp, name, reinterpret_cast<char *>(data),
                            selection, endStep);
        break;
    }
    case (adios2_type_signed_char):
    {
        adios2_fread_common(streamCpp, name,
                            reinterpret_cast<signed char *>(data), selection,
                            endStep);
        break;
    }
    case (adios2_type_short):
    {
        adios2_fread_common(streamCpp, name, reinterpret_cast<short *>(data),
                            selection, endStep);
        break;
    }
    case (adios2_type_int):
    {
        adios2_fread_common(streamCpp, name, reinterpret_cast<int *>(data),
                            selection, endStep);
        break;
    }
    case (adios2_type_long_int):
    {
        adios2_fread_common(streamCpp, name, reinterpret_cast<long int *>(data),
                            selection, endStep);
        break;
    }
    case (adios2_type_long_long_int):
    {
        adios2_fread_common(streamCpp, name,
                            reinterpret_cast<long long int *>(data), selection,
                            endStep);
        break;
    }
    case (adios2_type_unsigned_char):
    {
        adios2_fread_common(streamCpp, name,
                            reinterpret_cast<unsigned char *>(data), selection,
                            endStep);
        break;
    }
    case (adios2_type_unsigned_short):
    {
        adios2_fread_common(streamCpp, name,
                            reinterpret_cast<unsigned short *>(data), selection,
                            endStep);
        break;
    }
    case (adios2_type_unsigned_int):
    {
        adios2_fread_common(streamCpp, name,
                            reinterpret_cast<unsigned int *>(data), selection,
                            endStep);
        break;
    }
    case (adios2_type_unsigned_long_int):
    {
        adios2_fread_common(streamCpp, name,
                            reinterpret_cast<unsigned long int *>(data),
                            selection, endStep);
        break;
    }
    case (adios2_type_unsigned_long_long_int):
    {
        adios2_fread_common(streamCpp, name,
                            reinterpret_cast<unsigned long long int *>(data),
                            selection, endStep);
        break;
    }
    case (adios2_type_float):
    {
        adios2_fread_common(streamCpp, name, reinterpret_cast<float *>(data),
                            selection, endStep);
        break;
    }
    case (adios2_type_double):
    {
        adios2_fread_common(streamCpp, name, reinterpret_cast<double *>(data),
                            selection, endStep);
        break;
    }
    case (adios2_type_float_complex):
    {
        adios2_fread_common(streamCpp, name,
                            reinterpret_cast<std::complex<float> *>(data),
                            selection, endStep);
        break;
    }
    case (adios2_type_double_complex):
    {
        adios2_fread_common(streamCpp, name,
                            reinterpret_cast<std::complex<double> *>(data),
                            selection, endStep);
        break;
    }
    case (adios2_type_int8_t):
    {
        adios2_fread_common(streamCpp, name, reinterpret_cast<int8_t *>(data),
                            selection, endStep);
        break;
    }
    case (adios2_type_int16_t):
    {
        adios2_fread_common(streamCpp, name, reinterpret_cast<int16_t *>(data),
                            selection, endStep);
        break;
    }
    case (adios2_type_int32_t):
    {
        adios2_fread_common(streamCpp, name, reinterpret_cast<int32_t *>(data),
                            selection, endStep);
        break;
    }
    case (adios2_type_int64_t):
    {
        adios2_fread_common(streamCpp, name, reinterpret_cast<int64_t *>(data),
                            selection, endStep);
        break;
    }
    case (adios2_type_uint8_t):
    {
        adios2_fread_common(streamCpp, name, reinterpret_cast<uint8_t *>(data),
                            selection, endStep);
        break;
    }
    case (adios2_type_uint16_t):
    {
        adios2_fread_common(streamCpp, name, reinterpret_cast<uint16_t *>(data),
                            selection, endStep);
        break;
    }
    case (adios2_type_uint32_t):
    {
        adios2_fread_common(streamCpp, name, reinterpret_cast<uint32_t *>(data),
                            selection, endStep);
        break;
    }
    case (adios2_type_uint64_t):
    {
        adios2_fread_common(streamCpp, name, reinterpret_cast<uint64_t *>(data),
                            selection, endStep);
        break;
    }
    default:
    {
        throw std::invalid_argument(
            "ERROR: type not yet supported for variable " + std::string(name) +
            ", in call to adios2_fread\n");
    }
    }
}

void adios2_fread_steps(adios2_FILE *stream, const char *name,
                        const adios2_type type, void *data, const size_t ndims,
                        const size_t *selection_start,
                        const size_t *selection_count,
                        const size_t step_selection_start,
                        const size_t step_selection_count)
{
    if (stream == nullptr)
    {
        throw std::invalid_argument("ERROR: null adios2_FILE for variable " +
                                    std::string(name) +
                                    ", in call to adios2_fread_steps\n");
    }

    std::vector<size_t> startV, countV;

    if (selection_start != nullptr)
    {
        startV.assign(selection_start, selection_start + ndims);
    }

    if (selection_count != nullptr)
    {
        countV.assign(selection_count, selection_count + ndims);
    }

    if (selection_start == nullptr && selection_count != nullptr)
    {
        throw std::invalid_argument(
            "ERROR: when reading variable " + std::string(name) +
            " selection_start is NULL, in call to fread\n");
    }
    else if (selection_start != nullptr && selection_count == nullptr)
    {
        throw std::invalid_argument(
            "ERROR: when reading variable " + std::string(name) +
            " selection_count is NULL, in call to fread\n");
    }

    const bool stepOnly =
        (selection_start == nullptr && selection_count == nullptr &&
         step_selection_count == 1)
            ? true
            : false;
    adios2::Stream &streamCpp = *reinterpret_cast<adios2::Stream *>(stream);
    const adios2::Box<adios2::Dims> selection(startV, countV);
    const adios2::Box<size_t> stepSelection(step_selection_start,
                                            step_selection_count);

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
                       adios2::Box<size_t>(step_selection_start, 1));

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
                                  reinterpret_cast<double *>(data), selection,
                                  stepSelection, stepOnly);
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
                                  reinterpret_cast<float *>(data), selection,
                                  stepSelection, stepOnly);
        break;
    }
    case (adios2_type_float_complex):
    {
        adios2_fread_steps_common(streamCpp, name,
                                  reinterpret_cast<std::complex<float> *>(data),
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
                                  reinterpret_cast<int16_t *>(data), selection,
                                  stepSelection, stepOnly);
        break;
    }
    case (adios2_type_int32_t):
    {
        adios2_fread_steps_common(streamCpp, name,
                                  reinterpret_cast<int32_t *>(data), selection,
                                  stepSelection, stepOnly);
        break;
    }
    case (adios2_type_int64_t):
    {
        adios2_fread_steps_common(streamCpp, name,
                                  reinterpret_cast<int64_t *>(data), selection,
                                  stepSelection, stepOnly);
        break;
    }
    case (adios2_type_int8_t):
    {
        adios2_fread_steps_common(streamCpp, name,
                                  reinterpret_cast<int8_t *>(data), selection,
                                  stepSelection, stepOnly);
        break;
    }
    case (adios2_type_long_int):
    {
        adios2_fread_steps_common(streamCpp, name,
                                  reinterpret_cast<long int *>(data), selection,
                                  stepSelection, stepOnly);
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
                                  reinterpret_cast<short *>(data), selection,
                                  stepSelection, stepOnly);
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
                                  reinterpret_cast<uint16_t *>(data), selection,
                                  stepSelection, stepOnly);
        break;
    }
    case (adios2_type_uint32_t):
    {
        adios2_fread_steps_common(streamCpp, name,
                                  reinterpret_cast<uint32_t *>(data), selection,
                                  stepSelection, stepOnly);
        break;
    }
    case (adios2_type_uint64_t):
    {
        adios2_fread_steps_common(streamCpp, name,
                                  reinterpret_cast<uint64_t *>(data), selection,
                                  stepSelection, stepOnly);
        break;
    }
    case (adios2_type_uint8_t):
    {
        adios2_fread_steps_common(streamCpp, name,
                                  reinterpret_cast<uint8_t *>(data), selection,
                                  stepSelection, stepOnly);
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
        adios2_fread_steps_common(streamCpp, name,
                                  reinterpret_cast<unsigned long int *>(data),
                                  selection, stepSelection, stepOnly);
        break;
    }
    case (adios2_type_unsigned_long_long_int):
    {
        adios2_fread_steps_common(
            streamCpp, name, reinterpret_cast<unsigned long long int *>(data),
            selection, stepSelection, stepOnly);
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
            "ERROR: type not yet supported for variable " + std::string(name) +
            ", in call to adios2_fread\n");
    }
    }
}

void adios2_fclose(adios2_FILE *stream)
{
    adios2::Stream &streamCpp = *reinterpret_cast<adios2::Stream *>(stream);
    streamCpp.Close();
    delete (reinterpret_cast<adios2::Stream *>(stream));
    stream = nullptr;
}
