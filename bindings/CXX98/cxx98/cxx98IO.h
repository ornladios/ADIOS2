/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * cxx98IO.h
 *
 *  Created on: Apr 5, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef BINDINGS_CXX98_CXX98_CXX98IO_H_
#define BINDINGS_CXX98_CXX98_CXX98IO_H_

#include <cstddef>
#include <string>
#include <vector>

#include <adios2_c.h>

#include "cxx98Attribute.h"
#include "cxx98Engine.h"
#include "cxx98Variable.h"
#include "cxx98types.h"

namespace adios2
{
namespace cxx98
{

class IO
{
public:
    IO(adios2_io &io);

    ~IO();

    template <class T>
    Variable<T>
    DefineVariable(const std::string &name, const Dims &shape = Dims(),
                   const Dims &start = Dims(), const Dims &count = Dims(),
                   const bool constantDims = false, T *data = NULL);

    template <class T>
    Variable<T> InquireVariable(const std::string &name);

    /**
     * @brief Define array attribute
     * @param name must be unique for the IO object
     * @param array pointer to user data
     * @param elements number of data elements
     * @return reference to internal Attribute
     * @exception std::invalid_argument if Attribute with unique name is
     * already defined, in debug mode only
     */
    template <class T>
    Attribute<T> DefineAttribute(const std::string &name, const T *array,
                                 const size_t elements);

    /**
     * @brief Define single value attribute
     * @param name must be unique for the IO object
     * @param value single data value
     * @return reference to internal Attribute
     * @exception std::invalid_argument if Attribute with unique name is already
     * defined, in debug mode only
     */
    template <class T>
    Attribute<T> DefineAttribute(const std::string &name, const T &value);

/**
 * TODO Gets an existing attribute of primitive type by name
 * @param name of attribute to be retrieved
 * @return pointer to an existing attribute in current IO, NULL if not
 * found
 */
//    template <class T>
//    Attribute<T> InquireAttribute(const std::string &name);

#ifdef ADIOS2_HAVE_MPI
    Engine Open(const std::string &name, const Mode mode, MPI_Comm comm);
#endif

    Engine Open(const std::string &name, const Mode mode);

private:
    adios2_io &m_IO;
};

// Explicit declaration of the public template methods
#define declare_template_instantiation(T)                                      \
    extern template Variable<T> IO::InquireVariable<T>(const std::string &);

ADIOS2_FOREACH_CXX98_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace cxx98
} // end namespace adios2

#endif /* BINDINGS_CXX98_CXX98_CXX98IO_H_ */
