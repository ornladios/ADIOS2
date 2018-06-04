/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * cxx98Attribute.h
 *
 *  Created on: Apr 6, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_CXX98_CXX98_CXX98ATTRIBUTE_H_
#define ADIOS2_BINDINGS_CXX98_CXX98_CXX98ATTRIBUTE_H_

#include <string>
#include <vector>

struct adios2_attribute;

namespace adios2
{
namespace cxx98
{

// forward declare
class IO;

template <class T>
class Attribute
{
    friend class IO;

public:
    Attribute<T>();
    ~Attribute<T>();

    operator bool() const;

    std::string Name() const;
    std::string Type() const;
    std::vector<T> Data() const;

private:
    Attribute<T>(adios2_attribute *attribute);
    adios2_attribute *m_Attribute;
};

} // end namespace cxx98
} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX98_CXX98_CXX98ATTRIBUTE_H_ */
