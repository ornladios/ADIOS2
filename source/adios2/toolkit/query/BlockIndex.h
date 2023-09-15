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
        // blockIndex typename Variable<T>::BPInfo& m_BlockInfo;
        //
        std::vector<typename adios2::core::Variable<T>::BPInfo> m_SubBlockInfo;
    };

public:
    BlockIndex<T>(adios2::core::Variable<T> *var, adios2::core::IO &io,
                  adios2::core::Engine &reader)
    : m_VarPtr(var), m_IdxIO(io), m_IdxReader(reader)
    {
    }

    void Generate(std::string &fromBPFile, const adios2::Params &inputs) {}

    void Evaluate(const QueryVar &query, std::vector<adios2::Box<adios2::Dims>> &resultSubBlocks)
    {
        if (m_IdxReader.m_EngineType.find("5") != std::string::npos) // a bp5 reader
            RunBP5Stat(query, resultSubBlocks);
        else
            RunBP4Stat(query, resultSubBlocks);
    }

    void RunBP5Stat(const QueryVar &query, std::vector<adios2::Box<adios2::Dims>> &hitBlocks)
    {
        size_t currStep = m_IdxReader.CurrentStep();
        adios2::Dims currShape = m_VarPtr->Shape();
        if (!query.IsSelectionValid(currShape))
            return;

        auto MinBlocksInfo = m_IdxReader.MinBlocksInfo(*m_VarPtr, currStep);
        if (!MinBlocksInfo)
        { // no info, can't do anything
            return;
        }
        for (auto &blockInfo : MinBlocksInfo->BlocksInfo)
        {
            Dims ss(MinBlocksInfo->Dims);
            Dims cc(MinBlocksInfo->Dims);
            for (auto i = 0; i < ss.size(); i++)
            {
                ss[i] = blockInfo.Start[i];
                cc[i] = blockInfo.Count[i];
            }
            if (!query.TouchSelection(ss, cc))
                continue;

            T bmin = *(T *)&blockInfo.MinMax.MinUnion;
            T bmax = *(T *)&blockInfo.MinMax.MaxUnion;
            bool isHit = query.m_RangeTree.CheckInterval(bmin, bmax);
            if (isHit)
            {
                adios2::Box<adios2::Dims> box = {ss, cc};
                hitBlocks.push_back(box);
            }
        }
        delete MinBlocksInfo;
    }

    void RunBP4Stat(const QueryVar &query, std::vector<adios2::Box<adios2::Dims>> &hitBlocks)
    {
        size_t currStep = m_IdxReader.CurrentStep();
        adios2::Dims currShape = m_VarPtr->Shape();
        if (!query.IsSelectionValid(currShape))
            return;

        std::vector<typename adios2::core::Variable<T>::BPInfo> varBlocksInfo =
            m_IdxReader.BlocksInfo(*m_VarPtr, currStep);

        for (auto &blockInfo : varBlocksInfo)
        {
            if (!query.TouchSelection(blockInfo.Start, blockInfo.Count))
                continue;

            if (blockInfo.MinMaxs.size() > 0)
            {
                adios2::helper::CalculateSubblockInfo(blockInfo.Count, blockInfo.SubBlockInfo);
                unsigned int numSubBlocks = static_cast<unsigned int>(blockInfo.MinMaxs.size() / 2);
                for (unsigned int i = 0; i < numSubBlocks; i++)
                {
                    bool isHit = query.m_RangeTree.CheckInterval(blockInfo.MinMaxs[2 * i],
                                                                 blockInfo.MinMaxs[2 * i + 1]);
                    if (isHit)
                    {
                        adios2::Box<adios2::Dims> currSubBlock =
                            adios2::helper::GetSubBlock(blockInfo.Count, blockInfo.SubBlockInfo, i);
                        if (!query.TouchSelection(currSubBlock.first, currSubBlock.second))
                            continue;
                        hitBlocks.push_back(currSubBlock);
                    }
                }
            }
            else
            { // default
                bool isHit = query.m_RangeTree.CheckInterval(blockInfo.Min, blockInfo.Max);
                if (isHit)
                {
                    adios2::Box<adios2::Dims> box = {blockInfo.Start, blockInfo.Count};
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

        std::vector<typename adios2::core::Variable<T>::BPInfo> varBlocksInfo =
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

    // can not be unique_ptr as it changes with bp5 through steps
    // as BP5Deserializer::SetupForStep calls io.RemoveVariables()
    // must use ptr as bp5 associates ptrs with blockinfo, see MinBlocksInfo() in bp5
    adios2::core::Variable<T> *m_VarPtr;

private:
    //
    // blockid <=> vector of subcontents
    //
    // std::string& m_DataFileName;
    adios2::core::IO &m_IdxIO;
    adios2::core::Engine &m_IdxReader;

}; // class blockIndex

}; // end namespace query
}; // end namespace adios2

#endif
