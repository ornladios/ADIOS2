/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SstReader.cpp
 *
 *  Created on: Aug 17, 2017
 *      Author: Greg Eisenhauer
 */

#include "adios2/helper/adiosFunctions.h"
#include <cstring>
#include <string>

#include "SstReader.h"

namespace adios2
{

SstReader::SstReader(IO &io, const std::string &name, const Mode mode,
                     MPI_Comm mpiComm)
: Engine("SstReader", io, name, mode, mpiComm)
{
    SstStream output;
    char *cstr = new char[name.length() + 1];
    std::strcpy(cstr, name.c_str());

    Init();

    m_Input = SstReaderOpen(cstr, &Params, mpiComm);
    auto varCallback = [](void *reader, const char *variableName,
                          const char *type, void *data) {
        std::string Type(type);
        typename SstReader::SstReader *Reader =
            reinterpret_cast<typename SstReader::SstReader *>(reader);
        if (Type == "compound")
        {
            return (void *)NULL;
        }
#define declare_type(T)                                                        \
    else if (Type == GetType<T>())                                             \
    {                                                                          \
        Variable<T> *variable =                                                \
            &(Reader->m_IO.DefineVariable<T>(variableName));                   \
        variable->SetData((T *)data);                                          \
        variable->m_AvailableStepsCount = 1;                                   \
        return (void *)variable;                                               \
    }

        ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

        return (void *)NULL;
    };

    auto arrayCallback = [](void *reader, const char *variableName,
                            const char *type, int DimCount, size_t *Shape,
                            size_t *Start, size_t *Count) {
        std::vector<size_t> VecShape;
        std::vector<size_t> VecStart;
        std::vector<size_t> VecCount;
        std::string Type(type);
        typename SstReader::SstReader *Reader =
            reinterpret_cast<typename SstReader::SstReader *>(reader);
        /*
         * setup shape of array variable as global (I.E. Count == Shape,
         * Start == 0)
         */
        for (int i = 0; i < DimCount; i++)
        {
            VecShape.push_back(Shape[i]);
            VecStart.push_back(0);
            VecCount.push_back(Shape[i]);
        }
        if (Type == "compound")
        {
            return (void *)NULL;
        }
#define declare_type(T)                                                        \
    else if (Type == GetType<T>())                                             \
    {                                                                          \
        Variable<T> *variable = &(Reader->m_IO.DefineVariable<T>(              \
            variableName, VecShape, VecStart, VecCount));                      \
        variable->m_AvailableStepsCount = 1;                                   \
        return (void *)variable;                                               \
    }
        ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
        return (void *)NULL;
    };

    SstReaderInitCallback(m_Input, this, varCallback, arrayCallback);

    delete[] cstr;
}

StepStatus SstReader::BeginStep(StepMode mode, const float timeout_sec)
{
    SstStatusValue result;
    m_IO.RemoveAllVariables();
    m_IO.RemoveAllAttributes();
    result = SstAdvanceStep(m_Input, (int)mode, timeout_sec);
    if (result == SstSuccess)
    {
        return StepStatus::OK;
    }
    else if (result == SstEndOfStream)
    {
        return StepStatus::EndOfStream;
    }
    else
    {
        return StepStatus::OtherError;
    }
}

size_t SstReader::CurrentStep() const { return SstCurrentStep(m_Input); }

void SstReader::EndStep()
{
    SstPerformGets(m_Input);
    SstReleaseStep(m_Input);
}

// PRIVATE
void SstReader::Init()
{
    auto lf_SetBoolParameter = [&](const std::string key, int &parameter) {

        auto itKey = m_IO.m_Parameters.find(key);
        if (itKey != m_IO.m_Parameters.end())
        {
            if (itKey->second == "yes" || itKey->second == "true")
            {
                parameter = 1;
            }
            else if (itKey->second == "no" || itKey->second == "false")
            {
                parameter = 0;
            }
        }
    };
    auto lf_SetIntParameter = [&](const std::string key, int &parameter) {

        auto itKey = m_IO.m_Parameters.find(key);
        if (itKey != m_IO.m_Parameters.end())
        {
            parameter = std::stoi(itKey->second);
            return true;
        }
        return false;
    };

#define get_params(Param, Type, Typedecl, Default)                             \
    lf_Set##Type##Parameter("##Param##", m_##Param);
    SST_FOREACH_PARAMETER_TYPE_4ARGS(get_params);
#undef get_params
#define set_params(Param, Type, Typedecl, Default) Params.Param = m_##Param;
    SST_FOREACH_PARAMETER_TYPE_4ARGS(set_params);
#undef set_params
}

#define declare_type(T)                                                        \
    void SstReader::DoGetSync(Variable<T> &variable, T *data)                  \
    {                                                                          \
        SstGetDeferred(m_Input, (void *)&variable, variable.m_Name.c_str(),    \
                       variable.m_Start.size(), variable.m_Start.data(),       \
                       variable.m_Count.data(), data);                         \
        SstPerformGets(m_Input);                                               \
    }                                                                          \
    void SstReader::DoGetDeferred(Variable<T> &variable, T *data)              \
    {                                                                          \
        SstGetDeferred(m_Input, (void *)&variable, variable.m_Name.c_str(),    \
                       variable.m_Start.size(), variable.m_Start.data(),       \
                       variable.m_Count.data(), data);                         \
    }                                                                          \
    void SstReader::DoGetDeferred(Variable<T> &variable, T &data)              \
    {                                                                          \
        SstGetDeferred(m_Input, (void *)&variable, variable.m_Name.c_str(),    \
                       variable.m_Start.size(), variable.m_Start.data(),       \
                       variable.m_Count.data(), &data);                        \
    }
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void SstReader::PerformGets() { SstPerformGets(m_Input); }

void SstReader::DoClose(const int transportIndex) { SstReaderClose(m_Input); }

} // end namespace adios2
