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
        if (m_IdxReader.m_EngineType.find("BP4") >= 0)
        {
            RunBP4Stat(query, resultSubBlocks);
        }
        else if (m_IdxReader.m_EngineType.find("BP3") >= 0)
        {
            RunDefaultBPStat(query, resultSubBlocks);
        }
    }

    void RunBP4Stat(const QueryVar &query,
                    std::vector<adios2::Box<adios2::Dims>> &hitBlocks)
    {
        std::cout << "BlockIndexEval(BP4) using BP3 now" << std::endl;
        RunDefaultBPStat(query, hitBlocks);
    }

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
    /*
    void Init (adios2::Engine& idxReader, const adios2::Variable<T>& var, int
    ts); void SetContent(const QueryVar& query, int ts);

    bool isBlockValid(adios2::Dims& blockStart, adios2::Dims& blockCount,
                      adios2::Dims& selStart, adios2::Dims& selCount);

    void WalkThrough(const QueryVar& query, std::vector<unsigned int>& results);
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
