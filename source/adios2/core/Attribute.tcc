/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Attribute.tcc
 *
 *  Created on: Oct 9, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_ATTRIBUTE_TCC_
#define ADIOS2_CORE_ATTRIBUTE_TCC_

#include "Attribute.h"

#include "adios2/helper/adiosType.h"

namespace adios2
{
namespace core
{

template <class T>
std::string Attribute<T>::DoGetInfoValue() const noexcept
{
    std::string value;
    if (m_IsSingleValue)
    {
        value = helper::ValueToString(m_DataSingleValue);
    }
    else
    {
        value = "{ " + helper::VectorToCSV(m_DataArray) + " }";
    }
    return value;
}

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_ATTRIBUTE_TCC_ */
