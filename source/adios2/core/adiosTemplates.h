/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosTemplates.h
 *
 *  Created on: Jan 26, 2017
 *      Author: wfg
 */

#ifndef ADIOS2_CORE_ADIOSTEMPLATES_H_
#define ADIOS2_CORE_ADIOSTEMPLATES_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <algorithm> //std::minmax_element
#include <cmath>     //std::sqrt
#include <complex>
#include <cstring> //std::memcpy
#include <iostream>
#include <set>
#include <thread>
#include <vector>
/// \endcond

#include "adios2/ADIOSConfig.h"
#include "adios2/ADIOSTypes.h"

namespace adios
{
/**
 * Get the primitive type in a string from a template
 * @return if T is a char, returns string = "char"
 */
template <class T>
inline std::string GetType() noexcept
{
    return "compound";
}
template <>
inline std::string GetType<void>() noexcept
{
    return "unknown";
}
template <>
inline std::string GetType<char>() noexcept
{
    return "char";
}
template <>
inline std::string GetType<unsigned char>() noexcept
{
    return "unsigned char";
}
template <>
inline std::string GetType<short>() noexcept
{
    return "short";
}
template <>
inline std::string GetType<unsigned short>() noexcept
{
    return "unsigned short";
}
template <>
inline std::string GetType<int>() noexcept
{
    return "int";
}
template <>
inline std::string GetType<unsigned int>() noexcept
{
    return "unsigned int";
}
template <>
inline std::string GetType<long int>() noexcept
{
    return "long int";
}
template <>
inline std::string GetType<unsigned long int>() noexcept
{
    return "unsigned long int";
}
template <>
inline std::string GetType<long long int>() noexcept
{
    return "long long int";
}
template <>
inline std::string GetType<unsigned long long int>() noexcept
{
    return "unsigned long long int";
}
template <>
inline std::string GetType<float>() noexcept
{
    return "float";
}
template <>
inline std::string GetType<double>() noexcept
{
    return "double";
}
template <>
inline std::string GetType<long double>() noexcept
{
    return "long double";
}
template <>
inline std::string GetType<std::complex<float>>() noexcept
{
    return "float complex";
}
template <>
inline std::string GetType<std::complex<double>>() noexcept
{
    return "double complex";
}
template <>
inline std::string GetType<std::complex<long double>>() noexcept
{
    return "long double complex";
}

/**
 * Check in types set if "type" is one of the aliases for a certain type,
 * (e.g. if type = integer is an accepted alias for "int", returning true)
 * @param type input to be compared with an alias
 * @param aliases set containing aliases to a certain type, typically
 * Support::DatatypesAliases from Support.h
 * @return true: is an alias, false: is not
 */
template <class T>
bool IsTypeAlias(
    const std::string type,
    const std::map<std::string, std::set<std::string>> &aliases) noexcept
{
    if (type == GetType<T>()) // most of the time we will pass the same type,
                              // which is a key in aliases
        return true;

    bool isAlias = false;
    if (aliases.at(GetType<T>()).count(type) == 1)
        isAlias = true;

    return isAlias;
}

template <class T>
void GetMinMax(const T *values, const size_t size, T &min, T &max) noexcept
{
    auto bounds = std::minmax_element(values, values + size);
    min = *bounds.first;
    max = *bounds.second;
}

/**
 * Get the minimum and maximum values in one loop
 * @param values array of primitives
 * @param size of the values array
 * @param min from values
 * @param max from values
 */
template <class T>
void GetMinMaxThreads(const T *values, const size_t size, T &min, T &max,
                      const unsigned int threads = 1) noexcept
{
    if (threads == 1)
    {
        GetMinMax(values, size, min, max);
        return;
    }

    const size_t stride = size / threads;    // elements per thread
    const size_t remainder = size % threads; // remainder if not aligned
    const size_t last = stride + remainder;

    std::vector<T> mins(threads); // zero init
    std::vector<T> maxs(threads); // zero init

    std::vector<std::thread> getMinMaxThreads;
    getMinMaxThreads.reserve(threads);

    for (unsigned int t = 0; t < threads; ++t)
    {
        const size_t position = stride * t;

        if (t == threads - 1)
        {
            getMinMaxThreads.push_back(
                std::thread(adios::GetMinMax<T>, &values[position], last,
                            std::ref(mins[t]), std::ref(maxs[t])));
        }
        else
        {
            getMinMaxThreads.push_back(
                std::thread(adios::GetMinMax<T>, &values[position], stride,
                            std::ref(mins[t]), std::ref(maxs[t])));
        }
    }

    for (auto &getMinMaxThread : getMinMaxThreads)
    {
        getMinMaxThread.join();
    }

    auto itMin = std::min_element(mins.begin(), mins.end());
    min = *itMin;

    auto itMax = std::max_element(maxs.begin(), maxs.end());
    max = *itMax;
}

/**
 * Overloaded version for complex types, gets the "doughnut" range between min
 * and max modulus
 * @param values array of complex numbers
 * @param size of the values array
 * @param min modulus from values
 * @param max modulus from values
 */
template <class T>
void GetMinMaxComplex(const std::complex<T> *values, const size_t size, T &min,
                      T &max) noexcept
{

    min = std::norm(values[0]);
    max = min;

    for (size_t i = 1; i < size; ++i)
    {
        T norm = std::norm(values[i]);

        if (norm < min)
        {
            min = norm;
            continue;
        }

        if (norm > max)
        {
            max = norm;
        }
    }

    min = std::sqrt(min);
    max = std::sqrt(max);
}

template <class T>
void GetMinMaxThreads(const std::complex<T> *values, const size_t size, T &min,
                      T &max, const unsigned int threads = 1) noexcept
{
    if (threads == 1)
    {
        GetMinMaxComplex(values, size, min, max);
        return;
    }

    const size_t stride = size / threads;    // elements per thread
    const size_t remainder = size % threads; // remainder if not aligned
    const size_t last = stride + remainder;

    std::vector<T> mins(threads); // zero init
    std::vector<T> maxs(threads); // zero init

    std::vector<std::thread> getMinMaxThreads;
    getMinMaxThreads.reserve(threads);

    for (unsigned int t = 0; t < threads; ++t)
    {
        const size_t position = stride * t;

        if (t == threads - 1)
        {
            getMinMaxThreads.push_back(
                std::thread(GetMinMaxComplex<T>, &values[position], last,
                            std::ref(mins[t]), std::ref(maxs[t])));
        }
        else
        {
            getMinMaxThreads.push_back(
                std::thread(GetMinMaxComplex<T>, &values[position], stride,
                            std::ref(mins[t]), std::ref(maxs[t])));
        }
    }

    for (auto &getMinMaxThread : getMinMaxThreads)
    {
        getMinMaxThread.join();
    }

    auto itMin = std::min_element(mins.begin(), mins.end());
    min = *itMin;

    auto itMax = std::max_element(maxs.begin(), maxs.end());
    max = *itMax;
}

/**
 * threaded version of std::memcpy
 * @param dest
 * @param source
 * @param count total number of bytes (as in memcpy)
 * @param nthreads
 */
template <class T, class U>
void MemcpyThreads(T *destination, const U *source, size_t count,
                   const unsigned int threads = 1)
{
    // do not decompose tasks to less than 4MB pieces
    const size_t minBlockSize = 4194304;
    const size_t maxNThreads = std::max((size_t)threads, count / minBlockSize);

    if (maxNThreads == 1)
    {
        std::memcpy(destination, source, count);
        return;
    }

    const std::size_t stride = count / maxNThreads;
    const std::size_t remainder = count % maxNThreads;
    const std::size_t last = stride + remainder;

    std::vector<std::thread> memcpyThreads;
    memcpyThreads.reserve(maxNThreads);

    for (unsigned int t = 0; t < maxNThreads; ++t)
    {
        const size_t initialDestination = stride * t / sizeof(T);
        const size_t initialSource = stride * t / sizeof(U);

        if (t == maxNThreads - 1)
            memcpyThreads.push_back(
                std::thread(std::memcpy, &destination[initialDestination],
                            &source[initialSource], last));
        else
            memcpyThreads.push_back(
                std::thread(std::memcpy, &destination[initialDestination],
                            &source[initialSource], stride));
    }
    // Now join the threads (is this really needed?)
    for (auto &thread : memcpyThreads)
        thread.join();
}

template <class T>
void MemcpyToBuffer(std::vector<char> &raw, std::size_t &position,
                    const T *source, std::size_t size) noexcept
{
    std::memcpy(&raw[position], source, size);
    position += size;
}

/**
 * Version that pushed to the end of the buffer, updates vec.size()
 * automatically
 * @param raw
 * @param source using pointer notation
 * @param elements
 */
template <class T>
void InsertToBuffer(std::vector<char> &buffer, const T *source,
                    const std::size_t elements = 1) noexcept
{
    const char *src = reinterpret_cast<const char *>(source);
    buffer.insert(buffer.end(), src, src + elements * sizeof(T));
}

/**
 * Copies data to a specific location in the buffer.
 * Updates position.
 * Does not update vec.size().
 * @param raw
 * @param position
 * @param source
 * @param elements
 */
template <class T>
void CopyToBuffer(std::vector<char> &buffer, size_t &position, const T *source,
                  const size_t elements = 1) noexcept
{
    const char *src = reinterpret_cast<const char *>(source);
    std::copy(src, src + elements * sizeof(T), buffer.begin() + position);
    position += elements * sizeof(T);
}

template <class T>
void CopyToBufferThreads(std::vector<char> &buffer, size_t &position,
                         const T *source, const size_t elements = 1,
                         const unsigned int threads = 1) noexcept
{
    if (threads == 1)
    {
        CopyToBuffer(buffer, position, source, elements);
        return;
    }

    const size_t stride = elements / threads;    // elements per thread
    const size_t remainder = elements % threads; // remainder if not aligned
    const size_t last = stride + remainder;

    std::vector<std::thread> copyThreads;
    copyThreads.reserve(threads);

    for (unsigned int t = 0; t < threads; ++t)
    {
        size_t bufferPosition = stride * t * sizeof(T);
        const size_t sourcePosition = stride * t;

        if (t == threads - 1) // last thread takes stride + remainder
        {
            copyThreads.push_back(std::thread(CopyToBuffer<T>, std::ref(buffer),
                                              std::ref(bufferPosition),
                                              &source[sourcePosition], last));
            position = bufferPosition; // last position
        }
        else
        {
            copyThreads.push_back(std::thread(CopyToBuffer<T>, std::ref(buffer),
                                              std::ref(bufferPosition),
                                              &source[sourcePosition], stride));
        }
    }

    for (auto &copyThread : copyThreads)
    {
        copyThread.join();
    }
}

template <class T>
void CopyFromBuffer(T *destination, std::size_t elements,
                    const std::vector<char> &raw,
                    std::size_t &position) noexcept
{
    std::copy(raw.begin() + position,
              raw.begin() + position + sizeof(T) * elements,
              reinterpret_cast<char *>(destination));
    position += elements * sizeof(T);
}

template <class T>
void PrintValues(const std::string name, const char *buffer,
                 const std::size_t position, const std::size_t elements)
{
    std::vector<T> values(elements);
    std::memcpy(values.data(), &buffer[position], elements * sizeof(T));

    std::cout << "Read " << name << "\n";
    for (const auto value : values)
        std::cout << value << " ";

    std::cout << "\n";
}

} // end namespace

#endif /* ADIOS2_CORE_ADIOSTEMPLATES_H_ */
