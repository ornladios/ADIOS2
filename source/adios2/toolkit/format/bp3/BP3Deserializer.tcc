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
        variable = &io.DefineVariable<T>(
            variableName,
            NewVectorType<uint64_t, size_t>(characteristics.ShapeU64),
            NewVectorType<uint64_t, size_t>(characteristics.StartU64),
            NewVectorType<uint64_t, size_t>(characteristics.CountU64));
    }

    // going back to get variable index position
    variable->m_IndexPosition =
        initialPosition - (header.Name.size() + header.GroupName.size() +
                           header.Path.size() + 23);

    const size_t endPosition =
        variable->m_IndexPosition + static_cast<size_t>(header.Length) + 4;

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
            variable->m_IndexStepSubsetPositions.push_back(subsetPositions);
            ++variable->m_AvailableStepsCount;
            subsetPositions.clear();
        }

        subsetPositions.push_back(subsetPosition);
        position = subsetPosition + subsetCharacteristics.EntryLength + 5;

        if (position == endPosition) // check if last one
        {
            variable->m_IndexStepSubsetPositions.push_back(subsetPositions);
            break;
        }
    }
}

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BP1_BP3DESERIALIZER_TCC_ */
