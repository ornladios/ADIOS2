/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Attribute.h : template class that defines typed attributes
 *
 *  Created on: Aug 1, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_ATTRIBUTE_H_
#define ADIOS2_CORE_ATTRIBUTE_H_

#include "adios2/core/AttributeBase.h"

namespace adios2
{
namespace core
{

/** @brief Attributes provide complementary information to IO Variables*/
template <class T>
class Attribute : public AttributeBase
{

public:
    std::vector<T> m_DataArray; ///< holds data for array attributes
    T m_DataSingleValue;        ///< holds data for single value attributes

    /**
     * Copy constructor (enforces zero-padding)
     * @param other
     */
    Attribute<T>(const Attribute<T> &other);

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

private:
    std::string DoGetInfoValue() const noexcept override;
};

template <typename V>
void AttributeVisit(AttributeBase const &a, V const &v)
{
    switch (a.m_Type)
    {
    case DataType::None:
        break;
    case DataType::Int8:
        v(static_cast<Attribute<int8_t> const &>(a));
        break;
    case DataType::Int16:
        v(static_cast<Attribute<int16_t> const &>(a));
        break;
    case DataType::Int32:
        v(static_cast<Attribute<int32_t> const &>(a));
        break;
    case DataType::Int64:
        v(static_cast<Attribute<int64_t> const &>(a));
        break;
    case DataType::UInt8:
        v(static_cast<Attribute<uint8_t> const &>(a));
        break;
    case DataType::UInt16:
        v(static_cast<Attribute<uint16_t> const &>(a));
        break;
    case DataType::UInt32:
        v(static_cast<Attribute<uint32_t> const &>(a));
        break;
    case DataType::UInt64:
        v(static_cast<Attribute<uint64_t> const &>(a));
        break;
    case DataType::Float:
        v(static_cast<Attribute<float> const &>(a));
        break;
    case DataType::Double:
        v(static_cast<Attribute<double> const &>(a));
        break;
    case DataType::LongDouble:
        v(static_cast<Attribute<long double> const &>(a));
        break;
    case DataType::FloatComplex:
        v(static_cast<Attribute<std::complex<float>> const &>(a));
        break;
    case DataType::DoubleComplex:
        v(static_cast<Attribute<std::complex<double>> const &>(a));
        break;
    case DataType::String:
        v(static_cast<Attribute<std::string> const &>(a));
        break;
    case DataType::Compound:
        // No compound attributes.
        break;
    }
}

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_ATTRIBUTE_H_ */
