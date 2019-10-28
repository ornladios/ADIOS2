/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SstReader.cpp
 *
 *  Created on: Aug 17, 2017
 *      Author: Greg Eisenhauer
 */

#include "SstReader.h"
#include "SstParamParser.h"
#include "SstReader.tcc"

#include <cstring>
#include <string>

#include "adios2/helper/adiosComm.h"
#include "adios2/helper/adiosFunctions.h"
#include "adios2/toolkit/profiling/taustubs/tautimer.hpp"

#ifdef ADIOS2_HAVE_MPI
#include "adios2/helper/adiosCommMPI.h"
#else
#include "adios2/toolkit/sst/mpiwrap.h"
#endif

namespace adios2
{
namespace core
{
namespace engine
{

SstReader::SstReader(IO &io, const std::string &name, const Mode mode,
                     helper::Comm comm)
: Engine("SstReader", io, name, mode, std::move(comm))
{
    char *cstr = new char[name.length() + 1];
    std::strcpy(cstr, name.c_str());

    Init();

    m_Input = SstReaderOpen(cstr, &Params,
#ifdef ADIOS2_HAVE_MPI
                            CommAsMPI(m_Comm)
#else
                            MPI_COMM_NULL
#endif
    );
    if (!m_Input)
    {
        throw std::runtime_error(
            "ERROR: SstReader did not find active "
            "Writer contact info in file \"" +
            m_Name + SST_POSTFIX +
            "\".  Timeout or non-current SST contact file?" + m_EndMessage);
    }

    // Maybe need other writer-side params in the future, but for now only
    // marshal method.
    SstReaderGetParams(m_Input, &m_WriterMarshalMethod);

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
    else if (Type == helper::GetType<T>())                                     \
    {                                                                          \
        Variable<T> *variable =                                                \
            &(Reader->m_IO.DefineVariable<T>(variableName));                   \
        variable->SetData((T *)data);                                          \
        variable->m_AvailableStepsCount = 1;                                   \
        return (void *)variable;                                               \
    }

        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

        return (void *)NULL;
    };

    auto attrFFSCallback = [](void *reader, const char *attrName,
                              const char *type, void *data) {
        class SstReader::SstReader *Reader =
            reinterpret_cast<class SstReader::SstReader *>(reader);
        if (attrName == NULL)
        {
            // if attrName is NULL, prepare for attr reinstallation
            Reader->m_IO.RemoveAllAttributes();
            return;
        }
        std::string Type(type);
        try
        {
            if (Type == "compound")
            {
                return;
            }
            else if (Type == helper::GetType<std::string>())
            {
                Reader->m_IO.DefineAttribute<std::string>(attrName,
                                                          *(char **)data);
            }
#define declare_type(T)                                                        \
    else if (Type == helper::GetType<T>())                                     \
    {                                                                          \
        Reader->m_IO.DefineAttribute<T>(attrName, *(T *)data);                 \
    }

            ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type
            else
            {
                std::cout << "Loading attribute matched no type " << Type
                          << std::endl;
            }
        }
        catch (...)
        {
            //            std::cout << "Load failed" << std::endl;
            return;
        }
        return;
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
        if (Shape)
        {
            for (int i = 0; i < DimCount; i++)
            {
                VecShape.push_back(Shape[i]);
                VecStart.push_back(0);
                VecCount.push_back(Shape[i]);
            }
        }
        else
        {
            VecShape = {};
            VecStart = {};
            for (int i = 0; i < DimCount; i++)
            {
                VecCount.push_back(Count[i]);
            }
        }

        if (Type == "compound")
        {
            return (void *)NULL;
        }
#define declare_type(T)                                                        \
    else if (Type == helper::GetType<T>())                                     \
    {                                                                          \
        Variable<T> *variable = &(Reader->m_IO.DefineVariable<T>(              \
            variableName, VecShape, VecStart, VecCount));                      \
        variable->m_AvailableStepsCount = 1;                                   \
        return (void *)variable;                                               \
    }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
        return (void *)NULL;
    };

    auto arrayBlocksInfoCallback =
        [](void *reader, void *variable, const char *type, int WriterRank,
           int DimCount, size_t *Shape, size_t *Start, size_t *Count) {
            std::vector<size_t> VecShape;
            std::vector<size_t> VecStart;
            std::vector<size_t> VecCount;
            std::string Type(type);
            class SstReader::SstReader *Reader =
                reinterpret_cast<class SstReader::SstReader *>(reader);
            size_t currentStep = SstCurrentStep(Reader->m_Input);
            /*
             * setup shape of array variable as global (I.E. Count == Shape,
             * Start == 0)
             */
            if (Shape)
            {
                for (int i = 0; i < DimCount; i++)
                {
                    VecShape.push_back(Shape[i]);
                    VecStart.push_back(Start[i]);
                    VecCount.push_back(Count[i]);
                }
            }
            else
            {
                VecShape = {};
                VecStart = {};
                for (int i = 0; i < DimCount; i++)
                {
                    VecCount.push_back(Count[i]);
                }
            }

            if (Type == "compound")
            {
                return;
            }
#define declare_type(T)                                                        \
    else if (Type == helper::GetType<T>())                                     \
    {                                                                          \
        Variable<T> *Var = reinterpret_cast<class Variable<T> *>(variable);    \
        auto savedShape = Var->m_Shape;                                        \
        auto savedCount = Var->m_Count;                                        \
        auto savedStart = Var->m_Start;                                        \
        Var->m_Shape = VecShape;                                               \
        Var->m_Count = VecCount;                                               \
        Var->m_Start = VecStart;                                               \
        Var->SetBlockInfo((T *)NULL, currentStep);                             \
        Var->m_Shape = savedShape;                                             \
        Var->m_Count = savedCount;                                             \
        Var->m_Start = savedStart;                                             \
    }
            ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
            return;
        };

    SstReaderInitFFSCallback(m_Input, this, varFFSCallback, arrayFFSCallback,
                             attrFFSCallback, arrayBlocksInfoCallback);

    delete[] cstr;
}

SstReader::~SstReader() { SstStreamDestroy(m_Input); }

StepStatus SstReader::BeginStep(StepMode Mode, const float timeout_sec)
{
    TAU_SCOPED_TIMER_FUNC();

    SstStatusValue result;
    if (m_BetweenStepPairs)
    {
        throw std::logic_error("ERROR: BeginStep() is called a second time "
                               "without an intervening EndStep()");
    }

    switch (Mode)
    {
    case adios2::StepMode::Append:
    case adios2::StepMode::Update:
        throw std::invalid_argument(
            "ERROR: SstReader::BeginStep inappropriate StepMode specified" +
            m_EndMessage);
    case adios2::StepMode::Read:
        break;
    }
    m_IO.RemoveAllVariables();
    m_IO.RemoveAllAttributes();
    result = SstAdvanceStep(m_Input, timeout_sec);
    if (result == SstEndOfStream)
    {
        return StepStatus::EndOfStream;
    }
    if (result == SstTimeout)
    {
        return StepStatus::NotReady;
    }

    if (result != SstSuccess)
    {
        return StepStatus::OtherError;
    }

    m_BetweenStepPairs = true;

    if (m_WriterMarshalMethod == SstMarshalBP)
    {
        TAU_SCOPED_TIMER(
            "BP Marshaling Case - deserialize and install metadata");
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

        m_BP3Deserializer = new format::BP3Deserializer(m_Comm, m_DebugMode);
        m_BP3Deserializer->Init(m_IO.m_Parameters,
                                "in call to BP3::Open for reading");

        m_BP3Deserializer->m_Metadata.Resize(
            (*m_CurrentStepMetaData->WriterMetadata)->DataSize,
            "in SST Streaming Listener");

        std::memcpy(m_BP3Deserializer->m_Metadata.m_Buffer.data(),
                    (*m_CurrentStepMetaData->WriterMetadata)->block,
                    (*m_CurrentStepMetaData->WriterMetadata)->DataSize);

        m_IO.RemoveAllVariables();
        m_BP3Deserializer->ParseMetadata(m_BP3Deserializer->m_Metadata, *this);
        m_IO.ResetVariablesStepSelection(true,
                                         "in call to SST Reader BeginStep");
    }
    else if (m_WriterMarshalMethod == SstMarshalFFS)
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
    m_BetweenStepPairs = false;
    TAU_SCOPED_TIMER_FUNC();
    if (m_ReaderSelectionsLocked && !m_DefinitionsNotified)
    {
        SstReaderDefinitionLock(m_Input, SstCurrentStep(m_Input));
        m_DefinitionsNotified = true;
    }
    if (m_WriterMarshalMethod == SstMarshalFFS)
    {
        SstStatusValue Result;
        // this does all the deferred gets and fills in the variable array data
        Result = SstFFSPerformGets(m_Input);
        if (Result != SstSuccess)
        {
            // tentative, until we change EndStep so that it has a return value
            throw std::runtime_error(
                "ERROR:  Writer failed before returning data");
        }
    }
    if (m_WriterMarshalMethod == SstMarshalBP)
    {

        PerformGets();
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
    SstReleaseStep(m_Input);
}

void SstReader::Flush(const int transportIndex) {}

// PRIVATE
void SstReader::Init()
{
    SstParamParser Parser;

    Parser.ParseParams(m_IO, Params);

#define set_params(Param, Type, Typedecl, Default) m_##Param = Params.Param;
    SST_FOREACH_PARAMETER_TYPE_4ARGS(set_params);
#undef set_params
}

#define declare_gets(T)                                                        \
    void SstReader::DoGetSync(Variable<T> &variable, T *data)                  \
    {                                                                          \
        if (m_BetweenStepPairs == false)                                       \
        {                                                                      \
            throw std::logic_error(                                            \
                "ERROR: When using the SST engine in ADIOS2, "                 \
                "Get() calls must appear between "                             \
                "BeginStep/EndStep pairs");                                    \
        }                                                                      \
                                                                               \
        if (m_WriterMarshalMethod == SstMarshalFFS)                            \
        {                                                                      \
            size_t *Start = NULL;                                              \
            size_t *Count = NULL;                                              \
            size_t DimCount = 0;                                               \
                                                                               \
            if (variable.m_SelectionType ==                                    \
                adios2::SelectionType::BoundingBox)                            \
            {                                                                  \
                DimCount = variable.m_Shape.size();                            \
                Start = variable.m_Start.data();                               \
                Count = variable.m_Count.data();                               \
                SstFFSGetDeferred(m_Input, (void *)&variable,                  \
                                  variable.m_Name.c_str(), DimCount, Start,    \
                                  Count, data);                                \
            }                                                                  \
            else if (variable.m_SelectionType ==                               \
                     adios2::SelectionType::WriteBlock)                        \
            {                                                                  \
                DimCount = variable.m_Count.size();                            \
                Count = variable.m_Count.data();                               \
                SstFFSGetLocalDeferred(m_Input, (void *)&variable,             \
                                       variable.m_Name.c_str(), DimCount,      \
                                       variable.m_BlockID, Count, data);       \
            }                                                                  \
            SstFFSPerformGets(m_Input);                                        \
        }                                                                      \
        if (m_WriterMarshalMethod == SstMarshalBP)                             \
        {                                                                      \
            /*  DoGetSync() is going to have terrible performance 'cause */    \
            /*  it's a bad idea in an SST-like environment.  But do */         \
            /*  whatever you do forDoGetDeferred() and then PerformGets() */   \
            DoGetDeferred(variable, data);                                     \
            PerformGets();                                                     \
        }                                                                      \
    }                                                                          \
                                                                               \
    void SstReader::DoGetDeferred(Variable<T> &variable, T *data)              \
    {                                                                          \
        if (m_BetweenStepPairs == false)                                       \
        {                                                                      \
            throw std::logic_error(                                            \
                "ERROR: When using the SST engine in ADIOS2, "                 \
                "Get() calls must appear between "                             \
                "BeginStep/EndStep pairs");                                    \
        }                                                                      \
                                                                               \
        if (m_WriterMarshalMethod == SstMarshalFFS)                            \
        {                                                                      \
            size_t *Start = NULL;                                              \
            size_t *Count = NULL;                                              \
            size_t DimCount = 0;                                               \
                                                                               \
            if (variable.m_SelectionType ==                                    \
                adios2::SelectionType::BoundingBox)                            \
            {                                                                  \
                DimCount = variable.m_Shape.size();                            \
                Start = variable.m_Start.data();                               \
                Count = variable.m_Count.data();                               \
                SstFFSGetDeferred(m_Input, (void *)&variable,                  \
                                  variable.m_Name.c_str(), DimCount, Start,    \
                                  Count, data);                                \
            }                                                                  \
            else if (variable.m_SelectionType ==                               \
                     adios2::SelectionType::WriteBlock)                        \
            {                                                                  \
                DimCount = variable.m_Count.size();                            \
                Count = variable.m_Count.data();                               \
                SstFFSGetLocalDeferred(m_Input, (void *)&variable,             \
                                       variable.m_Name.c_str(), DimCount,      \
                                       variable.m_BlockID, Count, data);       \
            }                                                                  \
        }                                                                      \
        if (m_WriterMarshalMethod == SstMarshalBP)                             \
        {                                                                      \
            /*  Look at the data requested and examine the metadata to see  */ \
            /*  what writer has what you need.  Build up a set of read */      \
            /*  requests (maybe just get all the data from every writer */     \
            /*  that has *something* you need).  You'll use this in EndStep,*/ \
            /*  when you have to get all the array data and put it where  */   \
            /*  it's supposed to go. */                                        \
            /* m_BP3Deserializer->GetDeferredVariable(variable, data);  */     \
            if (variable.m_SingleValue)                                        \
            {                                                                  \
                *data = variable.m_Value;                                      \
            }                                                                  \
            else                                                               \
            {                                                                  \
                m_BP3Deserializer->InitVariableBlockInfo(variable, data);      \
                m_BP3Deserializer->m_DeferredVariables.insert(                 \
                    variable.m_Name);                                          \
            }                                                                  \
        }                                                                      \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_gets)
#undef declare_gets

void SstReader::PerformGets()
{
    if (m_WriterMarshalMethod == SstMarshalFFS)
    {
        SstFFSPerformGets(m_Input);
    }
    else if (m_WriterMarshalMethod == SstMarshalBP)
    {
        if (m_BP3Deserializer->m_DeferredVariables.empty())
        {
            return;
        }

        for (const std::string &name : m_BP3Deserializer->m_DeferredVariables)
        {
            const std::string type = m_IO.InquireVariableType(name);

            if (type == "compound")
            {
            }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        Variable<T> &variable =                                                \
            FindVariable<T>(name, "in call to PerformGets, EndStep or Close"); \
        for (auto &blockInfo : variable.m_BlocksInfo)                          \
        {                                                                      \
            m_BP3Deserializer->SetVariableBlockInfo(variable, blockInfo);      \
        }                                                                      \
        ReadVariableBlocks(variable);                                          \
        variable.m_BlocksInfo.clear();                                         \
    }
            ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
        }

        m_BP3Deserializer->m_DeferredVariables.clear();
    }
    else
    {
        // unknown marshaling method, shouldn't happen
    }
}

void SstReader::DoClose(const int transportIndex) { SstReaderClose(m_Input); }

#define declare_type(T)                                                        \
    std::map<size_t, std::vector<typename Variable<T>::Info>>                  \
    SstReader::DoAllStepsBlocksInfo(const Variable<T> &variable) const         \
    {                                                                          \
        if (m_WriterMarshalMethod == SstMarshalFFS)                            \
        {                                                                      \
            throw std::invalid_argument("ERROR: SST Engine doesn't implement " \
                                        "function DoAllStepsBlocksInfo\n");    \
        }                                                                      \
        else if (m_WriterMarshalMethod == SstMarshalBP)                        \
        {                                                                      \
            return m_BP3Deserializer->AllStepsBlocksInfo(variable);            \
        }                                                                      \
        throw std::invalid_argument(                                           \
            "ERROR: Unknown marshal mechanism in DoAllStepsBlocksInfo\n");     \
    }                                                                          \
                                                                               \
    std::vector<typename Variable<T>::Info> SstReader::DoBlocksInfo(           \
        const Variable<T> &variable, const size_t step) const                  \
    {                                                                          \
        if (m_WriterMarshalMethod == SstMarshalFFS)                            \
        {                                                                      \
            return variable.m_BlocksInfo;                                      \
        }                                                                      \
        else if (m_WriterMarshalMethod == SstMarshalBP)                        \
        {                                                                      \
            return m_BP3Deserializer->BlocksInfo(variable, 0);                 \
        }                                                                      \
        throw std::invalid_argument(                                           \
            "ERROR: Unknown marshal mechanism in DoBlocksInfo\n");             \
    }

ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

} // end namespace engine
} // end namespace core
} // end namespace adios2
