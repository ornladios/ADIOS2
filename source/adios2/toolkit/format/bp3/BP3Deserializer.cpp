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

BP3Deserializer::BP3Deserializer(MPI_Comm mpiComm, const bool debugMode)
: BP3Base(mpiComm, debugMode)
{
}

void BP3Deserializer::ParseMetadata(const BufferSTL &bufferSTL, core::IO &io)
{
    ParseMinifooter(bufferSTL);
    ParsePGIndex(bufferSTL, io);
    ParseVariablesIndex(bufferSTL, io);
    ParseAttributesIndex(bufferSTL, io);
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
            throw std::runtime_error("ERROR: reader found BigEndian bp file, "
                                     "this version of ADIOS2 wasn't compiled "
                                     "with -DADIOS2_USE_ENDIAN_REVERSE=ON "
                                     "explicitly, in call to Open\n");
        }
    }
#endif

    position += 1;

    const uint8_t subFilesIndex = helper::ReadValue<uint8_t>(
        buffer, position, m_Minifooter.IsLittleEndian);
    if (subFilesIndex > 0)
    {
        m_Minifooter.HasSubFiles = true;
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
                                   const core::IO &io)
{
    const auto &buffer = bufferSTL.m_Buffer;
    size_t position = m_Minifooter.PGIndexStart;

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

    if (m_IsRowMajor != helper::IsRowMajor(io.m_HostLanguage))
    {
        m_ReverseDimensions = true;
    }
}

void BP3Deserializer::ParseVariablesIndex(const BufferSTL &bufferSTL,
                                          core::IO &io)
{
    auto lf_ReadElementIndex = [&](
        core::IO &io, const std::vector<char> &buffer, size_t position) {

        const ElementIndexHeader header = ReadElementIndexHeader(
            buffer, position, m_Minifooter.IsLittleEndian);

        switch (header.DataType)
        {

        case (type_string):
        {
            DefineVariableInIO<std::string>(header, io, buffer, position);
            break;
        }

        case (type_byte):
        {
            DefineVariableInIO<signed char>(header, io, buffer, position);
            break;
        }

        case (type_short):
        {
            DefineVariableInIO<short>(header, io, buffer, position);
            break;
        }

        case (type_integer):
        {
            DefineVariableInIO<int>(header, io, buffer, position);
            break;
        }

        case (type_long):
        {
            DefineVariableInIO<int64_t>(header, io, buffer, position);
            break;
        }

        case (type_unsigned_byte):
        {
            DefineVariableInIO<unsigned char>(header, io, buffer, position);
            break;
        }

        case (type_unsigned_short):
        {
            DefineVariableInIO<unsigned short>(header, io, buffer, position);
            break;
        }

        case (type_unsigned_integer):
        {
            DefineVariableInIO<unsigned int>(header, io, buffer, position);
            break;
        }

        case (type_unsigned_long):
        {
            DefineVariableInIO<uint64_t>(header, io, buffer, position);
            break;
        }

        case (type_real):
        {
            DefineVariableInIO<float>(header, io, buffer, position);
            break;
        }

        case (type_double):
        {
            DefineVariableInIO<double>(header, io, buffer, position);
            break;
        }

        case (type_long_double):
        {
            DefineVariableInIO<long double>(header, io, buffer, position);
            break;
        }

        case (type_complex):
        {
            DefineVariableInIO<std::complex<float>>(header, io, buffer,
                                                    position);
            break;
        }

        case (type_double_complex):
        {
            DefineVariableInIO<std::complex<double>>(header, io, buffer,
                                                     position);
            break;
        }

        } // end switch
    };

    const auto &buffer = bufferSTL.m_Buffer;
    size_t position = m_Minifooter.VarsIndexStart;

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
            lf_ReadElementIndex(io, buffer, position);

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
}

void BP3Deserializer::ParseAttributesIndex(const BufferSTL &bufferSTL,
                                           core::IO &io)
{
    auto lf_ReadElementIndex = [&](
        core::IO &io, const std::vector<char> &buffer, size_t position) {

        const ElementIndexHeader header = ReadElementIndexHeader(
            buffer, position, m_Minifooter.IsLittleEndian);

        switch (header.DataType)
        {

        case (type_string):
        {
            DefineAttributeInIO<std::string>(header, io, buffer, position);
            break;
        }

        case (type_string_array):
        {
            DefineAttributeInIO<std::string>(header, io, buffer, position);
            break;
        }

        case (type_byte):
        {
            DefineAttributeInIO<signed char>(header, io, buffer, position);
            break;
        }

        case (type_short):
        {
            DefineAttributeInIO<short>(header, io, buffer, position);
            break;
        }

        case (type_integer):
        {
            DefineAttributeInIO<int>(header, io, buffer, position);
            break;
        }

        case (type_long):
        {
            DefineAttributeInIO<int64_t>(header, io, buffer, position);
            break;
        }

        case (type_unsigned_byte):
        {
            DefineAttributeInIO<unsigned char>(header, io, buffer, position);
            break;
        }

        case (type_unsigned_short):
        {
            DefineAttributeInIO<unsigned short>(header, io, buffer, position);
            break;
        }

        case (type_unsigned_integer):
        {
            DefineAttributeInIO<unsigned int>(header, io, buffer, position);
            break;
        }

        case (type_unsigned_long):
        {
            DefineAttributeInIO<uint64_t>(header, io, buffer, position);
            break;
        }

        case (type_real):
        {
            DefineAttributeInIO<float>(header, io, buffer, position);
            break;
        }

        case (type_double):
        {
            DefineAttributeInIO<double>(header, io, buffer, position);
            break;
        }

        case (type_long_double):
        {
            DefineAttributeInIO<long double>(header, io, buffer, position);
            break;
        }

        } // end switch
    };

    const auto &buffer = bufferSTL.m_Buffer;
    size_t position = m_Minifooter.AttributesIndexStart;

    const uint32_t count = helper::ReadValue<uint32_t>(
        buffer, position, m_Minifooter.IsLittleEndian);
    const uint64_t length = helper::ReadValue<uint64_t>(
        buffer, position, m_Minifooter.IsLittleEndian);

    const size_t startPosition = position;
    size_t localPosition = 0;

    // Read sequentially
    while (localPosition < length)
    {
        lf_ReadElementIndex(io, buffer, position);
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
        ADIOS2_FOREACH_TYPE_1ARG(declare_type)
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
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
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

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
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

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace formata
} // end namespace adios2
