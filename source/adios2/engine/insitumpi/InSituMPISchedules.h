/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * InSituMPISchedules.h
 * common functions for in situ MPI Writer and Reader
 * It requires MPI
 *
 *  Created on: Dec 25, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#ifndef ADIOS2_ENGINE_INSITUMPISCHEDULES_H_
#define ADIOS2_ENGINE_INSITUMPISCHEDULES_H_

#include "adios2/helper/adiosType.h"

#include <map>
#include <vector>

namespace adios2
{

namespace insitumpi
{

// Recalculate in each SubFileInfo the Seek parameters as
// if the payload offset was always 0
void FixSeeksToZeroOffset(
    std::map<std::string, SubFileInfoMap> &variablesSubFileInfo) noexcept;
void FixSeeksToZeroOffset(SubFileInfo &record) noexcept;

// Serialize the read requests on a reader.
// Creates a separate buffer for each writer.
// Each buffer's format is the following:
// int N       : number of variables
// for each variable
//     for each rank separately have one buffer[rank]
//         there is one step, lrs is the vector of SubFileInfos
//         SerializeLocalReadSchedule (variable, lrs)
std::vector<std::vector<char>> SerializeLocalReadSchedule(
    const int nWriters,
    const std::map<std::string, SubFileInfoMap> &variablesSubFileInfo) noexcept;

// per-variable per rank schedule
using LocalReadSchedule = std::vector<SubFileInfo>;

// Serialize one variable's read schedule into one writer's buffer
//     int L   : length of variable name (without 0)
//     char[L] : variable name
//     int M   : number of SubFileInfo records
//     for each SubFileInfo
//         serialize SubFileInfo
void SerializeLocalReadSchedule(std::vector<char> &buffer,
                                const std::string varName,
                                const LocalReadSchedule lrs) noexcept;

// Box<Dims> BlockBox;
// Box<Dims> IntersectionBox; ///< first = Start point, second = End point
// Box<size_t> Seeks;
void SerializeSubFileInfo(std::vector<char> &buffer,
                          const SubFileInfo record) noexcept;

void SerializeBox(std::vector<char> &buffer, const Box<Dims> box) noexcept;
void SerializeBox(std::vector<char> &buffer, const Box<size_t> box) noexcept;

// Read schedule map on a writer:"
// For all variables
//    for all readers that requested it
//       the blocks
using WriteScheduleMap =
    std::map<std::string, std::map<size_t, std::vector<SubFileInfo>>>;

// One readers' schedule for all variables on a writer
using LocalReadScheduleMap = std::map<std::string, std::vector<SubFileInfo>>;

// Deserialize buffers from all readers
WriteScheduleMap
DeserializeReadSchedule(const std::vector<std::vector<char>> &buffers) noexcept;

// Deserialize buffer from one reader
LocalReadScheduleMap
DeserializeReadSchedule(const std::vector<char> &buffer) noexcept;

// Deserialize one variable from one reader
// LocalReadSchedule DeserializeReadSchedule(const std::vector<char> &buffer);

SubFileInfo DeserializeSubFileInfo(const std::vector<char> &buffer,
                                   size_t &position) noexcept;
Box<Dims> DeserializeBoxDims(const std::vector<char> &buffer,
                             size_t &position) noexcept;
Box<size_t> DeserializeBoxSizet(const std::vector<char> &buffer,
                                size_t &position) noexcept;

void PrintReadScheduleMap(const WriteScheduleMap &map) noexcept;
void PrintSubFileInfo(const SubFileInfo &sfi) noexcept;
void PrintBox(const Box<Dims> &box) noexcept;
void PrintBox(const Box<size_t> &box) noexcept;
void PrintDims(const Dims &dims) noexcept;

} // end namespace insitumpi

} // end namespace adios2

#endif /* ADIOS2_ENGINE_INSITUMPISCHEDULES_H_ */
