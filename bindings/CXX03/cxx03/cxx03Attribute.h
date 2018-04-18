/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * cxx03Attribute.h
 *
 *  Created on: Apr 6, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef BINDINGS_CXX03_CXX03_CXX03ATTRIBUTE_H_
#define BINDINGS_CXX03_CXX03_CXX03ATTRIBUTE_H_

#include <string>

#include <adios2_c.h>

namespace adios2
{
namespace cxx03
{

template <class T>
class Attribute
{
public:
    adios2_attribute *m_Attribute;

    Attribute<T>(adios2_attribute *attribute);

    ~Attribute<T>();

    operator bool() const;
};

} // end namespace cxx03
} // end namespace adios2

#endif /* BINDINGS_CXX03_CXX03_CXX03ATTRIBUTE_H_ */
