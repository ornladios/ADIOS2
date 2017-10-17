/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP3Deserializer.tcc
 *
 *  Created on: Sep 7, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP1_BP3DESERIALIZER_TCC_
#define ADIOS2_TOOLKIT_FORMAT_BP1_BP3DESERIALIZER_TCC_

#include "BP3Deserializer.h"

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace format
{

template <class T>
std::map<std::string, SubFileInfoMap>
BP3Deserializer::GetSyncVariableSubFileInfo(const Variable<T> &variable) const
{
    std::map<std::string, SubFileInfoMap> variableSubFileInfo;
    variableSubFileInfo[variable.m_Name] = GetSubFileInfo(variable);
    return variableSubFileInfo;
}

template <class T>
void BP3Deserializer::GetDeferredVariable(Variable<T> &variable, T *data)
{
    variable.SetData(data);
    m_DeferredVariables[variable.m_Name] = SubFileInfoMap();
}

// PRIVATE
template <class T>
void BP3Deserializer::DefineVariableInIO(const ElementIndexHeader &header,
                                         IO &io,
                                         const std::vector<char> &buffer,
                                         size_t position) const
{
    const size_t initialPosition = position;

    const Characteristics<T> characteristics =
        ReadElementIndexCharacteristics<T>(buffer, position);

    std::string variableName(header.Name);
    if (!header.Path.empty())
    {
        variableName = header.Path + PathSeparator + header.Name;
    }

    Variable<T> *variable = nullptr;
    {
        // std::mutex portion
        std::lock_guard<std::mutex> lock(m_Mutex);
        variable =
            &io.DefineVariable<T>(variableName, characteristics.Shape,
                                  characteristics.Start, characteristics.Count);
    }

    // going back to get variable index position
    variable->m_IndexStart =
        initialPosition - (header.Name.size() + header.GroupName.size() +
                           header.Path.size() + 23);

    const size_t endPosition =
        variable->m_IndexStart + static_cast<size_t>(header.Length) + 4;

    position = initialPosition;

    size_t currentStep = 1;

    std::vector<size_t> subsetPositions; // per step
    subsetPositions.reserve(1);          // expecting one subset per step

    while (position < endPosition)
    {
        const size_t subsetPosition = position;

        // read until step is found
        const Characteristics<T> subsetCharacteristics =
            ReadElementIndexCharacteristics<T>(buffer, position, true);

        if (subsetCharacteristics.Statistics.Step > currentStep)
        {
            currentStep = subsetCharacteristics.Statistics.Step;
            variable->m_IndexStepBlockStarts[currentStep] = subsetPositions;
            ++variable->m_AvailableStepsCount;
            subsetPositions.clear();
        }

        subsetPositions.push_back(subsetPosition);
        position = subsetPosition + subsetCharacteristics.EntryLength + 5;

        if (position == endPosition) // check if last one
        {
            variable->m_IndexStepBlockStarts[currentStep] = subsetPositions;
            break;
        }
    }
}

template <class T>
SubFileInfoMap
BP3Deserializer::GetSubFileInfo(const Variable<T> &variable) const
{
    SubFileInfoMap infoMap;

    const auto &buffer = m_Metadata.m_Buffer;

    const size_t stepStart = variable.m_StepStart;
    const size_t stepEnd = stepStart + variable.m_StepCount;

    // selection, start and count
    const Box<Dims> selection{variable.m_Start, variable.m_Count};

    for (size_t step = stepStart; step < stepEnd; ++step)
    {
        auto itBlockStarts = variable.m_IndexStepBlockStarts.find(step);
        if (itBlockStarts == variable.m_IndexStepBlockStarts.end())
        {
            continue;
        }

        const std::vector<size_t> &blockStarts = itBlockStarts->second;

        // blockPosition gets updated by Read, can't be const
        for (size_t blockPosition : blockStarts)
        {
            const Characteristics<T> blockCharacteristics =
                ReadElementIndexCharacteristics<T>(buffer, blockPosition);

            const Box<Dims> blockDimensions{blockCharacteristics.Start,
                                            blockCharacteristics.Count};

            // check if they intersect
            SubFileInfo info;
            info.IntersectionBox = IntersectionBox(selection, blockDimensions);

            if (info.IntersectionBox.first.empty() ||
                info.IntersectionBox.second.empty())
            {
                continue;
            }
            // if they intersect get info Seeks (first: start, second: end)
            // TODO: get row-major, zero-index for each language
            info.Seeks.first =
                LinearIndex(blockDimensions, info.IntersectionBox.first,
                            m_IsRowMajor, m_IsZeroIndex);

            info.Seeks.second =
                LinearIndex(blockDimensions, info.IntersectionBox.second,
                            m_IsRowMajor, m_IsZeroIndex);

            const size_t fileIndex = static_cast<unsigned int>(
                blockCharacteristics.Statistics.FileIndex);

            infoMap[fileIndex][step].push_back(info);
        }
    }

    return infoMap;
}

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BP1_BP3DESERIALIZER_TCC_ */
