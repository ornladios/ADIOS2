/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosMemory.inl definition of template functions in adiosMemory.h
 *
 *  Created on: May 17, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_HELPER_ADIOSMEMORY_INL_
#define ADIOS2_HELPER_ADIOSMEMORY_INL_
#ifndef ADIOS2_HELPER_ADIOSMEMORY_H_
#error "Inline file should only be included from it's header, never on it's own"
#endif

#include "adiosMemory.h"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <algorithm> //std::copy
#include <cstring>   //std::memcpy
#include <thread>
/// \endcond

namespace adios
{

template <class T>
void InsertToBuffer(std::vector<char> &buffer, const T *source,
                    const std::size_t elements) noexcept
{
    const char *src = reinterpret_cast<const char *>(source);
    buffer.insert(buffer.end(), src, src + elements * sizeof(T));
}

template <class T>
void CopyToBuffer(std::vector<char> &buffer, size_t &position, const T *source,
                  const size_t elements) noexcept
{
    const char *src = reinterpret_cast<const char *>(source);
    std::copy(src, src + elements * sizeof(T), buffer.begin() + position);
    position += elements * sizeof(T);
}

template <class T>
void CopyToBufferThreads(std::vector<char> &buffer, size_t &position,
                         const T *source, const size_t elements,
                         const unsigned int threads) noexcept
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
void CopyFromBuffer(T *destination, size_t elements,
                    const std::vector<char> &buffer, size_t &position) noexcept
{
    std::copy(buffer.begin() + position,
              buffer.begin() + position + sizeof(T) * elements,
              reinterpret_cast<char *>(destination));
    position += elements * sizeof(T);
}

template <class T>
void MemcpyToBuffer(std::vector<char> &buffer, size_t &position,
                    const T *source, size_t size) noexcept
{
    std::memcpy(&buffer[position], source, size);
    position += size;
}

template <class T>
void MemcpyToBufferThreads(std::vector<char> &buffer, size_t &position,
                           const T *source, size_t size,
                           const unsigned int threads)
{
    if (threads == 1)
    {
        std::memcpy(&buffer[position], source, size);
        return;
    }

    const size_t stride = size / threads;
    const size_t remainder = size % threads;
    const size_t last = stride + remainder;

    std::vector<std::thread> memcpyThreads;
    memcpyThreads.reserve(threads);

    for (unsigned int t = 0; t < threads; ++t)
    {
        const size_t initialDestination = position + stride * t;
        const size_t initialSource = stride * t / sizeof(T);

        if (t == threads - 1)
        {
            memcpyThreads.push_back(std::thread(std::memcpy,
                                                &buffer[initialDestination],
                                                &source[initialSource], last));
        }
        else
        {
            memcpyThreads.push_back(
                std::thread(std::memcpy, &buffer[initialDestination],
                            &source[initialSource], stride));
        }
    }

    for (auto &thread : memcpyThreads)
    {
        thread.join();
    }
}

} // end namespace

#endif /* ADIOS2_HELPER_ADIOSMEMORY_INL_ */
