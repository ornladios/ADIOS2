/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * InSituMPISchedules.cpp
 * common functions for in situ MPI Writer and Reader
 * It requires MPI
 *
 *  Created on: Dec 25, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#include "InSituMPISchedules.h"

#include "adios2/helper/adiosMemory.h"

namespace adios2
{

namespace insitumpi
{

std::vector<std::vector<char>> SerializeLocalReadSchedule(
    const int nWriters,
    const std::map<std::string, SubFileInfoMap> &variablesSubFileInfo)
{
    std::vector<std::vector<char>> buffers(nWriters);

    // Create a buffer for each writer
    std::vector<int> nVarPerWriter(nWriters);
    for (size_t i = 0; i < nWriters; i++)
    {
        nVarPerWriter[i] = 0;
        InsertToBuffer(buffers[i], &nVarPerWriter[i],
                       1); // allocate first 4 bytes
    }

    for (const auto &variableNamePair : variablesSubFileInfo)
    {
        const std::string variableName(variableNamePair.first);
        // <writer, <steps, SubFileInfo>>
        for (const auto &subFileIndexPair : variableNamePair.second)
        {
            const size_t subFileIndex = subFileIndexPair.first; // writer
            auto &lrs = buffers[subFileIndex];
            // <steps, SubFileInfo>  but there is only one step
            for (const auto &stepPair : subFileIndexPair.second)
            {
                // LocalReadSchedule sfi = subFileIndexPair.second[0];
                const std::vector<SubFileInfo> &sfi = stepPair.second;
                SerializeLocalReadSchedule(buffers[subFileIndex], variableName,
                                           sfi);
                ++nVarPerWriter[subFileIndex];
                break; // there is only one step here
            }
        }
    }

    // Record the number of variables for each buffer
    for (size_t i = 0; i < nWriters; i++)
    {
        size_t pos = 0;
        CopyToBuffer(buffers[i], pos, &nVarPerWriter[i]);
    }
    return buffers;
}

void SerializeLocalReadSchedule(std::vector<char> &buffer,
                                const std::string varName,
                                const LocalReadSchedule lrs)
{
    const int nameLen = varName.size();
    InsertToBuffer(buffer, &nameLen, 1);
    InsertToBuffer(buffer, varName.data(), nameLen);
    const int nSubFileInfos = lrs.size();
    InsertToBuffer(buffer, &nSubFileInfos, 1);
    for (const auto &blockInfo : lrs)
    {
        SerializeSubFileInfo(buffer, blockInfo);
    }
}

void SerializeSubFileInfo(std::vector<char> &buffer, const SubFileInfo record)
{
    SerializeBox(buffer, record.BlockBox);
    SerializeBox(buffer, record.BlockBox);
    SerializeBox(buffer, record.Seeks);
}

void SerializeBox(std::vector<char> &buffer, const Box<Dims> box)
{
    const int nDims = box.first.size();
    InsertToBuffer(buffer, &nDims, 1);
    InsertToBuffer(buffer, box.first.data(), nDims);
    InsertToBuffer(buffer, box.second.data(), nDims);
}

void SerializeBox(std::vector<char> &buffer, const Box<size_t> box)
{
    InsertToBuffer(buffer, &box.first, 1);
    InsertToBuffer(buffer, &box.second, 1);
}

ReadScheduleMap
DeserializeReadSchedule(const std::vector<std::vector<char>> &buffers)
{
    ReadScheduleMap map;

    for (int i = 0; i < buffers.size(); i++)
    {
        const auto &buffer = buffers[i];
        LocalReadScheduleMap lrsm = DeserializeReadSchedule(buffer);
        for (const auto &varSchedule : lrsm)
        {
            map[varSchedule.first][i] = varSchedule.second;
        }
    }
    return map;
}

LocalReadScheduleMap DeserializeReadSchedule(const std::vector<char> &buffer)
{
    LocalReadScheduleMap map;
    return map;
}

} // end namespace insitumpi

} // end namespace adios2
