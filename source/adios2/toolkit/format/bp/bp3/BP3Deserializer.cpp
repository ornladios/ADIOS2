/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP3Deserializer.cpp
 *
 *  Created on: Sep 7, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "BP3Deserializer.h"
#include "BP3Deserializer.tcc"

#include <future>
#include <unordered_set>
#include <vector>

#include "adios2/helper/adiosFunctions.h" //helper::ReadValue<T>

#ifdef _WIN32
#pragma warning(disable : 4503) // Windows complains about SubFileInfoMap levels
#endif

namespace adios2
{
namespace format
{

std::mutex BP3Deserializer::m_Mutex;

BP3Deserializer::BP3Deserializer(helper::Comm const &comm, const bool debugMode)
: BP3Base(comm, debugMode), BPBase(comm, debugMode), m_Minifooter(3)
{
}

void BP3Deserializer::ParseMetadata(const BufferSTL &bufferSTL,
                                    core::Engine &engine)

{
    ParseMinifooter(bufferSTL);
    ParsePGIndex(bufferSTL, engine.m_IO.m_HostLanguage);
    ParseVariablesIndex(bufferSTL, engine);
    ParseAttributesIndex(bufferSTL, engine);
}

const helper::BlockOperationInfo &BP3Deserializer::InitPostOperatorBlockData(
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

size_t BP3Deserializer::MetadataStart(const BufferSTL &bufferSTL)
{
    ParseMinifooter(bufferSTL);
    return m_Minifooter.PGIndexStart;
}

// PRIVATE
void BP3Deserializer::ParseMinifooter(const BufferSTL &bufferSTL)
{
    const auto &buffer = bufferSTL.m_Buffer;
    const size_t bufferSize = buffer.size();
    size_t position = bufferSize - 4;
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
                "with the cmake flag -DADIOS2_USE_Endian_Reverse=ON "
                "explicitly, in call to Open\n");
        }
    }
#endif

    position += 1;

    const int8_t fileType = helper::ReadValue<int8_t>(
        buffer, position, m_Minifooter.IsLittleEndian);
    if (fileType == 3)
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

    position = bufferSize - m_MetadataSet.MiniFooterSize;

    m_Minifooter.VersionTag.assign(&buffer[position], 28);
    position += 28;

    m_Minifooter.PGIndexStart = helper::ReadValue<uint64_t>(
        buffer, position, m_Minifooter.IsLittleEndian);
    m_Minifooter.VarsIndexStart = helper::ReadValue<uint64_t>(
        buffer, position, m_Minifooter.IsLittleEndian);
    m_Minifooter.AttributesIndexStart = helper::ReadValue<uint64_t>(
        buffer, position, m_Minifooter.IsLittleEndian);
}

void BP3Deserializer::ParsePGIndex(const BufferSTL &bufferSTL,
                                   const std::string hostLanguage)
{
    const auto &buffer = bufferSTL.m_Buffer;
    // always start from zero
    size_t position = 0;

    m_MetadataSet.DataPGCount = helper::ReadValue<uint64_t>(
        buffer, position, m_Minifooter.IsLittleEndian);
    const size_t length = helper::ReadValue<uint64_t>(
        buffer, position, m_Minifooter.IsLittleEndian);

    size_t localPosition = 0;

    std::unordered_set<uint32_t> stepsFound;
    m_MetadataSet.StepsCount = 0;

    while (localPosition < length)
    {
        ProcessGroupIndex index = ReadProcessGroupIndexHeader(
            buffer, position, m_Minifooter.IsLittleEndian);
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

    if (m_IsRowMajor != helper::IsRowMajor(hostLanguage))
    {
        m_ReverseDimensions = true;
    }
}

void BP3Deserializer::ParseVariablesIndex(const BufferSTL &bufferSTL,
                                          core::Engine &engine)
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
        DefineVariableInEngineIO<T>(header, engine, buffer, position);         \
        break;                                                                 \
    }
            ADIOS2_FOREACH_STDTYPE_1ARG(make_case)
#undef make_case

        } // end switch
    };

    const auto &buffer = bufferSTL.m_Buffer;
    size_t position = helper::GetDistance(
        m_Minifooter.VarsIndexStart, m_Minifooter.PGIndexStart, m_DebugMode,
        " BP3 variable index start < pg index start, in call to Open");

    const uint32_t count = helper::ReadValue<uint32_t>(
        buffer, position, m_Minifooter.IsLittleEndian);
    const uint64_t length = helper::ReadValue<uint64_t>(
        buffer, position, m_Minifooter.IsLittleEndian);

    const size_t startPosition = position;
    size_t localPosition = 0;

    if (m_Parameters.Threads == 1)
    {
        while (localPosition < length)
        {
            lf_ReadElementIndex(engine, buffer, position);

            const size_t elementIndexSize =
                static_cast<size_t>(helper::ReadValue<uint32_t>(
                    buffer, position, m_Minifooter.IsLittleEndian));
            position += elementIndexSize;
            localPosition = position - startPosition;
        }
        return;
    }

    // threads for reading Variables
    std::vector<std::future<void>> asyncs(m_Parameters.Threads);
    std::vector<size_t> asyncPositions(m_Parameters.Threads);

    bool launched = false;

    while (localPosition < length)
    {
        // extract async positions
        for (unsigned int t = 0; t < m_Parameters.Threads; ++t)
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
                asyncs[t] = std::async(std::launch::async, lf_ReadElementIndex,
                                       std::ref(engine), std::ref(buffer),
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
}

void BP3Deserializer::ParseAttributesIndex(const BufferSTL &bufferSTL,
                                           core::Engine &engine)
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

    size_t position = helper::GetDistance(
        m_Minifooter.AttributesIndexStart, m_Minifooter.PGIndexStart,
        m_DebugMode,
        " BP3 attributes index start < pg index start, in call to Open");

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

std::map<std::string, helper::SubFileInfoMap>
BP3Deserializer::PerformGetsVariablesSubFileInfo(core::IO &io)
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

void BP3Deserializer::ClipMemory(const std::string &variableName, core::IO &io,
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
    template void BP3Deserializer::GetSyncVariableDataFromStream(              \
        core::Variable<T> &, BufferSTL &) const;                               \
                                                                               \
    template typename core::Variable<T>::Info &                                \
    BP3Deserializer::InitVariableBlockInfo(core::Variable<T> &, T *) const;    \
                                                                               \
    template void BP3Deserializer::SetVariableBlockInfo(                       \
        core::Variable<T> &, typename core::Variable<T>::Info &) const;        \
                                                                               \
    template void BP3Deserializer::ClipContiguousMemory<T>(                    \
        typename core::Variable<T>::Info &, const std::vector<char> &,         \
        const Box<Dims> &, const Box<Dims> &) const;                           \
                                                                               \
    template void BP3Deserializer::GetValueFromMetadata(                       \
        core::Variable<T> &variable, T *) const;

ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

#define declare_template_instantiation(T)                                      \
                                                                               \
    template std::map<std::string, helper::SubFileInfoMap>                     \
    BP3Deserializer::GetSyncVariableSubFileInfo(const core::Variable<T> &)     \
        const;                                                                 \
                                                                               \
    template void BP3Deserializer::GetDeferredVariable(core::Variable<T> &,    \
                                                       T *);                   \
                                                                               \
    template helper::SubFileInfoMap BP3Deserializer::GetSubFileInfo(           \
        const core::Variable<T> &) const;                                      \
                                                                               \
    template std::map<size_t, std::vector<typename core::Variable<T>::Info>>   \
    BP3Deserializer::AllStepsBlocksInfo(const core::Variable<T> &) const;      \
                                                                               \
    template std::vector<std::vector<typename core::Variable<T>::Info>>        \
    BP3Deserializer::AllRelativeStepsBlocksInfo(const core::Variable<T> &)     \
        const;                                                                 \
                                                                               \
    template std::vector<typename core::Variable<T>::Info>                     \
    BP3Deserializer::BlocksInfo(const core::Variable<T> &, const size_t)       \
        const;                                                                 \
                                                                               \
    template void BP3Deserializer::PreDataRead(                                \
        core::Variable<T> &, typename core::Variable<T>::Info &,               \
        const helper::SubStreamBoxInfo &, char *&, size_t &, size_t &,         \
        const size_t);                                                         \
                                                                               \
    template void BP3Deserializer::PostDataRead(                               \
        core::Variable<T> &, typename core::Variable<T>::Info &,               \
        const helper::SubStreamBoxInfo &, const bool, const size_t);

ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace formata
} // end namespace adios2
