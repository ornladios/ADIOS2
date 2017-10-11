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
#include <vector>

#include "adios2/helper/adiosFunctions.h" //ReadValue<T>

namespace adios2
{
namespace format
{

std::mutex BP3Deserializer::m_Mutex;

BP3Deserializer::BP3Deserializer(MPI_Comm mpiComm, const bool debugMode)
: BP3Base(mpiComm, debugMode)
{
}

void BP3Deserializer::ParseMetadata(IO &io)
{
    ParseMinifooter();
    ParsePGIndex();
    ParseVariablesIndex(io);
    // ParseAttributesIndex(io);
}

// PRIVATE
void BP3Deserializer::ParseMinifooter()
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

    const auto &buffer = m_Metadata.m_Buffer;
    const size_t bufferSize = buffer.size();

    size_t position = bufferSize - 4;
    const uint8_t endianess = ReadValue<uint8_t>(buffer, position);
    lf_GetEndianness(endianess, m_Minifooter.IsLittleEndian);
    position += 1;

    const uint8_t subFilesIndex = ReadValue<uint8_t>(buffer, position);
    if (subFilesIndex > 0)
    {
        m_Minifooter.HasSubFiles = true;
    }

    m_Minifooter.Version = ReadValue<uint8_t>(buffer, position);
    if (m_Minifooter.Version < 3)
    {
        throw std::runtime_error("ERROR: ADIOS2 only supports bp format "
                                 "version 3 and above, found " +
                                 std::to_string(m_Minifooter.Version) +
                                 " version \n");
    }

    position = bufferSize - m_MetadataSet.MiniFooterSize;
    m_Minifooter.PGIndexStart = ReadValue<uint64_t>(buffer, position);
    m_Minifooter.VarsIndexStart = ReadValue<uint64_t>(buffer, position);
    m_Minifooter.AttributesIndexStart = ReadValue<uint64_t>(buffer, position);
}

void BP3Deserializer::ParsePGIndex()
{
    const auto &buffer = m_Metadata.m_Buffer;
    auto &position = m_Metadata.m_Position;
    position = m_Minifooter.PGIndexStart;

    m_MetadataSet.DataPGCount = ReadValue<uint64_t>(buffer, position);
    const uint64_t pgLength =
        ReadValue<uint64_t>(buffer, position); // not required
}

void BP3Deserializer::ParseVariablesIndex(IO &io)
{
    auto lf_ReadElementIndex = [&](IO &io, const std::vector<char> &buffer,
                                   size_t position) {

        const ElementIndexHeader header =
            ReadElementIndexHeader(buffer, position);

        switch (header.DataType)
        {

        case (type_byte):
        {
            DefineVariableInIO<char>(header, io, buffer, position);
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
            DefineVariableInIO<long int>(header, io, buffer, position);
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
            DefineVariableInIO<unsigned long int>(header, io, buffer, position);
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

        case (type_long_double_complex):
        {
            DefineVariableInIO<std::complex<long double>>(header, io, buffer,
                                                          position);
            break;
        }
            // TODO: string
        } // end switch
    };

    const auto &buffer = m_Metadata.m_Buffer;
    size_t position = m_Minifooter.VarsIndexStart;

    const uint32_t count = ReadValue<uint32_t>(buffer, position);
    const uint64_t length = ReadValue<uint64_t>(buffer, position);

    const size_t startPosition = position;
    size_t localPosition = 0;

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
            const size_t elementIndexSize = static_cast<const size_t>(
                ReadValue<uint32_t>(buffer, position));
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

void BP3Deserializer::ParseAttributesIndex(IO &io) {}

std::map<std::string, SubFileInfoMap>
BP3Deserializer::PerformGetsVariablesSubFileInfo(IO &io)
{
    if (m_DeferredVariables.empty())
    {
        return m_DeferredVariables;
    }

    for (auto &subFileInfoPair : m_DeferredVariables)
    {
        const std::string variableName(subFileInfoPair.first);
        const std::string type(io.InquireVariableType(variableName));

        if (type == "compound")
        {
        }
#define declare_type(T)                                                        \
    else if (type == GetType<T>())                                             \
    {                                                                          \
        subFileInfoPair.second =                                               \
            GetSubFileInfo(*io.InquireVariable<T>(variableName));              \
    }
        ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
    }
    return m_DeferredVariables;
}

#define declare_template_instantiation(T)                                      \
    template std::map<std::string, SubFileInfoMap>                             \
    BP3Deserializer::GetSyncVariableSubFileInfo(const Variable<T> &variable)   \
        const;                                                                 \
                                                                               \
    template void BP3Deserializer::GetDeferredVariable(Variable<T> &variable,  \
                                                       T *data);

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace format
} // end namespace adios2
