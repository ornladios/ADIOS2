/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_c_io.cpp
 *
 *  Created on: Nov 8, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adios2_c_io.h"

#include <vector>

#include "adios2/ADIOSMPI.h"
#include "adios2/core/IO.h"
#include "adios2/helper/adiosFunctions.h" //GetType<T>

#ifdef __cplusplus
extern "C" {
#endif

adios2_variable *
adios2_define_variable(adios2_io *io, const char *name, const adios2_type type,
                       const size_t ndims, const size_t *shape,
                       const size_t *start, const size_t *count,
                       const adios2_constant_dims constant_dims)
{
    adios2::helper::CheckForNullptr(
        io, "for adios2_io, in call to adios2_define_variable");

    const bool constantSizeBool =
        (constant_dims == adios2_constant_dims_true) ? true : false;

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

    adios2::core::IO &ioCpp = *reinterpret_cast<adios2::core::IO *>(io);
    adios2::core::VariableBase *variable = nullptr;

    switch (type)
    {

    case (adios2_type_string):
    {
        variable = &ioCpp.DefineVariable<std::string>(name, shapeV, startV,
                                                      countV, constantSizeBool);
        break;
    }
    case (adios2_type_char):
    {
        variable = &ioCpp.DefineVariable<char>(name, shapeV, startV, countV,
                                               constantSizeBool);
        break;
    }
    case (adios2_type_signed_char):
    {
        variable = &ioCpp.DefineVariable<signed char>(name, shapeV, startV,
                                                      countV, constantSizeBool);
        break;
    }
    case (adios2_type_short):
    {
        variable = &ioCpp.DefineVariable<short>(name, shapeV, startV, countV,
                                                constantSizeBool);
        break;
    }
    case (adios2_type_int):
    {
        variable = &ioCpp.DefineVariable<int>(name, shapeV, startV, countV,
                                              constantSizeBool);
        break;
    }
    case (adios2_type_long_int):
    {
        variable = &ioCpp.DefineVariable<long int>(name, shapeV, startV, countV,
                                                   constantSizeBool);
        break;
    }
    case (adios2_type_long_long_int):
    {
        variable = &ioCpp.DefineVariable<long long int>(
            name, shapeV, startV, countV, constantSizeBool);
        break;
    }
    case (adios2_type_unsigned_char):
    {
        variable = &ioCpp.DefineVariable<unsigned char>(
            name, shapeV, startV, countV, constantSizeBool);
        break;
    }
    case (adios2_type_unsigned_short):
    {
        variable = &ioCpp.DefineVariable<unsigned short>(
            name, shapeV, startV, countV, constantSizeBool);
        break;
    }
    case (adios2_type_unsigned_int):
    {
        variable = &ioCpp.DefineVariable<unsigned int>(
            name, shapeV, startV, countV, constantSizeBool);
        break;
    }
    case (adios2_type_unsigned_long_int):
    {
        variable = &ioCpp.DefineVariable<unsigned long int>(
            name, shapeV, startV, countV, constantSizeBool);
        break;
    }
    case (adios2_type_unsigned_long_long_int):
    {
        variable = &ioCpp.DefineVariable<unsigned long long int>(
            name, shapeV, startV, countV, constantSizeBool);
        break;
    }
    case (adios2_type_float):
    {
        variable = &ioCpp.DefineVariable<float>(name, shapeV, startV, countV,
                                                constantSizeBool);
        break;
    }
    case (adios2_type_double):
    {
        variable = &ioCpp.DefineVariable<double>(name, shapeV, startV, countV,
                                                 constantSizeBool);
        break;
    }
    case (adios2_type_float_complex):
    {
        variable = &ioCpp.DefineVariable<std::complex<float>>(
            name, shapeV, startV, countV, constantSizeBool);
        break;
    }
    case (adios2_type_double_complex):
    {
        variable = &ioCpp.DefineVariable<std::complex<double>>(
            name, shapeV, startV, countV, constantSizeBool);
        break;
    }
    case (adios2_type_int8_t):
    {

        variable = &ioCpp.DefineVariable<int8_t>(name, shapeV, startV, countV,
                                                 constantSizeBool);
        break;
    }
    case (adios2_type_int16_t):
    {
        variable = &ioCpp.DefineVariable<int16_t>(name, shapeV, startV, countV,
                                                  constantSizeBool);
        break;
    }
    case (adios2_type_int32_t):
    {
        variable = &ioCpp.DefineVariable<int32_t>(name, shapeV, startV, countV,
                                                  constantSizeBool);
        break;
    }
    case (adios2_type_int64_t):
    {
        variable = &ioCpp.DefineVariable<int64_t>(name, shapeV, startV, countV,
                                                  constantSizeBool);
        break;
    }
    case (adios2_type_uint8_t):
    {

        variable = &ioCpp.DefineVariable<uint8_t>(name, shapeV, startV, countV,
                                                  constantSizeBool);
        break;
    }
    case (adios2_type_uint16_t):
    {
        variable = &ioCpp.DefineVariable<uint16_t>(name, shapeV, startV, countV,
                                                   constantSizeBool);
        break;
    }
    case (adios2_type_uint32_t):
    {
        variable = &ioCpp.DefineVariable<uint32_t>(name, shapeV, startV, countV,
                                                   constantSizeBool);
        break;
    }
    case (adios2_type_uint64_t):
    {
        variable = &ioCpp.DefineVariable<uint64_t>(name, shapeV, startV, countV,
                                                   constantSizeBool);
        break;
    }
    default:
    {
        return nullptr;
    }
    }

    return reinterpret_cast<adios2_variable *>(variable);
}

adios2_variable *adios2_inquire_variable(adios2_io *io, const char *name)
{
    adios2::helper::CheckForNullptr(
        io, "for adios2_io, in call to adios2_inquire_variable");

    adios2::core::IO &ioCpp = *reinterpret_cast<adios2::core::IO *>(io);
    const auto &dataMap = ioCpp.GetVariablesDataMap();

    auto itVariable = dataMap.find(name);
    if (itVariable == dataMap.end()) // not found
    {
        return nullptr;
    }

    const std::string type(itVariable->second.first);
    adios2::core::VariableBase *variable = nullptr;

    if (type == "compound")
    {
        // not supported
    }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::helper::GetType<T>())                             \
    {                                                                          \
        variable = ioCpp.InquireVariable<T>(name);                             \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

    return reinterpret_cast<adios2_variable *>(variable);
}

int adios2_remove_variable(adios2_io *io, const char *name)
{
    adios2::helper::CheckForNullptr(
        io, "for adios2_io, in call to adios2_remove_variable");

    return (reinterpret_cast<adios2::core::IO *>(io)->RemoveVariable(name)) ? 1
                                                                            : 0;
}

void adios2_remove_all_variables(adios2_io *io)
{
    adios2::helper::CheckForNullptr(
        io, "for adios2_io, in call to adios2_remove_all_variables");

    reinterpret_cast<adios2::core::IO *>(io)->RemoveAllVariables();
}

adios2_attribute *adios2_define_attribute(adios2_io *io, const char *name,
                                          const adios2_type type,
                                          const void *data,
                                          const size_t elements)
{
    return adios2_define_variable_attribute(io, name, type, data, elements, "",
                                            "");
}

adios2_attribute *adios2_define_variable_attribute(
    adios2_io *io, const char *name, const adios2_type type, const void *data,
    const size_t elements, const char *variable_name, const char *separator)
{

    adios2::helper::CheckForNullptr(
        io, "for adios2_io, in call to adios2_define_attribute");

    adios2::helper::CheckForNullptr(
        data, "for const void* data, in call to adios2_define_attribute");

    adios2::core::IO &ioCpp = *reinterpret_cast<adios2::core::IO *>(io);
    adios2::core::AttributeBase *attribute = nullptr;

    switch (type)
    {

    case (adios2_type_string):
    {
        // relying on null terminated character for the size
        const std::string singleString(reinterpret_cast<const char *>(data));
        attribute = &ioCpp.DefineAttribute<std::string>(
            name, singleString, variable_name, separator);
        break;
    }
    case (adios2_type_string_array):
    {
        std::vector<std::string> arrayStrings(elements);
        const char *char2D = reinterpret_cast<const char *>(data);
        for (auto i = 0; i < elements; ++i)
        {
            arrayStrings[i] =
                std::string(&char2D[i * adios2_string_array_element_max_size]);
        }

        attribute = &ioCpp.DefineAttribute<std::string>(
            name, arrayStrings.data(), arrayStrings.size(), variable_name,
            separator);
        break;
    }
    case (adios2_type_char):
    {
        attribute = &ioCpp.DefineAttribute<char>(
            name, reinterpret_cast<const char *>(data), elements, variable_name,
            separator);
        break;
    }
    case (adios2_type_signed_char):
    {
        attribute = &ioCpp.DefineAttribute<signed char>(
            name, reinterpret_cast<const signed char *>(data), elements,
            variable_name, separator);
        break;
    }
    case (adios2_type_short):
    {
        attribute = &ioCpp.DefineAttribute<short>(
            name, reinterpret_cast<const short *>(data), elements,
            variable_name, separator);
        break;
    }
    case (adios2_type_int):
    {
        attribute = &ioCpp.DefineAttribute<int>(
            name, reinterpret_cast<const int *>(data), elements, variable_name,
            separator);
        break;
    }
    case (adios2_type_long_int):
    {
        attribute = &ioCpp.DefineAttribute<long int>(
            name, reinterpret_cast<const long int *>(data), elements,
            variable_name, separator);
        break;
    }
    case (adios2_type_long_long_int):
    {
        attribute = &ioCpp.DefineAttribute<long long int>(
            name, reinterpret_cast<const long long int *>(data), elements,
            variable_name, separator);
        break;
    }
    case (adios2_type_unsigned_char):
    {
        attribute = &ioCpp.DefineAttribute<unsigned char>(
            name, reinterpret_cast<const unsigned char *>(data), elements,
            variable_name, separator);
        break;
    }
    case (adios2_type_unsigned_short):
    {
        attribute = &ioCpp.DefineAttribute<unsigned short>(
            name, reinterpret_cast<const unsigned short *>(data), elements,
            variable_name, separator);
        break;
    }
    case (adios2_type_unsigned_int):
    {
        attribute = &ioCpp.DefineAttribute<unsigned int>(
            name, reinterpret_cast<const unsigned int *>(data), elements,
            variable_name, separator);
        break;
    }
    case (adios2_type_unsigned_long_int):
    {
        attribute = &ioCpp.DefineAttribute<unsigned long int>(
            name, reinterpret_cast<const unsigned long int *>(data), elements,
            variable_name, separator);
        break;
    }
    case (adios2_type_unsigned_long_long_int):
    {
        attribute = &ioCpp.DefineAttribute<unsigned long long int>(
            name, reinterpret_cast<const unsigned long long int *>(data),
            elements, variable_name, separator);
        break;
    }
    case (adios2_type_float):
    {
        attribute = &ioCpp.DefineAttribute<float>(
            name, reinterpret_cast<const float *>(data), elements,
            variable_name, separator);
        break;
    }
    case (adios2_type_double):
    {
        attribute = &ioCpp.DefineAttribute<double>(
            name, reinterpret_cast<const double *>(data), elements,
            variable_name, separator);
        break;
    }
    case (adios2_type_int8_t):
    {

        attribute = &ioCpp.DefineAttribute<int8_t>(
            name, reinterpret_cast<const int8_t *>(data), elements,
            variable_name, separator);
        break;
    }
    case (adios2_type_int16_t):
    {
        attribute = &ioCpp.DefineAttribute<int16_t>(
            name, reinterpret_cast<const int16_t *>(data), elements,
            variable_name, separator);
        break;
    }
    case (adios2_type_int32_t):
    {
        attribute = &ioCpp.DefineAttribute<int32_t>(
            name, reinterpret_cast<const int32_t *>(data), elements,
            variable_name, separator);
        break;
    }
    case (adios2_type_int64_t):
    {
        attribute = &ioCpp.DefineAttribute<int64_t>(
            name, reinterpret_cast<const int64_t *>(data), elements,
            variable_name, separator);
        break;
    }
    case (adios2_type_uint8_t):
    {
        attribute = &ioCpp.DefineAttribute<uint8_t>(
            name, reinterpret_cast<const uint8_t *>(data), elements,
            variable_name, separator);
        break;
    }
    case (adios2_type_uint16_t):
    {
        attribute = &ioCpp.DefineAttribute<uint16_t>(
            name, reinterpret_cast<const uint16_t *>(data), elements,
            variable_name, separator);
        break;
    }
    case (adios2_type_uint32_t):
    {
        attribute = &ioCpp.DefineAttribute<uint32_t>(
            name, reinterpret_cast<const uint32_t *>(data), elements,
            variable_name, separator);
        break;
    }
    case (adios2_type_uint64_t):
    {
        attribute = &ioCpp.DefineAttribute<uint64_t>(
            name, reinterpret_cast<const uint64_t *>(data), elements,
            variable_name, separator);
        break;
    }
    case (adios2_type_unknown):
    {
        break;
    }
    default:
    {
        return nullptr;
    }
    }

    return reinterpret_cast<adios2_attribute *>(attribute);
}

adios2_attribute *adios2_inquire_attribute(adios2_io *io, const char *name)
{
    adios2::helper::CheckForNullptr(
        io, "for adios2_io, in call to adios2_inquire_attribute");

    adios2::core::IO &ioCpp = *reinterpret_cast<adios2::core::IO *>(io);
    const auto &dataMap = ioCpp.GetAttributesDataMap();

    auto itAttribute = dataMap.find(name);
    if (itAttribute == dataMap.end()) // not found
    {
        return nullptr;
    }

    const std::string type(itAttribute->second.first);
    adios2::core::AttributeBase *attribute = nullptr;

    if (type == "compound")
    {
        // not supported
    }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::helper::GetType<T>())                             \
    {                                                                          \
        attribute = ioCpp.InquireAttribute<T>(name);                           \
    }
    ADIOS2_FOREACH_ATTRIBUTE_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

    return reinterpret_cast<adios2_attribute *>(attribute);
}

adios2_attribute *adios2_inquire_variable_attribute(adios2_io *io,
                                                    const char *name,
                                                    const char *variable_name,
                                                    const char *separator)
{
    const std::string globalName =
        std::string(variable_name) + std::string(separator) + std::string(name);
    return adios2_inquire_attribute(io, globalName.c_str());
}

int adios2_remove_attribute(adios2_io *io, const char *name)
{
    adios2::helper::CheckForNullptr(
        io, "for adios2_io, in call to adios2_remove_attribute");
    return (reinterpret_cast<adios2::core::IO *>(io)->RemoveAttribute(name))
               ? 1
               : 0;
}

void adios2_remove_all_attributes(adios2_io *io)
{
    adios2::helper::CheckForNullptr(
        io, "for adios2_io, in call to adios2_remove_all_attributes");
    reinterpret_cast<adios2::core::IO *>(io)->RemoveAllAttributes();
}

void adios2_set_engine(adios2_io *io, const char *engine_type)
{
    adios2::helper::CheckForNullptr(
        io, "for adios2_io, in call to adios2_set_engine");
    reinterpret_cast<adios2::core::IO *>(io)->SetEngine(engine_type);
}

void adios2_set_parameter(adios2_io *io, const char *key, const char *value)
{
    adios2::helper::CheckForNullptr(
        io, "for adios2_io, in call to adios2_set_parameter");
    reinterpret_cast<adios2::core::IO *>(io)->SetParameter(key, value);
}

size_t adios2_add_transport(adios2_io *io, const char *transport_type)
{
    adios2::helper::CheckForNullptr(
        io, "for adios2_io, in call to adios2_add_transport");
    return reinterpret_cast<adios2::core::IO *>(io)->AddTransport(
        transport_type);
}

void adios2_set_transport_parameter(adios2_io *io, const size_t transport_index,
                                    const char *key, const char *value)
{
    adios2::helper::CheckForNullptr(
        io, "for adios2_io, in call to adios2_set_transport_parameter");
    reinterpret_cast<adios2::core::IO *>(io)->SetTransportParameter(
        transport_index, key, value);
}

adios2_engine *adios2_open(adios2_io *io, const char *name,
                           const adios2_mode mode)
{
    adios2::helper::CheckForNullptr(io,
                                    "for adios2_io, in call to adios2_open");

    auto &ioCpp = *reinterpret_cast<adios2::core::IO *>(io);
    return adios2_open_new_comm(io, name, mode, ioCpp.m_MPIComm);
}

adios2_engine *adios2_open_new_comm(adios2_io *io, const char *name,
                                    const adios2_mode mode, MPI_Comm mpi_comm)
{
    adios2::helper::CheckForNullptr(
        io, "for adios2_io, in call to adios2_open_new_comm");
    auto &ioCpp = *reinterpret_cast<adios2::core::IO *>(io);
    adios2::core::Engine *engine = nullptr;

    switch (mode)
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

    default:
        return nullptr;
    }

    return reinterpret_cast<adios2_engine *>(engine);
}

void adios2_flush_all_engines(adios2_io *io)
{
    adios2::helper::CheckForNullptr(
        io, "for adios2_io, in call to adios2_flush_all_engines");
    reinterpret_cast<adios2::core::IO *>(io)->FlushAll();
}

void adios2_lock_definitions(adios2_io *io)
{
    adios2::helper::CheckForNullptr(
        io, "for adios2_io, in call to adios2_lock_definitions");
    reinterpret_cast<adios2::core::IO *>(io)->LockDefinitions();
}

const char *adios2_io_engine_type(const adios2_io *io, size_t *length)
{
    adios2::helper::CheckForNullptr(
        io, "for const adios2_io, in call to adios2_io_engine_type");

    const adios2::core::IO *ioCpp =
        reinterpret_cast<const adios2::core::IO *>(io);

    if (length != nullptr)
    {
        *length = ioCpp->m_EngineType.size();
    }
    return ioCpp->m_EngineType.c_str();
}

#ifdef __cplusplus
} // end extern C
#endif
