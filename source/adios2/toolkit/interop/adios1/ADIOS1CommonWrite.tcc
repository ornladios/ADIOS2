/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS1CommonWrite.tcc
 *
 *  Created on: Oct 26, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 *              William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_INTEROP_ADIOS1_ADIOS1COMMONWRITE_TCC_
#define ADIOS2_TOOLKIT_INTEROP_ADIOS1_ADIOS1COMMONWRITE_TCC_

#include "ADIOS1CommonWrite.h"

#include "adios2/helper/adiosFunctions.h" //GetType

namespace adios2
{
namespace interop
{

template <class T>
void ADIOS1CommonWrite::WriteVariable(const std::string &name,
                                      const ShapeID shapeID, const Dims ldims,
                                      const Dims gdims, const Dims offsets,
                                      const T *values)
{
    if (ReOpenAsNeeded())
    {
        if (GetADIOS1Type<T>() != adios_unknown)
        {
            DefineVariable(
                name, shapeID, GetADIOS1Type<T>(), DimsToCSVLocalAware(ldims),
                DimsToCSVLocalAware(gdims), DimsToCSVLocalAware(offsets));
            adios_write(m_ADIOSFile, name.c_str(), values);
        }
        else
        {
            throw std::invalid_argument("ERROR: ADIOS1 doesn't support type " +
                                        helper::GetType<T>() +
                                        ", in call to Write\n");
        }
    }
}

} // end namespace interop
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_INTEROP_ADIOS1_ADIOS1COMMONWRITE_TCC_ */
