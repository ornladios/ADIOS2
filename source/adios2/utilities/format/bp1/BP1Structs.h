/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP1Structs.h
 *
 *  Created on: Apr 3, 2017
 *      Author: wfg
 */

#ifndef ADIOS2_UTILITIES_FORMAT_BP1_BP1STRUCTS_H_
#define ADIOS2_UTILITIES_FORMAT_BP1_BP1STRUCTS_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
/// \endcond

#include "adios2/ADIOSConfig.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/core/IOChrono.h"

namespace adios
{
namespace format
{

/**
 * Metadata index used for Variables and Attributes, needed in a container for
 * characteristic
 * sets merge independently for each Variable or Attribute
 */
struct BP1Index
{
    std::vector<char> Buffer; ///< metadata variable index, start with 100Kb
    /** number of characteristics sets (time and spatial aggregation) */
    uint64_t Count = 0;
    /** unique ID assigned to each variable for counter */
    const uint32_t MemberID;

    BP1Index(const uint32_t memberID) : MemberID(memberID)
    {
        Buffer.reserve(500);
    }
};

/**
 * Single struct that tracks metadata indices in bp format
 */
struct BP1MetadataSet
{
    /**
     * updated with advance step, if append it will be updated to last,
     * starts with one in ADIOS1
     */
    uint32_t TimeStep = 1;

    BP1Index PGIndex = BP1Index(0); ///< single buffer for PGIndex

    // no priority for now
    /** @brief key: variable name, value: bp metadata variable index */
    std::unordered_map<std::string, BP1Index> VarsIndices;

    /** @brief key: attribute name, value: bp metadata attribute index */
    std::unordered_map<std::string, BP1Index> AttributesIndices;

    const unsigned int MiniFooterSize = 28; ///< from bpls reader

    // PG (relative) positions in Data buffer, to be updated every advance step
    // or init
    /** number of current PGs */
    uint64_t DataPGCount = 0;
    /** current PG initial ( relative ) position in data buffer */
    size_t DataPGLengthPosition = 0;
    /** number of variables in current PG */
    uint32_t DataPGVarsCount = 0;
    /** current PG variable count ( relative ) position */
    size_t DataPGVarsCountPosition = 0;

    /** true: currently writing to a pg, false: no current pg */
    bool DataPGIsOpen = false;

    profiling::IOChrono Log; ///< object that takes buffering profiling info
};

} // end namespace format
} // end namespace adios

#endif /* ADIOS2_UTILITIES_FORMAT_BP1_BP1STRUCTS_H_ */
