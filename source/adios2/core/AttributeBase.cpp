/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * AttributeBase.cpp
 *
 *  Created on: Aug 1, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "AttributeBase.h"

namespace adios2
{
namespace core
{

AttributeBase::AttributeBase(const std::string &name, const DataType type)
: m_Name(name), m_Type(type), m_Elements(1), m_IsSingleValue(true)
{
}

AttributeBase::AttributeBase(const std::string &name, const DataType type,
                             const size_t elements)
: m_Name(name), m_Type(type), m_Elements(elements), m_IsSingleValue(false)
{
}

Params AttributeBase::GetInfo() const noexcept
{
    Params info;
    info["Type"] = ToString(m_Type);
    info["Elements"] = std::to_string(m_Elements);
    info["Value"] = this->DoGetInfoValue();
    return info;
}

} // end namespace core
} // end namespace adios2
