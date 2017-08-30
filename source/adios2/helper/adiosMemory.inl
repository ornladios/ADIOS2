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

/// \cond EXCLUDE_FROM_DOXYGEN
#include <algorithm> //std::copy
#include <cstring>   //std::memcpy
#include <thread>
/// \endcond

namespace adios2
{

template <class T>
void InsertToBuffer(std::vector<char> &buffer, const T *source,
                    const size_t elements) noexcept
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
    if (threads == 1 || threads > elements)
    {
        CopyToBuffer(buffer, position, source, elements);
        return;
    }

    const size_t stride = elements / threads;    // elements per thread
    const size_t remainder = elements % threads; // remainder if not aligned
    const size_t last = stride + remainder;

    std::vector<std::thread> copyThreads;
    copyThreads.reserve(threads);

    const char *src = reinterpret_cast<const char *>(source);

    for (unsigned int t = 0; t < threads; ++t)
    {
        const size_t bufferStart = position + stride * t * sizeof(T);
        const size_t srcStart = stride * t * sizeof(T);
        if (t == threads - 1) // last thread takes stride + remainder
        {
            copyThreads.push_back(std::thread(std::memcpy, &buffer[bufferStart],
                                              &src[srcStart],
                                              last * sizeof(T)));
            // std::copy not working properly with std::thread...why?
            //            copyThreads.push_back(std::thread(std::copy,
            //            &src[srcStart],
            //                                              &src[srcStart] +
            //                                              last * sizeof(T),
            //                                              buffer.begin() +
            //                                              bufferStart));
        }
        else
        {
            copyThreads.push_back(std::thread(std::memcpy, &buffer[bufferStart],
                                              &src[srcStart],
                                              stride * sizeof(T)));
            // std::copy not working properly with std::thread...why?
            //            copyThreads.push_back(std::thread(
            //                std::copy, &src[srcStart], &src[srcStart] + stride
            //                * sizeof(T),
            //                buffer.begin() + bufferStart));
        }
    }

    for (auto &copyThread : copyThreads)
    {
        copyThread.join();
    }

    position += elements * sizeof(T);
}

template <class T>
void CopyFromBuffer(const std::vector<char> &buffer, size_t &position,
                    T *destination, size_t elements) noexcept
{
    std::copy(buffer.begin() + position,
              buffer.begin() + position + sizeof(T) * elements,
              reinterpret_cast<char *>(destination));
    position += elements * sizeof(T);
}

template <class T>
void InsertU64(std::vector<char> &buffer, const T element) noexcept
{
    const uint64_t element64 = static_cast<const uint64_t>(element);
    InsertToBuffer(buffer, &element64);
}

template <class T>
T ReadValue(const std::vector<char> &buffer, size_t &position) noexcept
{
    T value;
    CopyFromBuffer(buffer, position, &value);
    return value;
}

} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSMEMORY_INL_ */
