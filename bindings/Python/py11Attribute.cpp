/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * py11Variable.cpp :
 *
 *  Created on: Dec 11, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "py11Attribute.h"

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace py11
{

Attribute::Attribute(core::AttributeBase *attribute) : m_Attribute(attribute) {}

Attribute::operator bool() const noexcept
{
    return (m_Attribute == nullptr) ? false : true;
}

std::string Attribute::Name() const
{
    helper::CheckForNullptr(m_Attribute, "in call to Attribute::Name");
    return m_Attribute->m_Name;
}

std::string Attribute::Type() const
{
    helper::CheckForNullptr(m_Attribute, "in call to Attribute::Type");
    return m_Attribute->m_Type;
}

pybind11::array Attribute::Data()
{
    helper::CheckForNullptr(m_Attribute, "in call to Attribute::Data");
    // TODO
    return m_Attribute->m_Name;
}

} // end namespace py11
} // end namespace adios2
