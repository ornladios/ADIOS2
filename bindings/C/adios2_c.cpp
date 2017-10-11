/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_c.h : ADIOS2 C bindings definitions
 *
 *  Created on: Mar 13, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adios2_c.h"

#include <string>
#include <vector>

#include <adios2.h>
#include <adios2/ADIOSMPI.h>
#include <adios2/ADIOSMacros.h>
#include <adios2/helper/adiosFunctions.h>

adios2_ADIOS *adios2_init_config(const char *config_file, MPI_Comm mpi_comm,
                                 const adios2_debug_mode debug_mode)
{
    const bool debugBool = (debug_mode == adios2_debug_mode_on) ? true : false;
    adios2_ADIOS *adios = reinterpret_cast<adios2_ADIOS *>(
        new adios2::ADIOS(config_file, mpi_comm, debugBool));
    adios2_set_host_language(adios, "C");

    return adios;
}

adios2_ADIOS *adios2_init(MPI_Comm mpi_comm, const adios2_debug_mode debug_mode)
{
    return adios2_init_config("", mpi_comm, debug_mode);
}

adios2_ADIOS *adios2_init_config_nompi(const char *config_file,
                                       const adios2_debug_mode debug_mode)
{
    return adios2_init_config(config_file, MPI_COMM_SELF, debug_mode);
}

adios2_ADIOS *adios2_init_nompi(const adios2_debug_mode debug_mode)
{
    return adios2_init_config("", MPI_COMM_SELF, debug_mode);
}

void adios2_set_host_language(adios2_ADIOS *adios, const char *host_language)
{
    adios2::ADIOS &adiosCpp = *reinterpret_cast<adios2::ADIOS *>(adios);
    adiosCpp.m_HostLanguage = std::string(host_language);
}

adios2_IO *adios2_declare_io(adios2_ADIOS *adios, const char *ioName)
{
    adios2_IO *io = reinterpret_cast<adios2_IO *>(
        &reinterpret_cast<adios2::ADIOS *>(adios)->DeclareIO(ioName));
    return io;
}

adios2_Variable *
adios2_define_variable(adios2_IO *io, const char *name, const adios2_type type,
                       const size_t ndims, const size_t *shape,
                       const size_t *start, const size_t *count,
                       const adios2_constant_dims constant_dims)
{
    const bool constantSizeBool =
        (constant_dims == adios2_constant_dims_true) ? true : false;

    std::vector<size_t> shapeV, startV, countV;

    if (shape != NULL)
    {
        shapeV.assign(shape, shape + ndims);
    }

    if (start != NULL)
    {
        startV.assign(start, start + ndims);
    }

    if (count != NULL)
    {
        countV.assign(count, count + ndims);
    }

    adios2::IO &ioCpp = *reinterpret_cast<adios2::IO *>(io);
    adios2::VariableBase *variable = nullptr;

    switch (type)
    {
    case (adios2_type_string):;
        break;

    case (adios2_type_char):
        variable =
            dynamic_cast<adios2::Variable<char> *>(&ioCpp.DefineVariable<char>(
                name, shapeV, startV, countV, constantSizeBool));
        break;

    case (adios2_type_signed_char):
        variable = dynamic_cast<adios2::Variable<signed char> *>(
            &ioCpp.DefineVariable<char>(name, shapeV, startV, countV,
                                        constantSizeBool));
        break;

    case (adios2_type_short):
        variable = dynamic_cast<adios2::Variable<short> *>(
            &ioCpp.DefineVariable<short>(name, shapeV, startV, countV,
                                         constantSizeBool));
        break;

    case (adios2_type_int):
        variable =
            dynamic_cast<adios2::Variable<int> *>(&ioCpp.DefineVariable<int>(
                name, shapeV, startV, countV, constantSizeBool));
        break;

    case (adios2_type_long_int):
        variable = dynamic_cast<adios2::Variable<long int> *>(
            &ioCpp.DefineVariable<long int>(name, shapeV, startV, countV,
                                            constantSizeBool));
        break;

    case (adios2_type_long_long_int):
        variable = dynamic_cast<adios2::Variable<long long int> *>(
            &ioCpp.DefineVariable<long long int>(name, shapeV, startV, countV,
                                                 constantSizeBool));
        break;

    case (adios2_type_unsigned_char):
        variable = dynamic_cast<adios2::Variable<unsigned char> *>(
            &ioCpp.DefineVariable<unsigned char>(name, shapeV, startV, countV,
                                                 constantSizeBool));
        break;

    case (adios2_type_unsigned_short):
        variable = dynamic_cast<adios2::Variable<unsigned short> *>(
            &ioCpp.DefineVariable<unsigned short>(name, shapeV, startV, countV,
                                                  constantSizeBool));
        break;

    case (adios2_type_unsigned_int):
        variable = dynamic_cast<adios2::Variable<unsigned int> *>(
            &ioCpp.DefineVariable<unsigned int>(name, shapeV, startV, countV,
                                                constantSizeBool));
        break;
    case (adios2_type_unsigned_long_int):
        variable = dynamic_cast<adios2::Variable<unsigned long int> *>(
            &ioCpp.DefineVariable<unsigned long int>(name, shapeV, startV,
                                                     countV, constantSizeBool));
        break;

    case (adios2_type_unsigned_long_long_int):
        variable = dynamic_cast<adios2::Variable<unsigned long long int> *>(
            &ioCpp.DefineVariable<unsigned long long int>(
                name, shapeV, startV, countV, constantSizeBool));
        break;

    case (adios2_type_float):
        variable = dynamic_cast<adios2::Variable<float> *>(
            &ioCpp.DefineVariable<float>(name, shapeV, startV, countV,
                                         constantSizeBool));
        break;

    case (adios2_type_double):
        variable = dynamic_cast<adios2::Variable<double> *>(
            &ioCpp.DefineVariable<double>(name, shapeV, startV, countV,
                                          constantSizeBool));
        break;

    case (adios2_type_float_complex):
        variable = dynamic_cast<adios2::Variable<std::complex<float>> *>(
            &ioCpp.DefineVariable<std::complex<float>>(
                name, shapeV, startV, countV, constantSizeBool));
        break;

    case (adios2_type_double_complex):
        variable = dynamic_cast<adios2::Variable<std::complex<double>> *>(
            &ioCpp.DefineVariable<std::complex<double>>(
                name, shapeV, startV, countV, constantSizeBool));
        break;

    case (adios2_type_int8_t):

        variable = dynamic_cast<adios2::Variable<int8_t> *>(
            &ioCpp.DefineVariable<int8_t>(name, shapeV, startV, countV,
                                          constantSizeBool));
        break;

    case (adios2_type_int16_t):
        variable = dynamic_cast<adios2::Variable<int16_t> *>(
            &ioCpp.DefineVariable<int16_t>(name, shapeV, startV, countV,
                                           constantSizeBool));
        break;

    case (adios2_type_int32_t):
        variable = dynamic_cast<adios2::Variable<int32_t> *>(
            &ioCpp.DefineVariable<int32_t>(name, shapeV, startV, countV,
                                           constantSizeBool));
        break;

    case (adios2_type_int64_t):
        variable = dynamic_cast<adios2::Variable<int64_t> *>(
            &ioCpp.DefineVariable<int64_t>(name, shapeV, startV, countV,
                                           constantSizeBool));
        break;

    case (adios2_type_uint8_t):

        variable = dynamic_cast<adios2::Variable<uint8_t> *>(
            &ioCpp.DefineVariable<uint8_t>(name, shapeV, startV, countV,
                                           constantSizeBool));
        break;

    case (adios2_type_uint16_t):
        variable = dynamic_cast<adios2::Variable<uint16_t> *>(
            &ioCpp.DefineVariable<uint16_t>(name, shapeV, startV, countV,
                                            constantSizeBool));
        break;

    case (adios2_type_uint32_t):
        variable = dynamic_cast<adios2::Variable<uint32_t> *>(
            &ioCpp.DefineVariable<uint32_t>(name, shapeV, startV, countV,
                                            constantSizeBool));
        break;

    case (adios2_type_uint64_t):
        variable = dynamic_cast<adios2::Variable<uint64_t> *>(
            &ioCpp.DefineVariable<uint64_t>(name, shapeV, startV, countV,
                                            constantSizeBool));
        break;
    }

    return reinterpret_cast<adios2_Variable *>(variable);
}

adios2_Variable *adios2_inquire_variable(adios2_IO *io, const char *name)
{
    adios2::IO &ioCpp = *reinterpret_cast<adios2::IO *>(io);
    const auto &dataMap = ioCpp.GetVariablesDataMap();

    auto itVariable = dataMap.find(name);
    if (itVariable == dataMap.end()) // not found
    {
        return nullptr;
    }

    const std::string type(itVariable->second.first);
    adios2::VariableBase *variable = nullptr;

    if (type == "compound")
    {
        // not supported
    }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::GetType<T>())                                     \
    {                                                                          \
        variable = ioCpp.InquireVariable<T>(name);                             \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

    return reinterpret_cast<adios2_Variable *>(variable);
}

void adios2_set_engine(adios2_IO *io, const char *engine_type)
{
    reinterpret_cast<adios2::IO *>(io)->SetEngine(engine_type);
}

void adios2_set_param(adios2_IO *io, const char *key, const char *value)
{
    reinterpret_cast<adios2::IO *>(io)->SetParameter(key, value);
}

unsigned int adios2_add_transport(adios2_IO *io, const char *transport_type)
{
    return reinterpret_cast<adios2::IO *>(io)->AddTransport(transport_type);
}

void adios2_set_transport_param(adios2_IO *io,
                                const unsigned int transport_index,
                                const char *key, const char *value)
{
    reinterpret_cast<adios2::IO *>(io)->SetTransportParameter(transport_index,
                                                              key, value);
}

adios2_Engine *adios2_open(adios2_IO *io, const char *name,
                           const adios2_mode open_mode)
{
    auto &ioCpp = *reinterpret_cast<adios2::IO *>(io);
    return adios2_open_new_comm(io, name, open_mode, ioCpp.m_MPIComm);
}

adios2_Engine *adios2_open_new_comm(adios2_IO *io, const char *name,
                                    const adios2_mode open_mode,
                                    MPI_Comm mpi_comm)
{
    auto &ioCpp = *reinterpret_cast<adios2::IO *>(io);
    adios2::Engine *engine = nullptr;

    switch (open_mode)
    {

    case adios2_mode_write:
        engine = &ioCpp.Open(name, adios2::Mode::Write, mpi_comm);
        break;

    case adios2_mode_read:
        engine = &ioCpp.Open(name, adios2::Mode::Read, mpi_comm);
        break;

    case adios2_mode_append:
        engine = &ioCpp.Open(name, adios2::Mode::Append, mpi_comm);
        break;

    case adios2_mode_undefined:

        break;
    }

    return reinterpret_cast<adios2_Engine *>(engine);
}

void adios2_acquire_step(adios2_Engine *engine)
{
    auto &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);
    engineCpp.BeginStep();
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

void adios2_release_step(adios2_Engine *engine)
{
    auto &engineCpp = *reinterpret_cast<adios2::Engine *>(engine);
    engineCpp.EndStep();
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

void adios2_finalize(adios2_ADIOS *adios)
{
    delete reinterpret_cast<adios2::ADIOS *>(adios);
}
