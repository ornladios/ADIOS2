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

#include <adios2/ADIOSTypes.h>

namespace adios2
{

/// \cond EXCLUDE_FROM_DOXYGEN
// forward declare
class IO; // friend

namespace core
{
template <class T>
class Attribute; // private implementation
}
/// \endcond

template <class T>
class Attribute
{
    using IOType = typename TypeInfo<T>::IOType;

    friend class IO;

public:
    /**
     * Empty (default) constructor, use it as a placeholder for future
     * attributes from IO:DefineAttribute<T> or IO:InquireAttribute<T>.
     * Can be used with STL containers.
     */
    Attribute<T>() = default;
    ~Attribute<T>() = default;

    /** Checks if object is valid, e.g. if( attribute ) { //..valid } */
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
    Attribute<T>(core::Attribute<IOType> *attribute);
    core::Attribute<IOType> *m_Attribute = nullptr;
};

template <typename T>
std::string ToString(const Attribute<T> &attribute);

} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX11_CXX11_ATTRIBUTE_H_ */
