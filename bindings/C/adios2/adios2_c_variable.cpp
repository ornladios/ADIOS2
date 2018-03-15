/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_c_variable.cpp
 *
 *  Created on: Nov 10, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adios2_c_variable.h"

#include "adios2/core/Variable.h"
#include "adios2/helper/adiosFunctions.h"

const std::map<std::string, std::vector<adios2_type>> adios2_types_map = {
    {"char", {adios2_type_char}},
    {"int", {adios2_type_int32_t, adios2_type_int}},
    {"float", {adios2_type_float}},
    {"double", {adios2_type_double}},
    {"float complex", {adios2_type_float_complex}},
    {"double complex", {adios2_type_double_complex}},
    {"signed char", {adios2_type_int8_t, adios2_type_signed_char}},
    {"short", {adios2_type_int16_t, adios2_type_short}},
    {"long int", {adios2_type_int64_t, adios2_type_long_int}},
    {"long long int", {adios2_type_int64_t, adios2_type_long_long_int}},
    {"string", {adios2_type_string}},
    {"string array", {adios2_type_string_array}},
    {"unsigned char", {adios2_type_unsigned_char, adios2_type_uint8_t}},
    {"unsigned short", {adios2_type_unsigned_short, adios2_type_uint16_t}},
    {"unsigned int", {adios2_type_unsigned_int, adios2_type_uint32_t}},
    {"unsigned long int",
     {adios2_type_unsigned_long_int, adios2_type_uint64_t}},
    {"unsigned long long int",
     {adios2_type_unsigned_long_long_int, adios2_type_uint64_t}},
};

const char *adios2_variable_name(const adios2_variable *variable,
                                 size_t *length)
{
    const adios2::VariableBase *variableBase =
        reinterpret_cast<const adios2::VariableBase *>(variable);
    if (length != nullptr)
    {
        *length = variableBase->m_Name.size();
    }
    return variableBase->m_Name.c_str();
}

adios2_type adios2_variable_type(const adios2_variable *variable)
{
    const adios2::VariableBase *variableBase =
        reinterpret_cast<const adios2::VariableBase *>(variable);

    auto itType = adios2_types_map.find(variableBase->m_Type);

    if (itType == adios2_types_map.end())
    {
        return adios2_type_unknown;
    }

    return itType->second.front();
}

int adios2_variable_is_constant_dims(const adios2_variable *variable)
{
    const adios2::VariableBase *variableBase =
        reinterpret_cast<const adios2::VariableBase *>(variable);
    const int isConstantDims = (variableBase->IsConstantDims()) ? 1 : 0;
    return isConstantDims;
}

size_t adios2_variable_ndims(const adios2_variable *variable)
{
    const adios2::VariableBase *variableBase =
        reinterpret_cast<const adios2::VariableBase *>(variable);
    return variableBase->m_Shape.size();
}

const size_t *adios2_variable_shape(const adios2_variable *variable)
{
    const adios2::VariableBase *variableBase =
        reinterpret_cast<const adios2::VariableBase *>(variable);
    return variableBase->m_Shape.data();
}

const size_t *adios2_variable_start(const adios2_variable *variable)
{
    const adios2::VariableBase *variableBase =
        reinterpret_cast<const adios2::VariableBase *>(variable);
    return variableBase->m_Start.data();
}

const size_t *adios2_variable_count(const adios2_variable *variable)
{
    const adios2::VariableBase *variableBase =
        reinterpret_cast<const adios2::VariableBase *>(variable);
    return variableBase->m_Count.data();
}

size_t adios2_variable_available_steps_start(const adios2_variable *variable)
{
    const adios2::VariableBase *variableBase =
        reinterpret_cast<const adios2::VariableBase *>(variable);
    return variableBase->m_AvailableStepsStart;
}

size_t adios2_variable_available_steps_count(const adios2_variable *variable)
{
    const adios2::VariableBase *variableBase =
        reinterpret_cast<const adios2::VariableBase *>(variable);
    return variableBase->m_AvailableStepsCount;
}

void adios2_set_dimensions(adios2_variable *variable, const size_t ndims,
                           const size_t *shape, const size_t *start,
                           const size_t *count)
{
    auto lf_Assign = [](const size_t ndims, const size_t *input,
                        adios2::Dims &dims) {

        if (input != nullptr)
        {
            dims.clear();
            dims.assign(input, input + ndims);
        }
    };

    adios2::VariableBase *variableBase =
        reinterpret_cast<adios2::VariableBase *>(variable);

    lf_Assign(ndims, shape, variableBase->m_Shape);
    lf_Assign(ndims, start, variableBase->m_Start);
    lf_Assign(ndims, count, variableBase->m_Count);
    variableBase->CheckDimensions("in call to adios2_set_selection");
}

void adios2_set_shape(adios2_variable *variable, const size_t ndims,
                      const size_t *shape)
{
    adios2::VariableBase *variableBase =
        reinterpret_cast<adios2::VariableBase *>(variable);
    const adios2::Dims shapeV(shape, shape + ndims);
    variableBase->SetShape(shapeV);
}

void adios2_set_selection(adios2_variable *variable, const size_t ndims,
                          const size_t *start, const size_t *count)
{
    adios2::VariableBase *variableBase =
        reinterpret_cast<adios2::VariableBase *>(variable);

    const adios2::Dims startV(start, start + ndims);
    const adios2::Dims countV(count, count + ndims);
    variableBase->SetSelection({startV, countV});
    variableBase->CheckDimensions("in call to adios2_set_selection");
}

void adios2_set_step_selection(adios2_variable *variable,
                               const size_t step_start, const size_t step_count)
{
    adios2::VariableBase *variableBase =
        reinterpret_cast<adios2::VariableBase *>(variable);
    variableBase->SetStepSelection(adios2::Box<size_t>{step_start, step_count});
}

void *adios2_get_data(const adios2_variable *variable)
{
    const adios2::VariableBase *variableBase =
        reinterpret_cast<const adios2::VariableBase *>(variable);
    const std::string type(variableBase->m_Type);

    void *data = nullptr;
    if (type == "compound")
    {
        // not supported
    }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::GetType<T>())                                     \
    {                                                                          \
        const adios2::Variable<T> *variableCpp =                               \
            reinterpret_cast<const adios2::Variable<T> *>(variable);           \
        data = variableCpp->GetData();                                         \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

    return data;
}

void adios2_set_data(adios2_variable *variable, const void *data)
{
    adios2::VariableBase *variableBase =
        reinterpret_cast<adios2::VariableBase *>(variable);

    const std::string type(variableBase->m_Type);

    if (type == "compound")
    {
        // not supported
    }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::GetType<T>())                                     \
    {                                                                          \
        adios2::Variable<T> *variableCpp =                                     \
            reinterpret_cast<adios2::Variable<T> *>(variable);                 \
        variableCpp->SetData(reinterpret_cast<const T *>(data));               \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
}
