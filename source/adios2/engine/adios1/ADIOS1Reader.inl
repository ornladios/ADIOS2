/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS1Reader.inl
 *
 *  Created on: Jun 2, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#ifndef ADIOS2_ENGINE_ADIOS1_ADIOS1READER_INL_
#define ADIOS2_ENGINE_ADIOS1_ADIOS1READER_INL_
#ifndef ADIOS2_ENGINE_ADIOS1_ADIOS1READER_H_
#error "Inline file should only be included from it's header, never on it's own"
#endif

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
Variable<T> *
ADIOS1Reader::InquireVariableCommon(const std::string &variableName)
{
    // here read variable metadata (dimensions, type, etc.)...then create a
    // Variable like below:
    // Variable<T>& variable = m_ADIOS.DefineVariable<T>( m_Name + "/" +
    // name, )
    // return &variable; //return address if success
    ADIOS_VARINFO *vi = m_ADIOS1.InqVar(variableName);
    Variable<T> *var = nullptr;
    if (vi != nullptr)
    {
        CheckADIOS1TypeCompatibility(variableName, helper::GetType<T>(),
                                     vi->type); // throws

        if (vi->ndim > 0)
        {
            Dims gdims = helper::Uint64ArrayToSizetVector(vi->ndim, vi->dims);

            bool joinedread = false;
            if (gdims[0] == JoinedDim)
            {
                /* Joined Array */
                m_ADIOS1.InqVarBlockInfo(vi);
                size_t joined_size = 0;
                for (int i = 0; i < *vi->nblocks; i++)
                {
                    joined_size += vi->blockinfo[i].count[0];
                }
                gdims[0] = joined_size;
                joinedread = true;
            }

            if (!vi->global)
            {
                /* Local array */
                for (int j = 0; j < vi->ndim; ++j)
                {
                    gdims[j] = IrregularDim;
                }
            }
            else
            {
                /* Check if dimensions change in time */
                for (int step = 1; step < vi->nsteps; ++step)
                {
                    Dims dims =
                        gdims; // GetGlobalDimsAtStep(vi, step, joinedread);
                    for (int j = 0; j < vi->ndim; ++j)
                    {
                        if (dims[j] != gdims[j])
                            gdims[j] = IrregularDim;
                    }
                }
            }
            var = &m_IO.DefineVariable<T>(variableName, gdims);
            if (joinedread)
            {
                var->m_ReadAsJoined = true;
            }
        }
        else /* Scalars */
        {
            /* scalar variable but global value or local value*/
            bool isLocalValue = m_ADIOS1.IsVarLocalValue(vi);
            if (isLocalValue)
            {
                /* Local Value */
                bool changingDims = false;
                for (int step = 1; step < vi->nsteps; ++step)
                {
                    if (vi->nblocks[step] != vi->nblocks[0])
                        changingDims = true;
                }
                if (changingDims)
                {
                    var = &m_IO.DefineVariable<T>(variableName, {IrregularDim});
                }
                else
                {
                    var = &m_IO.DefineVariable<T>(
                        variableName, {(unsigned int)vi->nblocks[0]});
                }
                var->m_ReadAsLocalValue = true;
            }
            else
            {
                /* Global value: store only one value */
                var = &m_IO.DefineVariable<T>(variableName);
                if (var)
                {
                    var->m_Data = std::vector<T>(1);
                    var->m_Data[0] = *static_cast<T *>(vi->value);
                }
            }
        }
        var->m_AvailableSteps = vi->nsteps;
        m_ADIOS1.FreeVarInfo(vi);
    }
    return var;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_ADIOS1_ADIOS1READER_INL_ */
