/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Attribute.tcc : templated member functions for Attribute<T>
 *
 *  Created on: Feb 23, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_ATTRIBUTE_TCC_
#define ADIOS2_CORE_ATTRIBUTE_TCC_

#include "Attribute.h"

#include "adios2/helper/adiosFunctions.h" //GetType<T>

namespace adios2
{
namespace core
{
template <class T>
Attribute<T>::Attribute(const std::string &name, const T *array,
                        const size_t elements)
: AttributeBase(name, helper::GetType<T>(), elements), m_DataSingleValue()
{
    m_DataArray = std::vector<T>(array, array + elements);
}

template <class T>
Attribute<T>::Attribute(const std::string &name, const T &value)
: AttributeBase(name, helper::GetType<T>()), m_DataSingleValue(value)
{
}

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_ATTRIBUTE_TCC_ */
