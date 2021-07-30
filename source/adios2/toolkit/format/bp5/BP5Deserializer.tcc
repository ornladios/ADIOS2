/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP5Deserializer.tcc
 *
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP5_BP5DESERIALIZER_TCC_
#define ADIOS2_TOOLKIT_FORMAT_BP5_BP5DESERIALIZER_TCC_

#include "BP5Deserializer.h"

#include <algorithm> //std::reverse
#include <unordered_set>

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace format
{

template <class T>
std::vector<typename core::Variable<T>::BPInfo>
BP5Deserializer::BlocksInfo(const core::Variable<T> &variable,
                            const size_t step) const
{
    auto VarRec = VarByKey.find((void *)&variable)->second;
    if (!VarRec)
    {
        return std::vector<typename core::Variable<T>::BPInfo>();
    }
    std::vector<typename core::Variable<T>::BPInfo> Ret;
    for (size_t WriterRank = 0; WriterRank < m_WriterCohortSize; WriterRank++)
    {
        const void *BaseData = MetadataBaseAddrs[WriterRank];
        struct ControlStruct *ControlArray =
            &ActiveControl[WriterRank]->Controls[0];
        int i = 0;
        while (ControlArray[i].VarRec != VarRec)
            i++;
        int FieldOffset = ControlArray[i].FieldOffset;
        void *field_data = (char *)BaseData + FieldOffset;
        MetaArrayRec *meta_base = (MetaArrayRec *)field_data;
        for (size_t i = 0; i < VarRec->PerWriterBlockCount[WriterRank]; i++)
        {
            size_t *Offsets = NULL;
            if (meta_base->Offsets)
                Offsets = meta_base->Offsets + (i * meta_base->Dims);
            typename core::Variable<T>::BPInfo Tmp;
            std::vector<size_t> VecShape;
            std::vector<size_t> VecStart;
            std::vector<size_t> VecCount;
            size_t DimCount = meta_base->Dims;
            size_t *Start = Offsets;
            size_t *Shape = meta_base->Shape;
            size_t *Count = meta_base->Count;
            if (Shape)
            {
                for (size_t i = 0; i < DimCount; i++)
                {
                    VecShape.push_back(Shape[i]);
                    VecStart.push_back(Start[i]);
                    VecCount.push_back(Count[i]);
                }
            }
            else
            {
                VecShape = {};
                VecStart = {};
                for (size_t i = 0; i < DimCount; i++)
                {
                    VecCount.push_back(Count[i]);
                }
            }
            Tmp.Shape = VecShape;
            Tmp.Start = VecStart;
            Tmp.Count = VecCount;
            Ret.push_back(Tmp);
        }
    }
    return Ret;
}

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BP5_BP5DESERIALIZER_TCC_ */
