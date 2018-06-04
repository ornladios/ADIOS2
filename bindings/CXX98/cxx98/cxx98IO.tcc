/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * cxx98IO.tcc
 *
 *  Created on: Apr 5, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef BINDINGS_CXX98_CXX98_CXX98IO_TCC_
#define BINDINGS_CXX98_CXX98_CXX98IO_TCC_

#include "cxx98IO.h"

namespace adios2
{
namespace cxx98
{

namespace
{

adios2_constant_dims GetConstantDims(const bool constantDims)
{
    const adios2_constant_dims constant_dims =
        constantDims ? adios2_constant_dims_true : adios2_constant_dims_false;
    return constant_dims;
}

} // end empty namespace

#define V0(v) (v.empty() ? NULL : &v[0])

// DefineVariable
template <>
Variable<std::string> IO::DefineVariable(const std::string &name,
                                         const Dims &shape, const Dims &start,
                                         const Dims &count,
                                         const bool constantDims)
{
    return Variable<std::string>(adios2_define_variable(
        &m_IO, name.c_str(), adios2_type_string, count.size(), V0(shape),
        V0(start), V0(count), GetConstantDims(constantDims)));
}

template <>
Variable<char> IO::DefineVariable(const std::string &name, const Dims &shape,
                                  const Dims &start, const Dims &count,
                                  const bool constantDims)
{
    return Variable<char>(adios2_define_variable(
        &m_IO, name.c_str(), adios2_type_char, count.size(), V0(shape),
        V0(start), V0(count), GetConstantDims(constantDims)));
}

template <>
Variable<signed char> IO::DefineVariable(const std::string &name,
                                         const Dims &shape, const Dims &start,
                                         const Dims &count,
                                         const bool constantDims)
{
    return Variable<signed char>(adios2_define_variable(
        &m_IO, name.c_str(), adios2_type_signed_char, count.size(), V0(shape),
        V0(start), V0(count), GetConstantDims(constantDims)));
}

template <>
Variable<unsigned char> IO::DefineVariable(const std::string &name,
                                           const Dims &shape, const Dims &start,
                                           const Dims &count,
                                           const bool constantDims)
{
    return Variable<unsigned char>(adios2_define_variable(
        &m_IO, name.c_str(), adios2_type_unsigned_char, count.size(), V0(shape),
        V0(start), V0(count), GetConstantDims(constantDims)));
}

template <>
Variable<short> IO::DefineVariable(const std::string &name, const Dims &shape,
                                   const Dims &start, const Dims &count,
                                   const bool constantDims)
{
    return Variable<short>(adios2_define_variable(
        &m_IO, name.c_str(), adios2_type_short, count.size(), V0(shape),
        V0(start), V0(count), GetConstantDims(constantDims)));
}

template <>
Variable<unsigned short>
IO::DefineVariable(const std::string &name, const Dims &shape,
                   const Dims &start, const Dims &count,
                   const bool constantDims)
{
    return Variable<unsigned short>(adios2_define_variable(
        &m_IO, name.c_str(), adios2_type_unsigned_short, count.size(),
        V0(shape), V0(start), V0(count), GetConstantDims(constantDims)));
}

template <>
Variable<int> IO::DefineVariable(const std::string &name, const Dims &shape,
                                 const Dims &start, const Dims &count,
                                 const bool constantDims)
{
    return Variable<int>(adios2_define_variable(
        &m_IO, name.c_str(), adios2_type_int, count.size(), V0(shape),
        V0(start), V0(count), GetConstantDims(constantDims)));
}

template <>
Variable<unsigned int> IO::DefineVariable(const std::string &name,
                                          const Dims &shape, const Dims &start,
                                          const Dims &count,
                                          const bool constantDims)
{
    return Variable<unsigned int>(adios2_define_variable(
        &m_IO, name.c_str(), adios2_type_unsigned_int, count.size(), V0(shape),
        V0(start), V0(count), GetConstantDims(constantDims)));
}

template <>
Variable<long int> IO::DefineVariable(const std::string &name,
                                      const Dims &shape, const Dims &start,
                                      const Dims &count,
                                      const bool constantDims)
{
    return Variable<long int>(adios2_define_variable(
        &m_IO, name.c_str(), adios2_type_long_int, count.size(), V0(shape),
        V0(start), V0(count), GetConstantDims(constantDims)));
}

template <>
Variable<unsigned long int>
IO::DefineVariable(const std::string &name, const Dims &shape,
                   const Dims &start, const Dims &count,
                   const bool constantDims)
{
    return Variable<unsigned long int>(adios2_define_variable(
        &m_IO, name.c_str(), adios2_type_unsigned_long_int, count.size(),
        V0(shape), V0(start), V0(count), GetConstantDims(constantDims)));
}

template <>
Variable<long long int> IO::DefineVariable(const std::string &name,
                                           const Dims &shape, const Dims &start,
                                           const Dims &count,
                                           const bool constantDims)
{
    return Variable<long long int>(adios2_define_variable(
        &m_IO, name.c_str(), adios2_type_long_long_int, count.size(), V0(shape),
        V0(start), V0(count), GetConstantDims(constantDims)));
}

template <>
Variable<unsigned long long int>
IO::DefineVariable(const std::string &name, const Dims &shape,
                   const Dims &start, const Dims &count,
                   const bool constantDims)
{
    return Variable<unsigned long long int>(adios2_define_variable(
        &m_IO, name.c_str(), adios2_type_unsigned_long_long_int, count.size(),
        V0(shape), V0(start), V0(count), GetConstantDims(constantDims)));
}

template <>
Variable<float> IO::DefineVariable(const std::string &name, const Dims &shape,
                                   const Dims &start, const Dims &count,
                                   const bool constantDims)
{
    return Variable<float>(adios2_define_variable(
        &m_IO, name.c_str(), adios2_type_float, count.size(), V0(shape),
        V0(start), V0(count), GetConstantDims(constantDims)));
}

template <>
Variable<double> IO::DefineVariable(const std::string &name, const Dims &shape,
                                    const Dims &start, const Dims &count,
                                    const bool constantDims)
{
    return Variable<double>(adios2_define_variable(
        &m_IO, name.c_str(), adios2_type_double, count.size(), V0(shape),
        V0(start), V0(count), GetConstantDims(constantDims)));
}

template <>
Variable<std::complex<float>>
IO::DefineVariable(const std::string &name, const Dims &shape,
                   const Dims &start, const Dims &count,
                   const bool constantDims)
{
    return Variable<std::complex<float>>(adios2_define_variable(
        &m_IO, name.c_str(), adios2_type_float_complex, count.size(), V0(shape),
        V0(start), V0(count), GetConstantDims(constantDims)));
}

template <>
Variable<std::complex<double>>
IO::DefineVariable(const std::string &name, const Dims &shape,
                   const Dims &start, const Dims &count,
                   const bool constantDims)
{
    return Variable<std::complex<double>>(adios2_define_variable(
        &m_IO, name.c_str(), adios2_type_double_complex, count.size(),
        V0(shape), V0(start), V0(count), GetConstantDims(constantDims)));
}

// DefineAttribute
template <>
Attribute<std::string> IO::DefineAttribute(const std::string &name,
                                           const std::string *array,
                                           const size_t elements)
{
    return Attribute<std::string>(adios2_define_attribute(
        &m_IO, name.c_str(), adios2_type_string_array, array, elements));
}

template <>
Attribute<char> IO::DefineAttribute(const std::string &name, const char *array,
                                    const size_t elements)
{
    return Attribute<char>(adios2_define_attribute(
        &m_IO, name.c_str(), adios2_type_char, array, elements));
}

template <>
Attribute<unsigned char> IO::DefineAttribute(const std::string &name,
                                             const unsigned char *array,
                                             const size_t elements)
{
    return Attribute<unsigned char>(adios2_define_attribute(
        &m_IO, name.c_str(), adios2_type_unsigned_char, array, elements));
}

template <>
Attribute<short> IO::DefineAttribute(const std::string &name,
                                     const short *array, const size_t elements)
{
    return Attribute<short>(adios2_define_attribute(
        &m_IO, name.c_str(), adios2_type_short, array, elements));
}

template <>
Attribute<unsigned short> IO::DefineAttribute(const std::string &name,
                                              const unsigned short *array,
                                              const size_t elements)
{
    return Attribute<unsigned short>(adios2_define_attribute(
        &m_IO, name.c_str(), adios2_type_unsigned_short, array, elements));
}

template <>
Attribute<int> IO::DefineAttribute(const std::string &name, const int *array,
                                   const size_t elements)
{
    return Attribute<int>(adios2_define_attribute(
        &m_IO, name.c_str(), adios2_type_int, array, elements));
}

template <>
Attribute<unsigned int> IO::DefineAttribute(const std::string &name,
                                            const unsigned int *array,
                                            const size_t elements)
{
    return Attribute<unsigned int>(adios2_define_attribute(
        &m_IO, name.c_str(), adios2_type_unsigned_int, array, elements));
}

template <>
Attribute<long int> IO::DefineAttribute(const std::string &name,
                                        const long int *array,
                                        const size_t elements)
{
    return Attribute<long int>(adios2_define_attribute(
        &m_IO, name.c_str(), adios2_type_long_int, array, elements));
}

template <>
Attribute<unsigned long int> IO::DefineAttribute(const std::string &name,
                                                 const unsigned long int *array,
                                                 const size_t elements)
{
    return Attribute<unsigned long int>(adios2_define_attribute(
        &m_IO, name.c_str(), adios2_type_unsigned_long_int, array, elements));
}

template <>
Attribute<long long int> IO::DefineAttribute(const std::string &name,
                                             const long long int *array,
                                             const size_t elements)
{
    return Attribute<long long int>(adios2_define_attribute(
        &m_IO, name.c_str(), adios2_type_long_long_int, array, elements));
}

template <>
Attribute<unsigned long long int>
IO::DefineAttribute(const std::string &name,
                    const unsigned long long int *array, const size_t elements)
{
    return Attribute<unsigned long long int>(adios2_define_attribute(
        &m_IO, name.c_str(), adios2_type_unsigned_long_long_int, array,
        elements));
}

template <>
Attribute<float> IO::DefineAttribute(const std::string &name,
                                     const float *array, const size_t elements)
{
    return Attribute<float>(adios2_define_attribute(
        &m_IO, name.c_str(), adios2_type_float, array, elements));
}

template <>
Attribute<double> IO::DefineAttribute(const std::string &name,
                                      const double *array,
                                      const size_t elements)
{
    return Attribute<double>(adios2_define_attribute(
        &m_IO, name.c_str(), adios2_type_double, array, elements));
}

// Define Single Value Attribute
template <>
Attribute<char> IO::DefineAttribute(const std::string &name, const char &value)
{
    const char valueLocal = value;
    return Attribute<char>(adios2_define_attribute(
        &m_IO, name.c_str(), adios2_type_char, &valueLocal, 1));
}

template <>
Attribute<unsigned char> IO::DefineAttribute(const std::string &name,
                                             const unsigned char &value)
{
    const unsigned char valueLocal = value;
    return Attribute<unsigned char>(adios2_define_attribute(
        &m_IO, name.c_str(), adios2_type_unsigned_char, &valueLocal, 1));
}

template <>
Attribute<short> IO::DefineAttribute(const std::string &name,
                                     const short &value)
{
    const short valueLocal = value;
    return Attribute<short>(adios2_define_attribute(
        &m_IO, name.c_str(), adios2_type_short, &valueLocal, 1));
}

template <>
Attribute<unsigned short> IO::DefineAttribute(const std::string &name,
                                              const unsigned short &value)
{
    const unsigned short valueLocal = value;
    return Attribute<unsigned short>(adios2_define_attribute(
        &m_IO, name.c_str(), adios2_type_unsigned_short, &valueLocal, 1));
}

template <>
Attribute<int> IO::DefineAttribute(const std::string &name, const int &value)
{
    const int valueLocal = value;
    return Attribute<int>(adios2_define_attribute(
        &m_IO, name.c_str(), adios2_type_int, &valueLocal, 1));
}

template <>
Attribute<unsigned int> IO::DefineAttribute(const std::string &name,
                                            const unsigned int &value)
{
    const unsigned int valueLocal = value;
    return Attribute<unsigned int>(adios2_define_attribute(
        &m_IO, name.c_str(), adios2_type_unsigned_int, &valueLocal, 1));
}

template <>
Attribute<long int> IO::DefineAttribute(const std::string &name,
                                        const long int &value)
{
    const long int valueLocal = value;
    return Attribute<long int>(adios2_define_attribute(
        &m_IO, name.c_str(), adios2_type_long_int, &valueLocal, 1));
}

template <>
Attribute<unsigned long int> IO::DefineAttribute(const std::string &name,
                                                 const unsigned long int &value)
{
    const unsigned long int valueLocal = value;
    return Attribute<unsigned long int>(adios2_define_attribute(
        &m_IO, name.c_str(), adios2_type_unsigned_long_int, &valueLocal, 1));
}

template <>
Attribute<long long int> IO::DefineAttribute(const std::string &name,
                                             const long long int &value)
{
    const long long int valueLocal = value;
    return Attribute<long long int>(adios2_define_attribute(
        &m_IO, name.c_str(), adios2_type_long_long_int, &valueLocal, 1));
}

template <>
Attribute<unsigned long long int>
IO::DefineAttribute(const std::string &name,
                    const unsigned long long int &value)
{
    const unsigned long long int valueLocal = value;
    return Attribute<unsigned long long int>(adios2_define_attribute(
        &m_IO, name.c_str(), adios2_type_unsigned_long_long_int, &valueLocal,
        1));
}

template <>
Attribute<float> IO::DefineAttribute(const std::string &name,
                                     const float &value)
{
    const float valueLocal = value;
    return Attribute<float>(adios2_define_attribute(
        &m_IO, name.c_str(), adios2_type_float, &valueLocal, 1));
}

template <>
Attribute<double> IO::DefineAttribute(const std::string &name,
                                      const double &value)
{
    const double valueLocal = value;
    return Attribute<double>(adios2_define_attribute(
        &m_IO, name.c_str(), adios2_type_double, &valueLocal, 1));
}

template <class T>
Variable<T> IO::InquireVariable(const std::string &name)
{
    return Variable<T>(adios2_inquire_variable(&m_IO, name.c_str()));
}

#undef V0

} // end namespace cxx98
} // end namespace adios2

#endif /* BINDINGS_CXX98_CXX98_CXX98IO_TCC_ */
