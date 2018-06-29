/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Attribute.h :
 *
 *  Created on: Jun 4, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_CXX11_CXX11_ATTRIBUTE_H_
#define ADIOS2_BINDINGS_CXX11_CXX11_ATTRIBUTE_H_

#include <string>
#include <vector>

namespace adios2
{

// forward declare
class IO; // friend

namespace core
{
template <class T>
class Attribute; // private implementation
}

template <class T>
class Attribute
{
    friend class IO;

public:
    Attribute<T>() = default;
    ~Attribute<T>() = default;

    /** true: valid, false: invalid */
    explicit operator bool() const noexcept;

    /**
     * Inspect attribute name
     * @return unique name identifier
     */
    std::string Name() const;

    /**
     * Inspect attribute type
     * @return type
     */
    std::string Type() const;

    /**
     * Inspect attribute data
     * @return data
     */
    std::vector<T> Data() const;

private:
    Attribute<T>(core::Attribute<T> *attribute);
    core::Attribute<T> *m_Attribute = nullptr;
};

} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX11_CXX11_ATTRIBUTE_H_ */
