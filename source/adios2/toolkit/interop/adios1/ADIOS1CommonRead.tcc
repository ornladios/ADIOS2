/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS1Common.tcc
 *
 *  Created on: Jun 1, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 *              William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_INTEROP_ADIOS1_ADIOS1COMMONREAD_TCC_
#define ADIOS2_TOOLKIT_INTEROP_ADIOS1_ADIOS1COMMONREAD_TCC_

#include "ADIOS1CommonRead.h"

#include "adios2/helper/adiosFunctions.h" //GetType

namespace adios2
{
namespace interop
{

template <class T>
void ADIOS1CommonRead::DefineADIOS2Variable(core::IO &io, const char *name,
                                            const ADIOS_VARINFO *vi, Dims gdims,
                                            bool isJoined, bool isGlobal)
{
    if (vi != nullptr)
    {
        Dims zeros(gdims.size(), 0);
        core::Variable<T> &var =
            io.DefineVariable<T>(name, gdims, zeros, gdims);
        if (vi->ndim == 0 && isGlobal)
        {
            /* Global value: store the value now */
            /*var.m_Data = std::vector<T>(1);
            var.m_Data[0] = *static_cast<T *>(vi->value);*/
            var.m_Value = *(
                reinterpret_cast<typename TypeInfo<T>::ValueType *>(vi->value));
            var.m_Min = var.m_Max = var.m_Value;
        }
        var.m_ReadAsLocalValue = (vi->ndim > 0 && !isGlobal);
        var.m_ReadAsJoined = isJoined;
        var.m_AvailableStepsCount = vi->nsteps;
    }
}

template <class T>
void ADIOS1CommonRead::DefineADIOS2Attribute(core::IO &io, const char *name,
                                             void *value)
{
    core::Attribute<T> &attr = io.DefineAttribute<T>(
        name, *(reinterpret_cast<typename TypeInfo<T>::ValueType *>(value)));
}

} // end namespace interop
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_INTEROP_ADIOS1_ADIOS1COMMONREAD_TCC_ */
