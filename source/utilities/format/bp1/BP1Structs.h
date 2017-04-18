/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP1Structs.h
 *
 *  Created on: Apr 3, 2017
 *      Author: wfg
 */

#ifndef BP1STRUCTS_H_
#define BP1STRUCTS_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
/// \endcond

#include "utilities/profiling/iochrono/IOChrono.h"

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
    std::uint64_t Count =
        0; ///< number of characteristics sets (time and spatial aggregation)
    const std::uint32_t MemberID;

    BP1Index(const std::uint32_t memberID) : MemberID{memberID}
    {
        Buffer.reserve(500);
    }
};

/**
 * Single struct that tracks metadata indices in bp format
 */
struct BP1MetadataSet
{
    std::uint32_t
        TimeStep; ///< current time step, updated with advance step, if
    /// append it will be updated to last, starts with one in ADIOS1

    BP1Index PGIndex = BP1Index(0); ///< single buffer for PGIndex

    // no priority for now
    /** @brief key: variable name, value: bp metadata variable index */
    std::unordered_map<std::string, BP1Index> VarsIndices;

    std::unordered_map<std::string, BP1Index>
        AttributesIndices; ///< key: name, value: attribute bp index

    const unsigned int MiniFooterSize = 28; ///< from bpls reader

    // PG (relative) positions in Data buffer to be updated
    std::uint64_t DataPGCount = 0;
    std::size_t DataPGLengthPosition = 0; ///< current PG initial ( relative )
                                          /// position, needs to be updated in
    /// every advance step or init
    std::uint32_t DataPGVarsCount = 0; ///< variables in current PG

    /**
     * current PG variable count ( relative ) position, needs to be
     * updated in very Advance
     */
    std::size_t DataPGVarsCountPosition = 0;

    bool DataPGIsOpen = false;

    profiling::IOChrono Log; ///< object that takes buffering profiling info
};

} // end namespace format
} // end namespace adios

#endif /* BP1STRUCTS_H_ */
