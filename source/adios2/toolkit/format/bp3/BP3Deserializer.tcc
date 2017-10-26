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
            // TODO: map to sizeof(T)?
            info.Seeks.first =
                LinearIndex(blockDimensions, info.IntersectionBox.first,
                            m_IsRowMajor, m_IsZeroIndex) *
                sizeof(T);

            info.Seeks.second =
                LinearIndex(blockDimensions, info.IntersectionBox.second,
                            m_IsRowMajor, m_IsZeroIndex) *
                sizeof(T);

            const size_t fileIndex = static_cast<const size_t>(
                blockCharacteristics.Statistics.FileIndex);

            infoMap[fileIndex][step].push_back(info);
        }
    }

    return infoMap;
}

template <class T>
void BP3Deserializer::ClipContiguousMemoryCommon(
    Variable<T> &variable, const std::vector<char> &contiguousMemory,
    const Box<Dims> &intersectionBox)
{
    const Dims &start = intersectionBox.first;
    if (start.size() == 1) // 1D copy memory
    {
        // normalize intersection start with variable.m_Start
        const size_t normalizedStart =
            (start[0] - variable.m_Start[0]) * sizeof(T);
        char *rawVariableData = reinterpret_cast<char *>(variable.GetData());

        std::copy(contiguousMemory.begin(), contiguousMemory.end(),
                  &rawVariableData[normalizedStart]);

        return;
    }

    if (m_IsRowMajor && m_IsZeroIndex)
    {
        ClipContiguousMemoryCommonRowZero(variable, contiguousMemory,
                                          intersectionBox);
    }
}

template <class T>
void BP3Deserializer::ClipContiguousMemoryCommonRowZero(
    Variable<T> &variable, const std::vector<char> &contiguousMemory,
    const Box<Dims> &intersectionBox)
{
    const Dims &start = intersectionBox.first;
    const Dims &end = intersectionBox.second;
    const size_t dimensions = start.size();
    const size_t stride = end[dimensions - 1] - start[dimensions - 1];

    Dims currentPoint(start); // current point for memory copy

    const Box<Dims> variableSelection = variable.CurrentBoxSelection();

    bool run = true;

    while (run)
    {
        // here copy current linear memory between currentPoint and end
        const size_t contiguousStart =
            LinearIndex(intersectionBox, currentPoint, true, true);

        const size_t variableStart =
            LinearIndex(variableSelection, currentPoint, true, true);

        char *rawVariableData = reinterpret_cast<char *>(variable.GetData());

        std::copy(&contiguousMemory[contiguousStart],
                  &contiguousMemory[contiguousStart + stride],
                  &rawVariableData[variableStart]);

        // here update each index recursively, always starting from the 2nd
        // fastest changing index, since fastest changing index is the
        // continuous part in the previous std::copy
        size_t p = dimensions - 2;
        while (true)
        {
            ++currentPoint[p];

            if (currentPoint[p] == end[p])
            {
                if (p == 0)
                {
                    run = false; // we are done
                }
                else
                {
                    --p;
                }
            }
            else
            {
                break; // break inner p loop
            }
        } // dimension index update
    }     // run
}

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BP1_BP3DESERIALIZER_TCC_ */
