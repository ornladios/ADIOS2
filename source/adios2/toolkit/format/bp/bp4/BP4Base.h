/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4Base.h  base class for BP4Serializer and BP4Deserializer
 *
 *  Created on: Aug 1, 2018
 *  Author: William F Godoy godoywf@ornl.gov
 *          Lipeng Wan wanl@ornl.gov
 *          Norbert Podhorszki pnb@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP_BP4_BP4BASE_H_
#define ADIOS2_TOOLKIT_FORMAT_BP_BP4_BP4BASE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <bitset>
#include <map>
#include <memory> //std::shared_ptr
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
/// \endcond

#include "adios2/common/ADIOSConfig.h"
#include "adios2/common/ADIOSMacros.h"
#include "adios2/common/ADIOSTypes.h"
#include "adios2/core/Engine.h"
#include "adios2/core/VariableBase.h"
#include "adios2/helper/adiosComm.h"
#include "adios2/toolkit/aggregator/mpi/MPIChain.h"
#include "adios2/toolkit/format/bp/BPBase.h"
#include "adios2/toolkit/format/bp/bpOperation/BPOperation.h"
#include "adios2/toolkit/format/buffer/heap/BufferSTL.h"
#include "adios2/toolkit/profiling/iochrono/IOChrono.h"

namespace adios2
{
namespace format
{

/**
 * Base class for BP1Writer and BP1Reader format
 */
class BP4Base : virtual public BPBase
{

public:
    /* metadata index table*/
    std::unordered_map<uint64_t,
                       std::unordered_map<uint64_t, std::vector<uint64_t>>>
        m_MetadataIndexTable;

    BufferSTL m_MetadataIndex;

    /** Positions of flags in Index Table Header that Reader uses */
    static constexpr size_t m_EndianFlagPosition = 36;
    static constexpr size_t m_BPVersionPosition = 37;
    static constexpr size_t m_ActiveFlagPosition = 38;
    static constexpr size_t m_VersionTagPosition = 0;
    static constexpr size_t m_VersionTagLength = 32;

    /**
     * Unique constructor
     * @param mpiComm for m_BP1Aggregator
     * @param debugMode true: exceptions checks
     */
    BP4Base(helper::Comm const &comm, const bool debugMode);

    virtual ~BP4Base() = default;

    /** A BP4 dataset is a directory. Remove first the trailing \ from
     * the name if present. Makes it easier to append all file names
     * to it later.
     */
    std::string RemoveTrailingSlash(const std::string &name) const noexcept;

    /**
     * Vector version of BPBaseNames
     * @param names
     * @return vector of base (name.bp) names
     */
    std::vector<std::string>
    GetBPBaseNames(const std::vector<std::string> &names) const noexcept;

    /**
     * Get BP substream names from base names:
     * /path/name.bp.dir/name.bp.Index
     * where Index = Rank, and in aggregation = SubStreamID
     * @param baseNames inputs
     * @return vector of BP substream names for transports
     */
    std::vector<std::string>
    GetBPSubStreamNames(const std::vector<std::string> &baseNames) const
        noexcept;

    std::vector<std::string>
    GetBPMetadataFileNames(const std::vector<std::string> &names) const
        noexcept;

    std::string GetBPMetadataFileName(const std::string &name) const noexcept;

    std::vector<std::string>
    GetBPMetadataIndexFileNames(const std::vector<std::string> &names) const
        noexcept;

    std::string GetBPMetadataIndexFileName(const std::string &name) const
        noexcept;

    std::string GetBPSubFileName(const std::string &name,
                                 const size_t subFileIndex,
                                 const bool hasSubFiles = true,
                                 const bool isReader = false) const noexcept;

    /**
     * Returns the estimated variable index size. Used by ResizeBuffer public
     * function
     * @param variableName input
     * @param count input variable local dimensions
     */
    size_t GetBPIndexSizeInData(const std::string &variableName,
                                const Dims &count) const noexcept;

protected:
    /* The position of the length of the serialized variable (metadata + data
     * length) in the output data buffer (m_Data). This position must be passed
     * between PutVariableMetadata() and PutVariablePayload() of the serializer
     * to correctly record the length of the serialized length. The length
     * depends on whether an operation is applied at buffering in
     * PutVariablePayload(). This is a temporary, short lived value between the
     * calls of those functions, call from an engine.
     */
    size_t m_LastVarLengthPosInBuffer = 0;

    ElementIndexHeader
    ReadElementIndexHeader(const std::vector<char> &buffer, size_t &position,
                           const bool isLittleEndian = true) const
        noexcept final;

private:
    std::string GetBPSubStreamName(const std::string &name, const size_t id,
                                   const bool hasSubFiles = true,
                                   const bool isReader = false) const noexcept;
};

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BP_BP4_BP4BASE_H_ */
