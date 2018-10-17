/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4FileWriter.cpp
 *
 *  Created on: Aug 1, 2018
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#include "BP4FileWriter.h"
#include "BP4FileWriter.tcc"

#include "adios2/ADIOSMPI.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/core/IO.h"
#include "adios2/helper/adiosFunctions.h" //CheckIndexRange
#include "adios2/toolkit/transport/file/FileFStream.h"

namespace adios2
{
namespace core
{
namespace engine
{

BP4FileWriter::BP4FileWriter(IO &io, const std::string &name, const Mode mode,
                           MPI_Comm mpiComm)
: Engine("BP4FileWriter", io, name, mode, mpiComm),
  m_BP4Serializer(mpiComm, m_DebugMode),
  m_FileDataManager(mpiComm, m_DebugMode),
  m_FileMetadataManager(mpiComm, m_DebugMode),
  m_FileMetadataIndexManager(mpiComm, m_DebugMode)
{
    m_EndMessage = " in call to IO Open BP4FileWriter " + m_Name + "\n";
    Init();
}

BP4FileWriter::~BP4FileWriter() = default;

StepStatus BP4FileWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    m_BP4Serializer.m_DeferredVariables.clear();
    m_BP4Serializer.m_DeferredVariablesDataSize = 0;
    return StepStatus::OK;
}

size_t BP4FileWriter::CurrentStep() const
{
    return m_BP4Serializer.m_MetadataSet.CurrentStep;
}

void BP4FileWriter::PerformPuts()
{
    if (m_BP4Serializer.m_DeferredVariables.empty())
    {
        return;
    }

    m_BP4Serializer.ResizeBuffer(m_BP4Serializer.m_DeferredVariablesDataSize,
                                 "in call to PerformPuts");

    for (const std::string &variableName : m_BP4Serializer.m_DeferredVariables)
    {
        const std::string type = m_IO.InquireVariableType(variableName);
        if (type == "compound")
        {
            // not supported
        }
#define declare_template_instantiation(T)                                      \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        Variable<T> &variable = FindVariable<T>(                               \
            variableName, "in call to PerformPuts, EndStep or Close");         \
                                                                               \
        for (const auto &blockInfo : variable.m_BlocksInfo)                    \
        {                                                                      \
            PutSyncCommon(variable, blockInfo);                                \
        }                                                                      \
        variable.m_BlocksInfo.clear();                                         \
    }

        ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
    }
    m_BP4Serializer.m_DeferredVariables.clear();
}

void BP4FileWriter::EndStep()
{
    if (m_BP4Serializer.m_DeferredVariables.size() > 0)
    {
        PerformPuts();
    }

    // true: advances step
    m_BP4Serializer.SerializeData(m_IO, true);

    const size_t currentStep = CurrentStep();
    const size_t flushStepsCount = m_BP4Serializer.m_FlushStepsCount;

    if (currentStep % flushStepsCount == 0)
    {
        Flush();
    }
}

void BP4FileWriter::Flush(const int transportIndex)
{
    DoFlush(false, transportIndex);
    m_BP4Serializer.ResetBuffer(m_BP4Serializer.m_Data);

    if (m_BP4Serializer.m_CollectiveMetadata)
    {
        WriteCollectiveMetadataFile();
    }
}

// PRIVATE
void BP4FileWriter::Init()
{
    InitParameters();
    InitTransports();
    InitBPBuffer();
}

#define declare_type(T)                                                        \
    void BP4FileWriter::DoPutSync(Variable<T> &variable, const T *data)         \
    {                                                                          \
        PutSyncCommon(variable, variable.SetBlockInfo(data, CurrentStep()));   \
        variable.m_BlocksInfo.clear();                                         \
    }                                                                          \
    void BP4FileWriter::DoPutDeferred(Variable<T> &variable, const T *data)     \
    {                                                                          \
        PutDeferredCommon(variable, data);                                     \
    }

ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void BP4FileWriter::InitParameters()
{
    m_BP4Serializer.InitParameters(m_IO.m_Parameters);
}

void BP4FileWriter::InitTransports()
{
    // TODO need to add support for aggregators here later
    if (m_IO.m_TransportsParameters.empty())
    {
        Params defaultTransportParameters;
        defaultTransportParameters["transport"] = "File";
        m_IO.m_TransportsParameters.push_back(defaultTransportParameters);
    }

    // only consumers will interact with transport managers
    std::vector<std::string> bpSubStreamNames;

    if (m_BP4Serializer.m_Aggregator.m_IsConsumer)
    {
        // Names passed to IO AddTransport option with key "Name"
        const std::vector<std::string> transportsNames =
            m_FileDataManager.GetFilesBaseNames(m_Name,
                                                m_IO.m_TransportsParameters);

        // /path/name.bp.dir/name.bp.rank
        bpSubStreamNames = m_BP4Serializer.GetBPSubStreamNames(transportsNames);
    }

    m_BP4Serializer.ProfilerStart("mkdir");
    m_FileDataManager.MkDirsBarrier(bpSubStreamNames);
    m_BP4Serializer.ProfilerStop("mkdir");

    if (m_BP4Serializer.m_Aggregator.m_IsConsumer)
    {
        m_FileDataManager.OpenFiles(bpSubStreamNames, m_OpenMode,
                                    m_IO.m_TransportsParameters,
                                    m_BP4Serializer.m_Profiler.IsActive);
    }
}

void BP4FileWriter::InitBPBuffer()
{
    if (m_OpenMode == Mode::Append)
    {
        throw std::invalid_argument(
            "ADIOS2: OpenMode Append hasn't been implemented, yet");
        // TODO: Get last pg timestep and update timestep counter in
    }
    else
    {
        m_BP4Serializer.PutProcessGroupIndex(
            m_IO.m_Name, m_IO.m_HostLanguage,
            m_FileDataManager.GetTransportsTypes());
    }
}

void BP4FileWriter::DoFlush(const bool isFinal, const int transportIndex)
{
    if (m_BP4Serializer.m_Aggregator.m_IsActive)
    {
        AggregateWriteData(isFinal, transportIndex);
    }
    else
    {
        WriteData(isFinal, transportIndex);
    }
}

void BP4FileWriter::DoClose(const int transportIndex)
{
    if (m_BP4Serializer.m_DeferredVariables.size() > 0)
    {
        PerformPuts();
        
        DoFlush(false, transportIndex);

        if (m_BP4Serializer.m_CollectiveMetadata &&
            m_FileDataManager.AllTransportsClosed())
        {
            WriteCollectiveMetadataFile(false);
        }
    }

    DoFlush(true, transportIndex);

    if (m_BP4Serializer.m_Aggregator.m_IsConsumer)
    {
        m_FileDataManager.CloseFiles(transportIndex);
    }
    
    if (m_BP4Serializer.m_CollectiveMetadata &&
        m_FileDataManager.AllTransportsClosed())
    {
        WriteCollectiveMetadataFile(true);
    }
    

    if (m_BP4Serializer.m_Profiler.IsActive &&
        m_FileDataManager.AllTransportsClosed())
    {
        WriteProfilingJSONFile();
    }
    if (m_BP4Serializer.m_Aggregator.m_IsActive)
    {
        m_BP4Serializer.m_Aggregator.Close();
    }
}

void BP4FileWriter::WriteProfilingJSONFile()
{
    auto transportTypes = m_FileDataManager.GetTransportsTypes();
    auto transportProfilers = m_FileDataManager.GetTransportsProfilers();

    auto transportTypesMD = m_FileMetadataManager.GetTransportsTypes();
    auto transportProfilersMD = m_FileMetadataManager.GetTransportsProfilers();

    transportTypes.insert(transportTypes.end(), transportTypesMD.begin(),
                          transportTypesMD.end());

    transportProfilers.insert(transportProfilers.end(),
                              transportProfilersMD.begin(),
                              transportProfilersMD.end());

    const std::string lineJSON(m_BP4Serializer.GetRankProfilingJSON(
                                   transportTypes, transportProfilers) +
                               ",\n");

    const std::vector<char> profilingJSON(
        m_BP4Serializer.AggregateProfilingJSON(lineJSON));

    if (m_BP4Serializer.m_RankMPI == 0)
    {
        transport::FileFStream profilingJSONStream(m_MPIComm, m_DebugMode);
        auto bpBaseNames = m_BP4Serializer.GetBPBaseNames({m_Name});
        profilingJSONStream.Open(bpBaseNames[0] + "/profiling.json",
                                 Mode::Write);
        profilingJSONStream.Write(profilingJSON.data(), profilingJSON.size());
        profilingJSONStream.Close();
    }
}

/*generate the header for the metadata index file*/
void BP4FileWriter::PopulateMetadataIndexFileHeader(std::vector<char> &buffer, 
        size_t &position, const uint8_t version, const bool addSubfiles)
{
    auto lf_CopyVersionChar = [](const std::string version,
                                 std::vector<char> &buffer, size_t &position) {
        helper::CopyToBuffer(buffer, position, version.c_str());
    };

    const std::string majorVersion(std::to_string(ADIOS2_VERSION_MAJOR));
    const std::string minorVersion(std::to_string(ADIOS2_VERSION_MINOR));
    const std::string patchVersion(std::to_string(ADIOS2_VERSION_PATCH));

    const std::string versionLongTag("ADIOS-BP v" + majorVersion + "." +
                                     minorVersion + "." + patchVersion);
    const size_t versionLongTagSize = versionLongTag.size();
    if (versionLongTagSize < 24)
    {
        helper::CopyToBuffer(buffer, position, versionLongTag.c_str(),
                     versionLongTagSize);
        position += 24 - versionLongTagSize;
    }
    else
    {
        helper::CopyToBuffer(buffer, position, versionLongTag.c_str(), 24);
    }

    lf_CopyVersionChar(majorVersion, buffer, position);
    lf_CopyVersionChar(minorVersion, buffer, position);
    lf_CopyVersionChar(patchVersion, buffer, position);
    ++position;

    const uint8_t endianness = helper::IsLittleEndian() ? 0 : 1;
    helper::CopyToBuffer(buffer, position, &endianness);

    if (addSubfiles)
    {
        position += 1;
        helper::CopyToBuffer(buffer, position, &version);
    }
    else
    {
        position += 2;
    }
    helper::CopyToBuffer(buffer, position, &version);
    position += 16;
}

/*write the content of metadata index file*/
void BP4FileWriter::PopulateMetadataIndexFileContent(const uint64_t currentStep, 
    const uint64_t mpirank, const uint64_t pgIndexStart, const uint64_t variablesIndexStart,
    const uint64_t attributesIndexStart, const uint64_t currentStepEndPos, std::vector<char> &buffer, 
    size_t &position)
{
    helper::CopyToBuffer(buffer, position, &currentStep);
    helper::CopyToBuffer(buffer, position, &mpirank);
    helper::CopyToBuffer(buffer, position, &pgIndexStart);
    helper::CopyToBuffer(buffer, position, &variablesIndexStart);
    helper::CopyToBuffer(buffer, position, &attributesIndexStart);
    helper::CopyToBuffer(buffer, position, &currentStepEndPos);
}

void BP4FileWriter::WriteCollectiveMetadataFile(const bool isFinal)
{

    m_BP4Serializer.AggregateCollectiveMetadata(
        m_MPIComm, m_BP4Serializer.m_Metadata, true);

    if (m_BP4Serializer.m_RankMPI == 0)
    {
        if (isFinal && m_BP4Serializer.m_MetadataSet.metadataFileLength > 0)
        {
            // if some metadata has already been written, don't need to write
            // it at close.  
            return;
        }
        // first init metadata files
        const std::vector<std::string> transportsNames =
            m_FileMetadataManager.GetFilesBaseNames(
                m_Name, m_IO.m_TransportsParameters);

        const std::vector<std::string> bpMetadataFileNames =
            m_BP4Serializer.GetBPMetadataFileNames(transportsNames);

        /*
        m_FileMetadataManager.OpenFiles(bpMetadataFileNames, m_OpenMode,
                                        m_IO.m_TransportsParameters,
                                        m_BP4Serializer.m_Profiler.IsActive);
        */

        m_FileMetadataManager.OpenFiles(bpMetadataFileNames, adios2::Mode::Append,
                                        m_IO.m_TransportsParameters,
                                        m_BP4Serializer.m_Profiler.IsActive);

        m_FileMetadataManager.WriteFiles(
            m_BP4Serializer.m_Metadata.m_Buffer.data(),
            m_BP4Serializer.m_Metadata.m_Position);
        m_FileMetadataManager.CloseFiles();

        /*record the starting position of indices in metadata file*/
        const uint64_t pgIndexStartMetadataFile = m_BP4Serializer.m_MetadataSet.pgIndexStart + m_BP4Serializer.m_MetadataSet.metadataFileLength;
        const uint64_t varIndexStartMetadataFile = m_BP4Serializer.m_MetadataSet.varIndexStart + m_BP4Serializer.m_MetadataSet.metadataFileLength;
        const uint64_t attrIndexStartMetadataFile = m_BP4Serializer.m_MetadataSet.attrIndexStart + m_BP4Serializer.m_MetadataSet.metadataFileLength;
        size_t currentStepEndPos = m_BP4Serializer.m_MetadataSet.metadataFileLength + m_BP4Serializer.m_Metadata.m_Position;

        BufferSTL metadataIndex;
        metadataIndex.m_Buffer.resize(48);
        metadataIndex.m_Buffer.assign(metadataIndex.m_Buffer.size(), '\0');
        metadataIndex.m_Position = 0;

        //std::vector<std::string> metadataIndexFileNames;
        //metadataIndexFileNames.push_back(bpMetadataFileNames[0]+".mdx."+std::to_string(m_BP4Serializer.m_RankMPI));
        std::vector<std::string> metadataIndexFileNames = 
            m_BP4Serializer.GetBPMetadataIndexFileNames(transportsNames);

        m_FileMetadataIndexManager.OpenFiles(metadataIndexFileNames, adios2::Mode::Append,
                                        m_IO.m_TransportsParameters,
                                        m_BP4Serializer.m_Profiler.IsActive);
        //metadataIndexFileNames.pop_back();    

        uint64_t currentStep;
        if (isFinal && m_BP4Serializer.m_MetadataSet.metadataFileLength == 0) 
        {
            currentStep = m_BP4Serializer.m_MetadataSet.TimeStep;
        }
        else 
        {
            currentStep = m_BP4Serializer.m_MetadataSet.TimeStep-1; // The current TimeStep has already been increased by 1 at this point, so decrease it by 1
        }
        


        if ( currentStep == 1)     // TimeStep starts from 1
        {
            PopulateMetadataIndexFileHeader(metadataIndex.m_Buffer, metadataIndex.m_Position, 4, true);
            m_FileMetadataIndexManager.WriteFiles(
                metadataIndex.m_Buffer.data(),
                metadataIndex.m_Position);
            
            metadataIndex.m_Buffer.resize(48);
            metadataIndex.m_Buffer.assign(metadataIndex.m_Buffer.size(), '\0');
            metadataIndex.m_Position = 0;
        }

        PopulateMetadataIndexFileContent(currentStep, m_BP4Serializer.m_RankMPI,
            pgIndexStartMetadataFile, varIndexStartMetadataFile, attrIndexStartMetadataFile, currentStepEndPos, 
            metadataIndex.m_Buffer, metadataIndex.m_Position);
    
        m_FileMetadataIndexManager.WriteFiles(
            metadataIndex.m_Buffer.data(),
            metadataIndex.m_Position);
        m_FileMetadataIndexManager.CloseFiles();

        m_BP4Serializer.m_MetadataSet.metadataFileLength += m_BP4Serializer.m_Metadata.m_Position;

        if (!isFinal)
        {
            m_BP4Serializer.ResetBuffer(m_BP4Serializer.m_Metadata, true);
            m_FileMetadataManager.m_Transports.clear();
            /*close the transports for metadata index file*/
            m_FileMetadataIndexManager.m_Transports.clear();
        }

    }
    /*Clear the local indices buffer at the end of each step*/
    m_BP4Serializer.ResetBuffer(m_BP4Serializer.m_Metadata, true);
    m_BP4Serializer.ResetIndicesBuffer();
}

void BP4FileWriter::WriteData(const bool isFinal, const int transportIndex)
{
    size_t dataSize = m_BP4Serializer.m_Data.m_Position;

    if (isFinal)
    {
        m_BP4Serializer.CloseData(m_IO);
        dataSize = m_BP4Serializer.m_Data.m_Position;
    }
    else
    {
        m_BP4Serializer.CloseStream(m_IO);
    }

    m_FileDataManager.WriteFiles(m_BP4Serializer.m_Data.m_Buffer.data(),
                                 dataSize, transportIndex);

    m_FileDataManager.FlushFiles(transportIndex);
}

void BP4FileWriter::AggregateWriteData(const bool isFinal,
                                      const int transportIndex)
{
    m_BP4Serializer.CloseStream(m_IO, false);

    // async?
    for (int r = 0; r < m_BP4Serializer.m_Aggregator.m_Size; ++r)
    {
        std::vector<MPI_Request> dataRequests =
            m_BP4Serializer.m_Aggregator.IExchange(m_BP4Serializer.m_Data, r);

        std::vector<MPI_Request> absolutePositionRequests =
            m_BP4Serializer.m_Aggregator.IExchangeAbsolutePosition(
                m_BP4Serializer.m_Data, r);

        if (m_BP4Serializer.m_Aggregator.m_IsConsumer)
        {
            const BufferSTL &bufferSTL =
                m_BP4Serializer.m_Aggregator.GetConsumerBuffer(
                    m_BP4Serializer.m_Data);

            m_FileDataManager.WriteFiles(bufferSTL.m_Buffer.data(),
                                         bufferSTL.m_Position, transportIndex);

            m_FileDataManager.FlushFiles(transportIndex);
        }

        m_BP4Serializer.m_Aggregator.WaitAbsolutePosition(
            absolutePositionRequests, r);

        m_BP4Serializer.m_Aggregator.Wait(dataRequests, r);
        m_BP4Serializer.m_Aggregator.SwapBuffers(r);
    }

    m_BP4Serializer.UpdateOffsetsInMetadata();

    if (isFinal) // Write metadata footer
    {
        BufferSTL &bufferSTL = m_BP4Serializer.m_Data;
        m_BP4Serializer.ResetBuffer(bufferSTL, false, false);

        m_BP4Serializer.AggregateCollectiveMetadata(
            m_BP4Serializer.m_Aggregator.m_Comm, bufferSTL, false);

        if (m_BP4Serializer.m_Aggregator.m_IsConsumer)
        {
            m_FileDataManager.WriteFiles(bufferSTL.m_Buffer.data(),
                                         bufferSTL.m_Position, transportIndex);

            m_FileDataManager.FlushFiles(transportIndex);
        }
        m_BP4Serializer.m_Aggregator.Close();
    }

    m_BP4Serializer.m_Aggregator.ResetBuffers();
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
