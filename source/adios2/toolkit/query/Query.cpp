#include "Query.h"
#include "BlockIndex.h"
#include "adios2/helper/adiosFunctions.h"

#include "Query.tcc"

namespace adios2
{
namespace query
{

adios2::query::Relation strToRelation(std::string relationStr) noexcept
{
    if ((relationStr.compare("or") == 0) || (relationStr.compare("OR") == 0))
        return adios2::query::Relation::OR;

    return adios2::query::Relation::AND; // default
}

adios2::query::Op strToQueryOp(std::string opStr) noexcept
{
    if ((opStr.compare("lt") == 0) || (opStr.compare("LT") == 0))
        return adios2::query::Op::LT;
    if ((opStr.compare("gt") == 0) || (opStr.compare("GT") == 0))
        return adios2::query::Op::GT;
    if ((opStr.compare("ge") == 0) || (opStr.compare("GE") == 0))
        return adios2::query::Op::GE;
    if ((opStr.compare("le") == 0) || (opStr.compare("LE") == 0))
        return adios2::query::Op::LE;
    if ((opStr.compare("eq") == 0) || (opStr.compare("EQ") == 0))
        return adios2::query::Op::EQ;
    if ((opStr.compare("ne") == 0) || (opStr.compare("NE") == 0))
        return adios2::query::Op::NE;

    return adios2::query::Op::EQ; // default
}

adios2::Dims split(const std::string &s, char delim)
{
    adios2::Dims dim;

    std::stringstream ss(s);
    std::string item;

    while (getline(ss, item, delim))
    {
        std::stringstream curr(item);
        size_t val;
        curr >> val;
        dim.push_back(val);
    }

    return dim;
}

void QueryBase::ApplyOutputRegion(std::vector<Box<Dims>> &touchedBlocks,
                                  const adios2::Box<Dims> &referenceRegion)
{
    if (m_OutputRegion.first.size() == 0)
        return;

    adios2::Dims diff;
    diff.resize(m_OutputRegion.first.size());
    bool isDifferent = false;
    for (size_t k = 0; k < m_OutputRegion.first.size(); k++)
    {
        diff[k] = m_OutputRegion.first[k] - referenceRegion.first[k];
        if (diff[k] != 0)
            isDifferent = true;
    }

    if (!isDifferent)
        return;

    // blocks are usually part of the reference region
    for (auto it = touchedBlocks.begin(); it != touchedBlocks.end(); it++)
    {
        for (size_t k = 0; k < m_OutputRegion.first.size(); k++)
            it->first[k] += diff[k];
    }
}
bool QueryComposite::AddNode(QueryBase *var)
{
    if (nullptr == var)
        return false;

    if (adios2::query::Relation::NOT == m_Relation)
    {
        // if (m_Nodes.size() > 0) return false;
        // don't want to support NOT for composite queries
        // return false;
        helper::Throw<std::ios_base::failure>(
            "Toolkit", "query::QueryComposite", "AddNode",
            "Currently NOT is not suppprted for composite query");
    }
    m_Nodes.push_back(var);
    return true;
}

void QueryComposite::BlockIndexEvaluate(adios2::core::IO &io,
                                        adios2::core::Engine &reader,
                                        std::vector<Box<Dims>> &touchedBlocks)
{
    auto lf_ApplyAND = [&](std::vector<Box<Dims>> &touched,
                           const std::vector<Box<Dims>> &curr) -> void {
        if (curr.size() == 0)
        {
            touched.clear();
            return;
        }

        for (auto i = touched.size(); i >= 1; i--)
        {
            bool intersects = false;
            for (auto b : curr)
            {
                adios2::Box<Dims> curr = GetIntersection(touched[i - 1], b);
                if (curr.first.size() != 0) // has intersection
                {
                    intersects = true;
                    break;
                }
            }
            if (!intersects)
                // it = touched.erase(it);
                touched.erase(touched.begin() + i - 1);
            // if (touched.end() == it)
            // break;
        }

        for (auto b : curr)
        {
            for (auto it = touched.begin(); it != touched.end(); it++)
            {
                adios2::Box<Dims> curr = GetIntersection(*it, b);
                if (curr.first.size() != 0) // has intersection
                {
                    *it = curr;
                }
            }
        }
    }; // lf_ApplyAND

    auto lf_ApplyOR = [&](std::vector<Box<Dims>> &touched,
                          const std::vector<Box<Dims>> &curr) -> void {
        if (curr.size() == 0)
            return;

        for (auto b : curr)
        {
            bool duplicated = false;
            for (auto box : touched)
            {
                if (adios2::helper::IdenticalBoxes(box, b))
                {
                    duplicated = true;
                    continue;
                }
            }
            if (!duplicated)
                touched.push_back(b);
        }
    }; // lf_ApplyOR

    /*
    auto lf_ApplyRelation = [&](std::vector<Box<Dims>> &collection,
                                const Box<Dims> &block) -> void {
        if (adios2::query::Relation::AND == m_Relation)
        {
            for (auto it = touchedBlocks.begin(); it != touchedBlocks.end();
                 it++)
            {
                adios2::Box<Dims> curr = GetIntersection(*it, block);
                // adios2::helper::IntersectionBox(*it, block);
                if (curr.first.size() == 0) // no intersection
                  {
                    it = touchedBlocks.erase(it);
                    if (touchedBlocks.end() == it)
                      return;
                  }
                else
                  *it = curr;
            }

            return;
        }

        if (adios2::query::Relation::OR == m_Relation)
        {
            for (auto box : touchedBlocks)
            {
                if (adios2::helper::IdenticalBoxes(box, block))
                    return;
            }
            touchedBlocks.push_back(block);
            return;
        }
    }; // local
    */

    if (m_Nodes.size() == 0)
        return;

    int counter = 0;
    for (auto node : m_Nodes)
    {
        counter++;
        std::vector<Box<Dims>> currBlocks;
        node->BlockIndexEvaluate(io, reader, currBlocks);
        if (counter == 1)
        {
            touchedBlocks = currBlocks;
            continue;
        }

        if (currBlocks.size() == 0)
        {
            if (adios2::query::Relation::AND == m_Relation)
            {
                touchedBlocks.clear();
                break;
            }
            else
                continue;
        }

        if (adios2::query::Relation::AND == m_Relation)
            lf_ApplyAND(touchedBlocks, currBlocks);
        else if (adios2::query::Relation::OR == m_Relation)
            lf_ApplyOR(touchedBlocks, currBlocks);
    }
    // plan to shift all var results to regions start at 0, and find out the
    // overlapped regions boxes can be different size especially if they are
    // from BP3
}

bool QueryVar::IsSelectionValid(adios2::Dims &shape) const
{
    if (0 == m_Selection.first.size())
        return true;

    if (m_Selection.first.size() != shape.size())
    {
        helper::Log(
            "Query", "QueryVar", "IsSelectionValid",
            "Query selection dimension is different from shape dimension",
            helper::FATALERROR);
        return false; // different dimension
    }

    /*
    for (size_t i = 0; i < shape.size(); i++)
    {
        if ((m_Selection.first[i] > shape[i]) ||
            (m_Selection.second[i] > shape[i]))
            return false;
    }
    */
    return true;
}

void QueryVar::LoadSelection(const std::string &startStr,
                             const std::string &countStr)
{
    adios2::Dims start = split(startStr, ',');
    adios2::Dims count = split(countStr, ',');

    if (start.size() != count.size())
    {
        helper::Throw<std::ios_base::failure>(
            "Toolkit", "query::QueryVar", "LoadSelection",
            "dim of startcount does match in the bounding box definition");
    }

    // simpleQ.setSelection(box.first, box.second);
    adios2::Dims shape =
        this->m_Selection.second; // set at the creation for default
    this->SetSelection(start, count);
    if (!this->IsSelectionValid(shape))
        helper::Throw<std::ios_base::failure>(
            "Toolkit", "query::QueryVar", "LoadSelection",
            "invalid selections for selection of var: " + this->GetVarName());
}

bool QueryVar::TouchSelection(adios2::Dims &start, adios2::Dims &count) const
{
    if (0 == m_Selection.first.size())
        return true;

    const size_t dimensionsSize = start.size();

    for (size_t i = 0; i < dimensionsSize; i++)
    {
        size_t end = start[i] + count[i];
        size_t selEnd = m_Selection.first[i] + m_Selection.second[i];

        if (end <= m_Selection.first[i])
            return false;
        if (selEnd <= start[i])
            return false;
    }
    return true;
}

void QueryVar::BlockIndexEvaluate(adios2::core::IO &io,
                                  adios2::core::Engine &reader,
                                  std::vector<Box<Dims>> &touchedBlocks)
{
    const DataType varType = io.InquireVariableType(m_VarName);

    // Variable<int> var = io.InquireVariable<int>(m_VarName);
    // BlockIndex<int> idx(io, reader);

    // var already exists when loading query. skipping validity checking
#define declare_type(T)                                                        \
    if (varType == adios2::helper::GetDataType<T>())                           \
    {                                                                          \
        core::Variable<T> *var = io.InquireVariable<T>(m_VarName);             \
        BlockIndex<T> idx(*var, io, reader);                                   \
        idx.Evaluate(*this, touchedBlocks);                                    \
    }
    // ADIOS2_FOREACH_ATTRIBUTE_TYPE_1ARG(declare_type) //skip complex types
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type

    if (touchedBlocks.size() > 0)
    {
        LimitToSelection(touchedBlocks);
        ApplyOutputRegion(touchedBlocks, m_Selection);
    }
}
} // namespace query
} // namespace adios2
