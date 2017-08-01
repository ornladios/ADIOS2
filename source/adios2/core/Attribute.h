/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Attribute.h : template class
 *
 *  Created on: Aug 1, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_ATTRIBUTE_H_
#define ADIOS2_CORE_ATTRIBUTE_H_

#include "adios2/core/AttributeBase.h"

namespace adios2
{
/** @brief Attributes provide complementary information to IO Variables*/
template <class T>
class Attribute : public AttributeBase
{

public:
    const T *m_DataArray = nullptr;
    T m_DataValue;

    /**
     * Data array constructor
     * @param name
     * @param data
     * @param elements
     */
    Attribute<T>(const std::string &name, const T *data, const size_t elements);

    /**
     * Single value constructor
     * @param name
     * @param data
     * @param elements
     */
    Attribute<T>(const std::string &name, const T &data);

    ~Attribute<T>() = default;
};

} // end namespace adios2

#endif /* SOURCE_ADIOS2_CORE_ATTRIBUTE_H_ */
