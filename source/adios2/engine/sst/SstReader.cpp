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
#include "adios2/toolkit/format/bp3/BP3.h"
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
    if (!m_Input)
    {
        throw std::invalid_argument("ERROR: SstReader did not find active "
                                    "Writer contact info in file \"" +
                                    m_Name + SST_POSTFIX +
                                    "\".  Non-current SST contact file?" +
                                    m_EndMessage);
    }

    // Maybe need other writer-side params in the future, but for now only
    // marshal methods.
    SstReaderGetParams(m_Input, &m_WriterFFSmarshal, &m_WriterBPmarshal);

    auto varFFSCallback = [](void *reader, const char *variableName,
                             const char *type, void *data) {
        std::string Type(type);
        class SstReader::SstReader *Reader =
            reinterpret_cast<class SstReader::SstReader *>(reader);
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

    auto arrayFFSCallback = [](void *reader, const char *variableName,
                               const char *type, int DimCount, size_t *Shape,
                               size_t *Start, size_t *Count) {
        std::vector<size_t> VecShape;
        std::vector<size_t> VecStart;
        std::vector<size_t> VecCount;
        std::string Type(type);
        class SstReader::SstReader *Reader =
            reinterpret_cast<class SstReader::SstReader *>(reader);
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

    SstReaderInitFFSCallback(m_Input, this, varFFSCallback, arrayFFSCallback);

    delete[] cstr;
}

StepStatus SstReader::BeginStep(StepMode mode, const float timeout_sec)
{

    SstStatusValue result;
    m_IO.RemoveAllVariables();
    m_IO.RemoveAllAttributes();
    result = SstAdvanceStep(m_Input, (int)mode, timeout_sec);
    if (result == SstEndOfStream)
    {
        return StepStatus::EndOfStream;
    }
    if (result != SstSuccess)
    {
        return StepStatus::OtherError;
    }

    if (m_WriterBPmarshal)
    {
        m_CurrentStepMetaData = SstGetCurMetadata(m_Input);
        // At begin step, you get metadata from the writers.  You need to
        // use this for two things: First, you need to create the
        // appropriate variables on the reader side to represent what has
        // been written.  Second, you must keep the metadata around (or a
        // summary structure that represents its contents), so that you can
        // look at it in DoGets* and issue the appropriate
        // ReadRemoteMemory() requests to the writers.  This is because the
        // full data isn't here yet.  We don't know what we'll need on this
        // reader, and we don't want to send *all* data from *all* writers
        // here because that's not scalable.  So, we'll fetch what we need.
        // This is one of the core ideas of SST.

        //   Further clarification:
        //   m_CurrentStepMetaData is a struct like this:

        //     struct _SstFullMetadata
        //     {
        //       int WriterCohortSize;
        //       struct _SstData **WriterMetadata;
        //       void **DP_TimestepInfo;
        //     };

        //   WriterMetadata has an element for each Writer rank
        //   (WriterCohortSize).  WriterMetadata[i] is a pointer to (a copy
        //   of) the SstData block of Metadata that was passed to
        //   SstProvideTimestep by Writer rank i.  SST has simply gathered
        //   them and provided them to each reader.  The DP_TimestepInfo is
        //   for the DataPlane, and DP_TimestepInfo[i] should be passed as
        //   the DP_TimestepInfo parameter when a SstReadRemoteMemory call
        //   is made requesting rank i.  (This may contain MR keys, or
        //   anything else that the Data Plane needs for efficient RDMA on
        //   whatever transport it is using.  But it is opaque to the Engine
        //   (and to the control plane).)

        m_BP3Deserializer = new format::BP3Deserializer(m_MPIComm, m_DebugMode);
        m_BP3Deserializer->InitParameters(m_IO.m_Parameters);

        struct _SstData **d = m_CurrentStepMetaData->WriterMetadata;

        m_BP3Deserializer->m_Metadata.Resize(
            (*m_CurrentStepMetaData->WriterMetadata)->DataSize,
            "in SST Streaming Listener");

        std::memcpy(m_BP3Deserializer->m_Metadata.m_Buffer.data(),
                    (*m_CurrentStepMetaData->WriterMetadata)->block,
                    (*m_CurrentStepMetaData->WriterMetadata)->DataSize);

        m_IO.RemoveAllVariables();
        m_IO.RemoveAllAttributes();
        m_BP3Deserializer->ParseMetadata(m_BP3Deserializer->m_Metadata, m_IO);

        const auto variablesInfo = m_IO.GetAvailableVariables();
        for (const auto &variableInfoPair : variablesInfo)
        {
            std::string var = variableInfoPair.first;
            std::cout << "---- " << var << std::endl;
            for (const auto &parameter : variableInfoPair.second)
            {
                std::cout << "---- key = " << parameter.first
                          << ", value = " << parameter.second << std::endl;
            }
        }
        std::map<std::string, SubFileInfoMap> variablesSubFileInfo =
            m_BP3Deserializer->PerformGetsVariablesSubFileInfo(m_IO);
        std::cout << variablesSubFileInfo.size() << std::endl;

        for (const auto &variableNamePair : variablesSubFileInfo)
        {
            std::cout << ": " << variableNamePair.first << std::endl;
            const std::string variableName(variableNamePair.first);
            for (const auto &subFileIndexPair : variableNamePair.second)
            {
                const size_t subFileIndex = subFileIndexPair.first;
                std::cout << "subFileIndex: " << subFileIndex << std::endl;
            }
        }
    }
    else if (m_WriterFFSmarshal)
    {
        // For FFS-based marshaling, SstAdvanceStep takes care of installing
        // the metadata, creating variables using the varFFScallback and
        // arrayFFScallback, so there's nothing to be done now.  This
        // comment is just for clarification.
    }
    else
    {
        // unknown marshaling method, shouldn't happen
    }

    return StepStatus::OK;
}

size_t SstReader::CurrentStep() const { return SstCurrentStep(m_Input); }

void SstReader::EndStep()
{
    std::cout << "0111\n";
    if (m_FFSmarshal)
    {
        // this does all the deferred gets and fills in the variable array data
        SstFFSPerformGets(m_Input);
        std::cout << "1111\n";
    }
    else if (m_BPmarshal)
    {
        std::cout << "2111\n";
        //  I'm assuming that the DoGet calls below have been constructing
        //  some kind of data structure that indicates what data this reader
        //  needs from different writers, what read requests it needs to
        //  make, where to put the data when it arrives, etc.  Therefore the
        //  pseudocode here looks like:
        //         IssueReadRequests()   I.E. do calls to SstReadRemoteMemory()
        //         as necessary to get the data coming these are asynchronous
        //	   so that they can happen in parallel.
        //         WaitForReadRequests()   Wait for each of these read requests
        //         to complete.  See the SstWaitForCompletion function.
        //         FillReadRequests()    Given the data that was just read,
        //         place it appropriately into the waiting vars.
        //	   ClearReadRequests()  Clean up as necessary
        //
        delete m_BP3Deserializer;
    }
    else
    {
        // unknown marshaling method, shouldn't happen
    }
    std::cout << "3111\n";
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

    auto lf_SetStringParameter = [&](const std::string key, char *&parameter) {

        auto itKey = m_IO.m_Parameters.find(key);
        if (itKey != m_IO.m_Parameters.end())
        {
            parameter = strdup(itKey->second.c_str());
            return true;
        }
        return false;
    };

    auto lf_SetRegMethodParameter = [&](const std::string key,
                                        size_t &parameter) {
        auto itKey = m_IO.m_Parameters.find(key);
        if (itKey != m_IO.m_Parameters.end())
        {
            std::string method = itKey->second;
            std::transform(method.begin(), method.end(), method.begin(),
                           ::tolower);
            if (method == "file")
            {
                parameter = SstRegisterFile;
            }
            else if (method == "screen")
            {
                parameter = SstRegisterScreen;
            }
            else if (method == "cloud")
            {
                parameter = SstRegisterCloud;
                throw std::invalid_argument("ERROR: Sst RegistrationMethod "
                                            "\"cloud\" not yet implemented" +
                                            m_EndMessage);
            }
            else
            {
                throw std::invalid_argument(
                    "ERROR: Unknown Sst RegistrationMethod parameter \"" +
                    method + "\"" + m_EndMessage);
            }
            return true;
        }
        return false;
    };

#define get_params(Param, Type, Typedecl, Default)                             \
    lf_Set##Type##Parameter(#Param, m_##Param);
    SST_FOREACH_PARAMETER_TYPE_4ARGS(get_params);
#undef get_params
#define set_params(Param, Type, Typedecl, Default) Params.Param = m_##Param;
    SST_FOREACH_PARAMETER_TYPE_4ARGS(set_params);
#undef set_params
}

#define declare_gets(T)                                                        \
    void SstReader::DoGetSync(Variable<T> &variable, T *data)                  \
    {                                                                          \
        if (m_WriterFFSmarshal)                                                \
        {                                                                      \
            SstFFSGetDeferred(                                                 \
                m_Input, (void *)&variable, variable.m_Name.c_str(),           \
                variable.m_Start.size(), variable.m_Start.data(),              \
                variable.m_Count.data(), data);                                \
            SstFFSPerformGets(m_Input);                                        \
        }                                                                      \
        if (m_WriterBPmarshal)                                                 \
        {                                                                      \
            /*  DoGetSync() is going to have terrible performance 'cause */    \
            /*  it's a bad idea in an SST-like environment.  But do */         \
            /*  whatever you do forDoGetDeferred() and then PerformGets() */   \
            DoGetDeferred(variable, data);                                     \
            PerformGets();                                                     \
        }                                                                      \
    }                                                                          \
    void SstReader::DoGetDeferred(Variable<T> &variable, T *data)              \
    {                                                                          \
        if (m_WriterFFSmarshal)                                                \
        {                                                                      \
            SstFFSGetDeferred(                                                 \
                m_Input, (void *)&variable, variable.m_Name.c_str(),           \
                variable.m_Start.size(), variable.m_Start.data(),              \
                variable.m_Count.data(), data);                                \
        }                                                                      \
        if (m_WriterBPmarshal)                                                 \
        {                                                                      \
            std::cout << "c---\n";                                             \
            /*  Look at the data requested and examine the metadata to see  */ \
            /*  what writer has what you need.  Build up a set of read */      \
            /*  requests (maybe just get all the data from every writer */     \
            /*  that has *something* you need).  You'll use this in EndStep,*/ \
            /*  when you have to get all the array data and put it where  */   \
            /*  it's supposed to go. */                                        \
            std::map<std::string, SubFileInfoMap> variablesSubFileInfo =       \
                m_BP3Deserializer->PerformGetsVariablesSubFileInfo(m_IO);      \
            for (const auto &variableNamePair : variablesSubFileInfo)          \
            {                                                                  \
                std::cout << ": " << variableNamePair.first << std::endl;      \
                const std::string variableName(variableNamePair.first);        \
                for (const auto &subFileIndexPair : variableNamePair.second)   \
                {                                                              \
                    const size_t subFileIndex = subFileIndexPair.first;        \
                    std::cout << "subFileIndex: " << subFileIndex              \
                              << std::endl;                                    \
                }                                                              \
            }                                                                  \
        }                                                                      \
    }                                                                          \
    void SstReader::DoGetDeferred(Variable<T> &variable, T &data)              \
    {                                                                          \
        if (m_WriterFFSmarshal)                                                \
        {                                                                      \
            SstFFSGetDeferred(                                                 \
                m_Input, (void *)&variable, variable.m_Name.c_str(),           \
                variable.m_Start.size(), variable.m_Start.data(),              \
                variable.m_Count.data(), &data);                               \
        }                                                                      \
        if (m_WriterBPmarshal)                                                 \
        {                                                                      \
            std::cout << "d---\n";                                             \
            /* See the routine above.*/                                        \
        }                                                                      \
    }

ADIOS2_FOREACH_TYPE_1ARG(declare_gets)
#undef declare_gets

void SstReader::PerformGets() { SstFFSPerformGets(m_Input); }

void SstReader::DoClose(const int transportIndex) { SstReaderClose(m_Input); }

} // end namespace adios2
