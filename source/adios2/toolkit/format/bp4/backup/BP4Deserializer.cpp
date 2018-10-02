/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4Deserializer.cpp
 *
 *  Created on: Aug 3, 2018
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#include "BP4Deserializer.h"
#include "BP4Deserializer.tcc"

#include <future>
#include <unordered_set>
#include <vector>

#include <iostream>

#include "adios2/helper/adiosFunctions.h" //ReadValue<T>

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



void BP4Deserializer::ParseMetadata(const BufferSTL &bufferSTL, core::IO &io)
{
    size_t steps;
    steps = m_MetadataIndexTable.size();
    m_MetadataSet.StepsCount = steps;
    m_MetadataSet.CurrentStep = steps-1;
    for (int i = 0; i < steps; i++)
    {
        //for (int j = 0; j < m_MetadataIndexTable[i+1].size(); j++)
        //{
        //    std::cout << m_MetadataIndexTable[i+1][j] << std::endl;
        //}
        ParsePGIndexPerStep(bufferSTL, io, i+1);
        ParseVariablesIndexPerStep(bufferSTL, io, i+1);
        ParseAttributesIndexPerStep(bufferSTL, io, i+1);
    }
}


void BP4Deserializer::ParseMetadataIndex(const BufferSTL &bufferSTL)
{
    const auto &buffer = bufferSTL.m_Buffer;
    const size_t bufferSize = buffer.size();
    size_t position = 0;
    position += 48;
    while (position < bufferSize)
    {
        std::vector<uint64_t> ptrs;
        const uint64_t currentStep = helper::ReadValue<uint64_t>(buffer, position);
        const uint64_t mpiRank = helper::ReadValue<uint64_t>(buffer, position);
        const uint64_t pgIndexStart = helper::ReadValue<uint64_t>(buffer, position);
        ptrs.push_back(pgIndexStart);
        const uint64_t variablesIndexStart = helper::ReadValue<uint64_t>(buffer, position);
        ptrs.push_back(variablesIndexStart);
        const uint64_t attributesIndexStart = helper::ReadValue<uint64_t>(buffer, position);
        ptrs.push_back(attributesIndexStart);
        const uint64_t endptrval = helper::ReadValue<uint64_t>(buffer, position);
        ptrs.push_back(endptrval);
        m_MetadataIndexTable[currentStep] = ptrs;

    }
}

void BP4Deserializer::ClipContiguousMemory(
    const std::string &variableName, core::IO &io,
    const std::vector<char> &contiguousMemory, const Box<Dims> &blockBox,
    const Box<Dims> &intersectionBox) const
{
    // get variable pointer and set data in it with local dimensions
    const std::string type(io.InquireVariableType(variableName));

    if (type == "compound")
    {
    }
#define declare_type(T)                                                        \
    else if (type == GetType<T>())                                             \
    {                                                                          \
        core::Variable<T> *variable = io.InquireVariable<T>(variableName);           \
        if (variable != nullptr)                                               \
        {                                                                      \
            ClipContiguousMemoryCommon(*variable, contiguousMemory, blockBox,  \
                                       intersectionBox);                       \
        }                                                                      \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
}

void BP4Deserializer::SetVariableNextStepData(const std::string &variableName,
                                              core::IO &io) const
{
    const std::string type(io.InquireVariableType(variableName));

    if (type == "compound")
    {
    }
#define declare_type(T)                                                        \
    else if (type == GetType<T>())                                             \
    {                                                                          \
        core::Variable<T> *variable = io.InquireVariable<T>(variableName);           \
        if (variable != nullptr)                                               \
        {                                                                      \
            SetVariableNextStepDataCommon(*variable);                          \
        }                                                                      \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
}


void BP4Deserializer::ParsePGIndexPerStep(const BufferSTL &bufferSTL, const core::IO &io, size_t step)
{
    const auto &buffer = bufferSTL.m_Buffer;
    size_t position = m_MetadataIndexTable[step][0];

    std::cout << step << ", " << position << std::endl;

    m_MetadataSet.DataPGCount = m_MetadataSet.DataPGCount+helper::ReadValue<uint64_t>(buffer, position);
    //const size_t length = ReadValue<uint64_t>(buffer, position);
    //std::cout << m_MetadataSet.DataPGCount << ", " << length << std::endl;

    //size_t localPosition = 0;

    //std::unordered_set<uint32_t> stepsFound;
    //m_MetadataSet.StepsCount = 0;

    /*
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
    */
    ProcessGroupIndex index = ReadProcessGroupIndexHeader(buffer, position);
    if (index.IsColumnMajor == 'y')
    {
        m_IsRowMajor = false;
    }
    if (m_IsRowMajor != helper::IsRowMajor(io.m_HostLanguage))
    {
        m_ReverseDimensions = true;
    }
}


void BP4Deserializer::ParseVariablesIndexPerStep(const BufferSTL &bufferSTL, core::IO &io, size_t step)
{
    auto lf_ReadElementIndex = [&](core::IO &io, const std::vector<char> &buffer,
                                   size_t position, size_t step) {

        const ElementIndexHeader header =
            ReadElementIndexHeader(buffer, position);

        switch (header.DataType)
        {

        case (type_string):
        {
            //DefineVariableInIO<std::string>(header, io, buffer, position);
            DefineVariableInIOPerStep<std::string>(header, io, buffer, position, step);
            break;
        }

        case (type_byte):
        {
            //DefineVariableInIO<signed char>(header, io, buffer, position);
            DefineVariableInIOPerStep<signed char>(header, io, buffer, position, step);
            break;
        }

        case (type_short):
        {
            //DefineVariableInIO<short>(header, io, buffer, position);
            DefineVariableInIOPerStep<short>(header, io, buffer, position, step);
            break;
        }

        case (type_integer):
        {
            //DefineVariableInIO<int>(header, io, buffer, position);
            DefineVariableInIOPerStep<int>(header, io, buffer, position, step);
            break;
        }

        case (type_long):
        {
            //DefineVariableInIO<int64_t>(header, io, buffer, position);
            DefineVariableInIOPerStep<int64_t>(header, io, buffer, position, step);
            break;
        }

        case (type_unsigned_byte):
        {
            //DefineVariableInIO<unsigned char>(header, io, buffer, position);
            DefineVariableInIOPerStep<unsigned char>(header, io, buffer, position, step);
            break;
        }

        case (type_unsigned_short):
        {
            //DefineVariableInIO<unsigned short>(header, io, buffer, position);
            DefineVariableInIOPerStep<unsigned short>(header, io, buffer, position, step);
            break;
        }

        case (type_unsigned_integer):
        {
            //DefineVariableInIO<unsigned int>(header, io, buffer, position);
            DefineVariableInIOPerStep<unsigned int>(header, io, buffer, position, step);
            break;
        }

        case (type_unsigned_long):
        {
            //DefineVariableInIO<uint64_t>(header, io, buffer, position);
            DefineVariableInIOPerStep<uint64_t>(header, io, buffer, position, step);
            break;
        }

        case (type_real):
        {
            //DefineVariableInIO<float>(header, io, buffer, position);
            DefineVariableInIOPerStep<float>(header, io, buffer, position, step);
            break;
        }

        case (type_double):
        {
            //DefineVariableInIO<double>(header, io, buffer, position);
            DefineVariableInIOPerStep<double>(header, io, buffer, position, step);
            break;
        }

        case (type_long_double):
        {
            //DefineVariableInIO<long double>(header, io, buffer, position);
            DefineVariableInIOPerStep<long double>(header, io, buffer, position, step);
            break;
        }

        case (type_complex):
        {
            //DefineVariableInIO<std::complex<float>>(header, io, buffer,
            //                                        position);
            DefineVariableInIOPerStep<std::complex<float>>(header, io, buffer,
                                                    position, step);
            break;
        }

        case (type_double_complex):
        {
            //DefineVariableInIO<std::complex<double>>(header, io, buffer,
            //                                         position);
            DefineVariableInIOPerStep<std::complex<double>>(header, io, buffer,
                                                     position, step);
            break;
        }

        case (type_long_double_complex):
        {
            //DefineVariableInIO<std::complex<long double>>(header, io, buffer,
            //                                              position);
            DefineVariableInIOPerStep<std::complex<long double>>(header, io, buffer,
                                                          position, step);
            break;
        }

        } // end switch
    };

    const auto &buffer = bufferSTL.m_Buffer;
    size_t position = m_MetadataIndexTable[step][1];

    const uint32_t count = helper::ReadValue<uint32_t>(buffer, position);
    const uint64_t length = helper::ReadValue<uint64_t>(buffer, position);

    const size_t startPosition = position;
    size_t localPosition = 0;


    while (localPosition < length)
    {
        lf_ReadElementIndex(io, buffer, position, step);

        const size_t elementIndexSize =
                static_cast<size_t>(helper::ReadValue<uint32_t>(buffer, position));
        position += elementIndexSize;
        localPosition = position - startPosition;
    }
    return;
}

void BP4Deserializer::ParseAttributesIndexPerStep(const BufferSTL &bufferSTL, core::IO &io, size_t step)
{
    auto lf_ReadElementIndex = [&](core::IO &io, const std::vector<char> &buffer,
                                   size_t position) {

        const ElementIndexHeader header =
            ReadElementIndexHeader(buffer, position);

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
    size_t position = m_MetadataIndexTable[step][2];

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
}


std::map<std::string, helper::SubFileInfoMap>
BP4Deserializer::PerformGetsVariablesSubFileInfo(core::IO &io)
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
    template std::map<std::string, helper::SubFileInfoMap>                             \
    BP4Deserializer::GetSyncVariableSubFileInfo(const core::Variable<T> &variable)   \
        const;                                                                 \
                                                                               \
    template void BP4Deserializer::GetSyncVariableDataFromStream(              \
        core::Variable<T> &variable, BufferSTL &bufferSTL) const;                    \
                                                                               \
    template void BP4Deserializer::GetDeferredVariable(core::Variable<T> &variable,  \
                                                       T *data);               \
                                                                               \
    template void BP4Deserializer::GetValueFromMetadata(core::Variable<T> &variable) \
        const;

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace format
} // end namespace adios2
