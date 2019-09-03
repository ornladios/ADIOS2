#ifndef ADIOS2_BLOCK_INDEX_H
#define ADIOS2_BLOCK_INDEX_H

#include "Index.h"
#include "Query.h"

namespace adios2
{
namespace query
{

template <class T>
class BlockIndex
{
    struct Tree
    {
        //
        // ** no need to keep the original block. might be smaller than
        // blockIndex typename Variable<T>::info& m_BlockInfo;
        //
        std::vector<typename adios2::core::Variable<T>::Info> m_SubBlockInfo;
    };

public:
    BlockIndex<T>(adios2::core::Variable<T> &var, adios2::core::IO &io,
                  adios2::core::Engine &reader)
    : m_Var(var), m_IdxIO(io), m_IdxReader(reader)
    {
    }

    void Generate(std::string &fromBPFile, const adios2::Params &inputs) {}

    void Evaluate(const QueryVar &query,
                  std::vector<adios2::Box<adios2::Dims>> &resultSubBlocks)
    {
        RunBP4Stat(query, resultSubBlocks);
    }

    void RunBP4Stat(const QueryVar &query,
                    std::vector<adios2::Box<adios2::Dims>> &hitBlocks)
    {
        size_t currStep = m_IdxReader.CurrentStep();
        adios2::Dims currShape = m_Var.Shape();
        if (!query.IsSelectionValid(currShape))
            return;

        std::vector<typename adios2::core::Variable<T>::Info> varBlocksInfo =
            m_IdxReader.BlocksInfo(m_Var, currStep);

        for (auto &blockInfo : varBlocksInfo)
        {
            if (!query.TouchSelection(blockInfo.Start, blockInfo.Count))
                continue;

            if (blockInfo.MinMaxs.size() > 0)
            {
                adios2::helper::CalculateSubblockInfo(blockInfo.Count,
                                                      blockInfo.SubBlockInfo);
                auto numSubBlocks = blockInfo.MinMaxs.size() / 2;
                for (auto i = 0; i < numSubBlocks; i++)
                {
                    bool isHit = query.m_RangeTree.CheckInterval(
                        blockInfo.MinMaxs[2 * i], blockInfo.MinMaxs[2 * i + 1]);
                    if (isHit)
                    {
                        adios2::Box<adios2::Dims> currSubBlock =
                            adios2::helper::GetSubBlock(
                                blockInfo.Count, blockInfo.SubBlockInfo, i);
                        if (!query.TouchSelection(currSubBlock.first,
                                                  currSubBlock.second))
                            continue;
                        hitBlocks.push_back(currSubBlock);
                    }
                }
            }
            else
            { // default
                bool isHit = query.m_RangeTree.CheckInterval(blockInfo.Min,
                                                             blockInfo.Max);
                if (isHit)
                {
                    adios2::Box<adios2::Dims> box = {blockInfo.Start,
                                                     blockInfo.Count};
                    hitBlocks.push_back(box);
                }
            }
        }
    }

    /*
    void RunDefaultBPStat(const QueryVar &query,
                          std::vector<adios2::Box<adios2::Dims>> &hitBlocks)
    {
        size_t currStep = m_IdxReader.CurrentStep();
        adios2::Dims currShape = m_Var.Shape();
        if (!query.IsSelectionValid(currShape))
            return;

        std::vector<typename adios2::core::Variable<T>::Info> varBlocksInfo =
            m_IdxReader.BlocksInfo(m_Var, currStep);

        for (auto &blockInfo : varBlocksInfo)
        {
            if (!query.TouchSelection(blockInfo.Start, blockInfo.Count))
                continue;

            T min = blockInfo.Min;
            T max = blockInfo.Max;

            // std::cout<<" min: "<<min<<"  max: "<<max<<std::endl;
            bool isHit = query.m_RangeTree.CheckInterval(min, max);
            if (isHit)
            {
                adios2::Box<adios2::Dims> box = {blockInfo.Start,
                                                 blockInfo.Count};
                hitBlocks.push_back(box);
            }
        }
    }
    */

    Tree m_Content;
    adios2::core::Variable<T> m_Var;

private:
    //
    // blockid <=> vector of subcontents
    //
    // std::string& m_DataFileName;
    adios2::core::Engine &m_IdxReader;
    adios2::core::IO &m_IdxIO;

}; // class blockIndex

}; // end namespace query
}; // end namespace adios2

#endif
