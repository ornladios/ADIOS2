/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP5Helper.h
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP5_BP5HELPER_H_
#define ADIOS2_TOOLKIT_FORMAT_BP5_BP5HELPER_H_

#include "BP5Base.h"
#include "adios2/core/Attribute.h"
#include "adios2/core/CoreTypes.h"
#include "adios2/toolkit/profiling/iochrono/IOChrono.h"
#ifdef _WIN32
#pragma warning(disable : 4250)
#endif
#include <array>
#include <iomanip>
#include <iostream>

namespace adios2
{
namespace format
{

class BP5Helper : virtual public BP5Base
{
public:
    static void BP5AggregateInformation(helper::Comm &mpiComm,
                                        adios2::profiling::JSONProfiler &Profiler,
                                        std::vector<BP5Base::MetaMetaInfoBlock> &NewMetaMetaBlocks,
                                        std::vector<core::iovec> &AttributeEncodeBuffers,
                                        std::vector<size_t> &MetaEncodeSize,
                                        std::vector<uint64_t> &WriterDataPositions);

    static void GathervArraysTwoLevel(helper::Comm &groupComm, helper::Comm &groupLeaderComm,
                                      adios2::profiling::JSONProfiler &Profiler,
                                      uint64_t *MyContrib, size_t LocalSize,
                                      size_t *OverallRecvCounts, size_t OverallRecvCountsSize,
                                      uint64_t *OverallRecvBuffer, size_t DestRank);
    struct digest
    {
        uint64_t x[2] = {0, 0};
        // compare for order
        bool operator<(const digest &dg) const
        {
            if (x[0] != dg.x[0])
                return (x[0] < dg.x[0]);
            return (x[1] < dg.x[1]);
        };

        bool IsZero() { return ((x[0] == 0) && (x[1] == 0)); };
        friend std::ostream &operator<<(std::ostream &os, const digest &d)
        {
            std::cout << "0x" << std::setw(8) << std::setfill('0') << std::hex << d.x[0] << d.x[1];
            return os;
        };
    };

    static digest HashOfBlock(const void *block, const size_t block_len);

private:
#define FIXED_MMB_SLOT_COUNT 4
    struct node_contrib
    {
        digest AttrHash;
        size_t AttrSize;
        size_t MMBCount;
        digest MMBArray[FIXED_MMB_SLOT_COUNT];
        size_t MMBSizeArray[FIXED_MMB_SLOT_COUNT];
        size_t MetaEncodeSize;
        uint64_t WriterDataPosition;
    };
    static std::vector<char>
    BuildNodeContrib(const digest attrHash, size_t attrSize,
                     const std::vector<BP5Base::MetaMetaInfoBlock> NewMetaMetaBlocks,
                     const size_t MetaEncodeSize, const std::vector<uint64_t> WriterDataPositions);
    static std::vector<char>
    BuildFixedNodeContrib(const digest attrHash, size_t attrSize,
                          const std::vector<BP5Base::MetaMetaInfoBlock> NewMetaMetaBlocks,
                          const size_t MetaEncodeSize,
                          const std::vector<uint64_t> WriterDataPositions);
    static void
    BreakdownIncomingMInfo(const std::vector<size_t> RecvCounts, const std::vector<char> RecvBuffer,
                           std::vector<size_t> &SecondRecvCounts, std::vector<uint64_t> &BcastInfo,
                           std::vector<uint64_t> &WriterDataPositions,
                           std::vector<size_t> &MetaEncodeSize, std::vector<size_t> &AttrSizes,
                           std::vector<size_t> &MMBSizes, std::vector<digest> &MBBIDs);
    static void BreakdownFixedIncomingMInfo(
        const size_t NodeCount, const std::vector<char> RecvBuffer,
        std::vector<size_t> &SecondRecvCounts, std::vector<uint64_t> &BcastInfo,
        std::vector<uint64_t> &WriterDataPositions, std::vector<size_t> &MetaEncodeSize,
        std::vector<size_t> &AttrSizes, std::vector<size_t> &MMBSizes, std::vector<digest> &MBBIDs);
    static void BreakdownIncomingMData(const std::vector<size_t> &RecvCounts,
                                       std::vector<uint64_t> &BcastInfo,
                                       const std::vector<char> &IncomingMMA,
                                       std::vector<BP5Base::MetaMetaInfoBlock> &NewMetaMetaBlocks,
                                       std::vector<core::iovec> &AttributeEncodeBuffers,
                                       std::vector<size_t> AttrSize, std::vector<size_t> MMBSizes,
                                       std::vector<digest> MMBIDs);
};
} // end namespace format
} // end namespace adios2

#endif /*  ADIOS2_TOOLKIT_FORMAT_BP5_BP5HELPER_H_ */
