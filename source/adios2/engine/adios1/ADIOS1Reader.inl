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

#include "ADIOS1Reader.h"

namespace adios
{

template <class T>
Variable<T> *ADIOS1Reader::InquireVariableCommon(const std::string &name,
                                                 const bool readIn)
{
    // here read variable metadata (dimensions, type, etc.)...then create a
    // Variable like below:
    // Variable<T>& variable = m_ADIOS.DefineVariable<T>( m_Name + "/" +
    // name, )
    // return &variable; //return address if success
    ADIOS_VARINFO *vi = adios_inq_var(m_fh, name.c_str());
    adios::Variable<T> *var = nullptr;
    if (vi != nullptr)
    {
        CheckADIOS1TypeCompatibility(name, GetType<T>(), vi->type); // throws

        if (vi->ndim > 0)
        {
            Dims gdims = Uint64ArrayToSizetVector(vi->ndim, vi->dims);

            bool joinedread = false;
            if (gdims[0] == JoinedDim)
            {
                /* Joined Array */
                adios_inq_var_blockinfo(m_fh, vi);
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
            var = &m_IO.DefineVariable<T>(name, gdims);
            if (joinedread)
            {
                var->m_ReadAsJoined = true;
            }
        }
        else /* Scalars */
        {
            /* scalar variable but global value or local value*/
            std::string aname = name + "/ReadAsArray";
            bool isLocalValue = false;
            for (int i = 0; i < vi->nattrs; ++i)
            {
                if (!strcmp(m_fh->attr_namelist[vi->attr_ids[i]],
                            aname.c_str()))
                {
                    isLocalValue = true;
                }
            }
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
                    var = &m_IO.DefineVariable<T>(name, {IrregularDim});
                }
                else
                {
                    var = &m_IO.DefineVariable<T>(
                        name, {(unsigned int)vi->nblocks[0]});
                }
                var->m_ReadAsLocalValue = true;
            }
            else
            {
                /* Global value: store only one value */
                var = &m_IO.DefineVariable<T>(name);
                if (var)
                {
                    var->m_Data = std::vector<T>(1);
                    var->m_Data[0] = *static_cast<T *>(vi->value);
                }
            }
        }
        var->m_AvailableSteps = vi->nsteps;
        adios_free_varinfo(vi);
    }
    return var;
}

} // end namespace adios

#endif /* ADIOS2_ENGINE_ADIOS1_ADIOS1READER_INL_ */
