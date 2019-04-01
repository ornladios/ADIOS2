/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4Deserializer.cpp
 *
 *  Created on: Aug 1, 2018
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#include "BP4Deserializer.h"
#include "BP4Deserializer.tcc"

#include <future>
#include <unordered_set>
#include <vector>

#include <iostream>

#include "adios2/helper/adiosFunctions.h" //helper::ReadValue<T>

#ifdef _WIN32
#pragma warning(disable : 4503) // Windows complains about SubFileInfoMap levels
#endif

namespace adios2
{
namespace format
{

std::mutex BP4Deserializer::m_Mutex;

BP4Deserializer::BP4Deserializer(MPI_Comm mpiComm, const bool debugMode)
: BP4Base(mpiComm, debugMode)
{
}

void BP4Deserializer::ParseMetadata(const BufferSTL &bufferSTL,
                                    core::Engine &engine)
{
    // ParseMinifooter(bufferSTL);
    // ParsePGIndex(bufferSTL, io);
    // ParseVariablesIndex(bufferSTL, io);
    // ParseAttributesIndex(bufferSTL, io);
    size_t steps;
    steps = m_MetadataIndexTable[0].size();
    m_MetadataSet.StepsCount = steps;
    m_MetadataSet.CurrentStep = steps - 1;
    /* parse the metadata step by step using the pointers saved in the metadata
    index table */
    for (int i = 0; i < steps; i++)
    {
        ParsePGIndexPerStep(bufferSTL, engine.m_IO.m_HostLanguage, 0, i + 1);
        ParseVariablesIndexPerStep(bufferSTL, engine, 0, i + 1);
        ParseAttributesIndexPerStep(bufferSTL, engine, 0, i + 1);
    }
}

void BP4Deserializer::ParseMetadataIndex(const BufferSTL &bufferSTL)
{
    const auto &buffer = bufferSTL.m_Buffer;
    const size_t bufferSize = buffer.size();
    size_t position = 0;
    position += 28;
    const uint8_t endianness = helper::ReadValue<uint8_t>(buffer, position);
    m_Minifooter.IsLittleEndian = (endianness == 0) ? true : false;
#ifndef ADIOS2_HAVE_ENDIAN_REVERSE
    if (m_DebugMode)
    {
        if (helper::IsLittleEndian() != m_Minifooter.IsLittleEndian)
        {
            throw std::runtime_error(
                "ERROR: reader found BigEndian bp file, "
                "this version of ADIOS2 wasn't compiled "
                "with the cmake flag -DADIOS2_USE_ENDIAN_REVERSE=ON "
                "explicitly, in call to Open\n");
        }
    }
#endif

    position += 1;

    const int8_t fileType = helper::ReadValue<int8_t>(
        buffer, position, m_Minifooter.IsLittleEndian);
    if (fileType >= 3)
    {
        m_Minifooter.HasSubFiles = true;
    }
    else if (fileType == 0 || fileType == 2)
    {
        m_Minifooter.HasSubFiles = false;
    }

    m_Minifooter.Version = helper::ReadValue<uint8_t>(
        buffer, position, m_Minifooter.IsLittleEndian);
    if (m_Minifooter.Version < 3)
    {
        throw std::runtime_error("ERROR: ADIOS2 only supports bp format "
                                 "version 3 and above, found " +
                                 std::to_string(m_Minifooter.Version) +
                                 " version \n");
    }

    position = 0;
    m_Minifooter.VersionTag.assign(&buffer[position], 28);

    position += 48;
    while (position < bufferSize)
    {
        std::vector<uint64_t> ptrs;
        const uint64_t currentStep = helper::ReadValue<uint64_t>(
            buffer, position, m_Minifooter.IsLittleEndian);
        const uint64_t mpiRank = helper::ReadValue<uint64_t>(
            buffer, position, m_Minifooter.IsLittleEndian);
        const uint64_t pgIndexStart = helper::ReadValue<uint64_t>(
            buffer, position, m_Minifooter.IsLittleEndian);
        ptrs.push_back(pgIndexStart);
        const uint64_t variablesIndexStart = helper::ReadValue<uint64_t>(
            buffer, position, m_Minifooter.IsLittleEndian);
        ptrs.push_back(variablesIndexStart);
        const uint64_t attributesIndexStart = helper::ReadValue<uint64_t>(
            buffer, position, m_Minifooter.IsLittleEndian);
        ptrs.push_back(attributesIndexStart);
        const uint64_t currentStepEndPos = helper::ReadValue<uint64_t>(
            buffer, position, m_Minifooter.IsLittleEndian);
        ptrs.push_back(currentStepEndPos);
        m_MetadataIndexTable[mpiRank][currentStep] = ptrs;
    }
}

const helper::BlockOperationInfo &BP4Deserializer::InitPostOperatorBlockData(
    const std::vector<helper::BlockOperationInfo> &blockOperationsInfo) const
{
    size_t index = 0;
    for (const helper::BlockOperationInfo &blockOperationInfo :
         blockOperationsInfo)
    {
        const std::string type = blockOperationInfo.Info.at("Type");
        if (m_TransformTypes.count(type) == 1)
        {
            break;
        }
        ++index;
    }
    return blockOperationsInfo.at(index);
}

/* void BP4Deserializer::GetPreOperatorBlockData(
    const std::vector<char> &postOpData,
    const helper::BlockOperationInfo &blockOperationInfo,
    std::vector<char> &preOpData) const
{
    // pre-allocate decompressed block
    preOpData.resize(helper::GetTotalSize(blockOperationInfo.PreCount) *
                     blockOperationInfo.PreSizeOf);

    // get the right bp4Op
    std::shared_ptr<BP4Operation> bp4Op =
        SetBP4Operation(blockOperationInfo.Info.at("Type"));
    bp4Op->GetData(postOpData.data(), blockOperationInfo, preOpData.data());
} */

// PRIVATE
/* void BP4Deserializer::ParseMinifooter(const BufferSTL &bufferSTL)
{
    auto lf_GetEndianness = [](const uint8_t endianness, bool &isLittleEndian) {
        switch (endianness)
        {
        case 0:
            isLittleEndian = true;
            break;
        case 1:
            isLittleEndian = false;
            break;
        }
    };

    const auto &buffer = bufferSTL.m_Buffer;
    const size_t bufferSize = buffer.size();
    size_t position = bufferSize - 4;
    const uint8_t endianess = helper::ReadValue<uint8_t>(buffer, position);
    lf_GetEndianness(endianess, m_Minifooter.IsLittleEndian);
    position += 1;

    const uint8_t subFilesIndex = helper::ReadValue<uint8_t>(buffer, position);
    if (subFilesIndex > 0)
    {
        m_Minifooter.HasSubFiles = true;
    }

    m_Minifooter.Version = helper::ReadValue<uint8_t>(buffer, position);
    if (m_Minifooter.Version < 3)
    {
        throw std::runtime_error("ERROR: ADIOS2 only supports bp format "
                                 "version 3 and above, found " +
                                 std::to_string(m_Minifooter.Version) +
                                 " version \n");
    }

    position = bufferSize - m_MetadataSet.MiniFooterSize;

    m_Minifooter.VersionTag.assign(&buffer[position], 28);
    position += 28;

    m_Minifooter.PGIndexStart = helper::ReadValue<uint64_t>(buffer, position);
    m_Minifooter.VarsIndexStart = helper::ReadValue<uint64_t>(buffer, position);
    m_Minifooter.AttributesIndexStart =
        helper::ReadValue<uint64_t>(buffer, position);
} */

void BP4Deserializer::ParsePGIndexPerStep(const BufferSTL &bufferSTL,
                                          const std::string hostLanguage,
                                          size_t submetadatafileId, size_t step)
{
    const auto &buffer = bufferSTL.m_Buffer;
    size_t position = m_MetadataIndexTable[submetadatafileId][step][0];
    // std::cout << step << ", " << position << std::endl;
    m_MetadataSet.DataPGCount =
        m_MetadataSet.DataPGCount +
        helper::ReadValue<uint64_t>(buffer, position,
                                    m_Minifooter.IsLittleEndian);

    // read length of pg index which is not used, only for moving the pointer
    helper::ReadValue<uint64_t>(buffer, position, m_Minifooter.IsLittleEndian);

    ProcessGroupIndex index = ReadProcessGroupIndexHeader(
        buffer, position, m_Minifooter.IsLittleEndian);
    if (index.IsColumnMajor == 'y')
    {
        m_IsRowMajor = false;
    }
    if (m_IsRowMajor != helper::IsRowMajor(hostLanguage))
    {
        m_ReverseDimensions = true;
    }
}

/* void BP4Deserializer::ParsePGIndex(const BufferSTL &bufferSTL,
                                   const core::IO &io)
{
    const auto &buffer = bufferSTL.m_Buffer;
    size_t position = m_Minifooter.PGIndexStart;

    m_MetadataSet.DataPGCount = helper::ReadValue<uint64_t>(buffer, position);
    const size_t length = helper::ReadValue<uint64_t>(buffer, position);

    size_t localPosition = 0;

    std::unordered_set<uint32_t> stepsFound;
    m_MetadataSet.StepsCount = 0;

    while (localPosition < length)
    {
        ProcessGroupIndex index = ReadProcessGroupIndexHeader(buffer, position);
        if (index.IsColumnMajor == 'y')
        {
            m_IsRowMajor = false;
        }

        m_MetadataSet.CurrentStep = static_cast<size_t>(index.Step - 1);

        // Count the number of unseen steps
        if (stepsFound.insert(index.Step).second)
        {
            ++m_MetadataSet.StepsCount;
        }

        localPosition += index.Length + 2;
    }

    if (m_IsRowMajor != helper::IsRowMajor(io.m_HostLanguage))
    {
        m_ReverseDimensions = true;
    }
} */

void BP4Deserializer::ParseVariablesIndexPerStep(const BufferSTL &bufferSTL,
                                                 core::Engine &engine,
                                                 size_t submetadatafileId,
                                                 size_t step)
{
    auto lf_ReadElementIndexPerStep = [&](core::Engine &engine,
                                          const std::vector<char> &buffer,
                                          size_t position, size_t step) {
        const ElementIndexHeader header = ReadElementIndexHeader(
            buffer, position, m_Minifooter.IsLittleEndian);

        switch (header.DataType)
        {

#define make_case(T)                                                           \
    case (TypeTraits<T>::type_enum):                                           \
    {                                                                          \
        DefineVariableInEngineIOPerStep<T>(header, engine, buffer, position,   \
                                           step);                              \
        break;                                                                 \
    }
            ADIOS2_FOREACH_STDTYPE_1ARG(make_case)
#undef make_case

        } // end switch
    };

    const auto &buffer = bufferSTL.m_Buffer;
    size_t position = m_MetadataIndexTable[submetadatafileId][step][1];

    const uint32_t count = helper::ReadValue<uint32_t>(
        buffer, position, m_Minifooter.IsLittleEndian);
    const uint64_t length = helper::ReadValue<uint64_t>(
        buffer, position, m_Minifooter.IsLittleEndian);

    const size_t startPosition = position;
    size_t localPosition = 0;

    if (m_Threads == 1)
    {
        while (localPosition < length)
        {
            lf_ReadElementIndexPerStep(engine, buffer, position, step);

            const size_t elementIndexSize =
                static_cast<size_t>(helper::ReadValue<uint32_t>(
                    buffer, position, m_Minifooter.IsLittleEndian));
            position += elementIndexSize;
            localPosition = position - startPosition;
        }
        return;
    }

    // threads for reading Variables
    std::vector<std::future<void>> asyncs(m_Threads);
    std::vector<size_t> asyncPositions(m_Threads);

    bool launched = false;

    while (localPosition < length)
    {
        // extract async positions
        for (unsigned int t = 0; t < m_Threads; ++t)
        {
            asyncPositions[t] = position;
            const size_t elementIndexSize =
                static_cast<size_t>(helper::ReadValue<uint32_t>(
                    buffer, position, m_Minifooter.IsLittleEndian));
            position += elementIndexSize;
            localPosition = position - startPosition;

            if (launched)
            {
                asyncs[t].get();
            }

            if (localPosition <= length)
            {
                asyncs[t] =
                    std::async(std::launch::async, lf_ReadElementIndexPerStep,
                               std::ref(engine), std::ref(buffer),
                               asyncPositions[t], step);
            }
        }
        launched = true;
    }

    for (auto &async : asyncs)
    {
        if (async.valid())
        {
            async.wait();
        }
    }
}

/* void BP4Deserializer::ParseVariablesIndex(const BufferSTL &bufferSTL,
                                          core::IO &io)
{
    auto lf_ReadElementIndex = [&](
        core::IO &io, const std::vector<char> &buffer, size_t position) {
        const ElementIndexHeader header =
            ReadElementIndexHeader(buffer, position);

        switch (header.DataType)
        {

#define make_case(T)                                                           \
    case (TypeTraits<T>::type_enum):                                           \
    {                                                                          \
        DefineVariableInIO<T>(header, io, buffer, position);
        break;                                                                 \
    }
            ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(make_case)
#undef make_case

        } // end switch
    };

    const auto &buffer = bufferSTL.m_Buffer;
    size_t position = m_Minifooter.VarsIndexStart;

    const uint32_t count = helper::ReadValue<uint32_t>(buffer, position);
    const uint64_t length = helper::ReadValue<uint64_t>(buffer, position);

    const size_t startPosition = position;
    size_t localPosition = 0;

    if (m_Threads == 1)
    {
        while (localPosition < length)
        {
            lf_ReadElementIndex(io, buffer, position);

            const size_t elementIndexSize = static_cast<size_t>(
                helper::ReadValue<uint32_t>(buffer, position));
            position += elementIndexSize;
            localPosition = position - startPosition;
        }
        return;
    }

    // threads for reading Variables
    std::vector<std::future<void>> asyncs(m_Threads);
    std::vector<size_t> asyncPositions(m_Threads);

    bool launched = false;

    while (localPosition < length)
    {
        // extract async positions
        for (unsigned int t = 0; t < m_Threads; ++t)
        {
            asyncPositions[t] = position;
            const size_t elementIndexSize = static_cast<size_t>(
                helper::ReadValue<uint32_t>(buffer, position));
            position += elementIndexSize;
            localPosition = position - startPosition;

            if (launched)
            {
                asyncs[t].get();
            }

            if (localPosition <= length)
            {
                asyncs[t] = std::async(std::launch::async, lf_ReadElementIndex,
                                       std::ref(io), std::ref(buffer),
                                       asyncPositions[t]);
            }
        }
        launched = true;
    }

    for (auto &async : asyncs)
    {
        if (async.valid())
        {
            async.wait();
        }
    }
} */

/* Parse the attributes index at each step */
void BP4Deserializer::ParseAttributesIndexPerStep(const BufferSTL &bufferSTL,
                                                  core::Engine &engine,
                                                  size_t submetadatafileId,
                                                  size_t step)
{
    auto lf_ReadElementIndex = [&](core::Engine &engine,
                                   const std::vector<char> &buffer,
                                   size_t position) {
        const ElementIndexHeader header = ReadElementIndexHeader(
            buffer, position, m_Minifooter.IsLittleEndian);

        switch (header.DataType)
        {

#define make_case(T)                                                           \
    case (TypeTraits<T>::type_enum):                                           \
    {                                                                          \
        DefineAttributeInEngineIO<T>(header, engine, buffer, position);        \
        break;                                                                 \
    }
            ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(make_case)
#undef make_case
        case (type_string_array):
        {
            DefineAttributeInEngineIO<std::string>(header, engine, buffer,
                                                   position);
            break;
        }

        } // end switch
    };

    const auto &buffer = bufferSTL.m_Buffer;
    size_t position = m_MetadataIndexTable[submetadatafileId][step][2];

    const uint32_t count = helper::ReadValue<uint32_t>(
        buffer, position, m_Minifooter.IsLittleEndian);
    const uint64_t length = helper::ReadValue<uint64_t>(
        buffer, position, m_Minifooter.IsLittleEndian);

    const size_t startPosition = position;
    size_t localPosition = 0;

    // Read sequentially
    while (localPosition < length)
    {
        lf_ReadElementIndex(engine, buffer, position);
        const size_t elementIndexSize =
            static_cast<size_t>(helper::ReadValue<uint32_t>(
                buffer, position, m_Minifooter.IsLittleEndian));
        position += elementIndexSize;
        localPosition = position - startPosition;
    }
}

/* void BP4Deserializer::ParseAttributesIndex(const BufferSTL &bufferSTL,
                                           core::IO &io)
{
    auto lf_ReadElementIndex = [&](
        core::IO &io, const std::vector<char> &buffer, size_t position) {
        const ElementIndexHeader header =
            ReadElementIndexHeader(buffer, position);

        switch (header.DataType)
        {

#define make_case(T)                                                           \
    case (TypeTraits<T>::type_enum):                                           \
    {                                                                          \
        DefineAttributeInIO<T>(header, io, buffer, position);                  \
        break;                                                                 \
    }
            ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(make_case)
#undef make_case

        case (type_string_array):
        {
            DefineAttributeInIO<std::string>(header, io, buffer, position);
            break;
        }

        } // end switch
    };

    const auto &buffer = bufferSTL.m_Buffer;
    size_t position = m_Minifooter.AttributesIndexStart;

    const uint32_t count = helper::ReadValue<uint32_t>(buffer, position);
    const uint64_t length = helper::ReadValue<uint64_t>(buffer, position);

    const size_t startPosition = position;
    size_t localPosition = 0;

    // Read sequentially
    while (localPosition < length)
    {
        lf_ReadElementIndex(io, buffer, position);
        const size_t elementIndexSize =
            static_cast<size_t>(helper::ReadValue<uint32_t>(buffer, position));
        position += elementIndexSize;
        localPosition = position - startPosition;
    }
} */

std::map<std::string, helper::SubFileInfoMap>
BP4Deserializer::PerformGetsVariablesSubFileInfo(core::IO &io)
{
    if (m_DeferredVariablesMap.empty())
    {
        return m_DeferredVariablesMap;
    }

    for (auto &subFileInfoPair : m_DeferredVariablesMap)
    {
        const std::string variableName(subFileInfoPair.first);
        const std::string type(io.InquireVariableType(variableName));

        if (type == "compound")
        {
        }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        subFileInfoPair.second =                                               \
            GetSubFileInfo(*io.InquireVariable<T>(variableName));              \
    }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
    }
    return m_DeferredVariablesMap;
}

void BP4Deserializer::ClipMemory(const std::string &variableName, core::IO &io,
                                 const std::vector<char> &contiguousMemory,
                                 const Box<Dims> &blockBox,
                                 const Box<Dims> &intersectionBox) const
{
    const std::string type(io.InquireVariableType(variableName));

    if (type == "compound")
    {
    }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        core::Variable<T> *variable = io.InquireVariable<T>(variableName);     \
        if (variable != nullptr)                                               \
        {                                                                      \
            helper::ClipContiguousMemory(variable->m_Data, variable->m_Start,  \
                                         variable->m_Count, contiguousMemory,  \
                                         blockBox, intersectionBox,            \
                                         m_IsRowMajor, m_ReverseDimensions);   \
        }                                                                      \
    }
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
}

#define declare_template_instantiation(T)                                      \
    template void BP4Deserializer::GetSyncVariableDataFromStream(              \
        core::Variable<T> &, BufferSTL &) const;                               \
                                                                               \
    template typename core::Variable<T>::Info &                                \
    BP4Deserializer::InitVariableBlockInfo(core::Variable<T> &, T *) const;    \
                                                                               \
    template void BP4Deserializer::SetVariableBlockInfo(                       \
        core::Variable<T> &, typename core::Variable<T>::Info &) const;        \
                                                                               \
    template void BP4Deserializer::ClipContiguousMemory<T>(                    \
        typename core::Variable<T>::Info &, const std::vector<char> &,         \
        const Box<Dims> &, const Box<Dims> &) const;                           \
                                                                               \
    template void BP4Deserializer::GetValueFromMetadata(                       \
        core::Variable<T> &variable, T *) const;

ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

#define declare_template_instantiation(T)                                      \
                                                                               \
    template std::map<std::string, helper::SubFileInfoMap>                     \
    BP4Deserializer::GetSyncVariableSubFileInfo(const core::Variable<T> &)     \
        const;                                                                 \
                                                                               \
    template void BP4Deserializer::GetDeferredVariable(core::Variable<T> &,    \
                                                       T *);                   \
                                                                               \
    template helper::SubFileInfoMap BP4Deserializer::GetSubFileInfo(           \
        const core::Variable<T> &) const;                                      \
                                                                               \
    template std::map<size_t, std::vector<typename core::Variable<T>::Info>>   \
    BP4Deserializer::AllStepsBlocksInfo(const core::Variable<T> &) const;      \
                                                                               \
    template std::vector<typename core::Variable<T>::Info>                     \
    BP4Deserializer::BlocksInfo(const core::Variable<T> &, const size_t)       \
        const;                                                                 \
                                                                               \
    template void BP4Deserializer::PreDataRead(                                \
        core::Variable<T> &, typename core::Variable<T>::Info &,               \
        const helper::SubStreamBoxInfo &, char *&, size_t &, size_t &,         \
        const size_t);                                                         \
                                                                               \
    template void BP4Deserializer::PostDataRead(                               \
        core::Variable<T> &, typename core::Variable<T>::Info &,               \
        const helper::SubStreamBoxInfo &, const bool, const size_t);

ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace format
} // end namespace adios2
