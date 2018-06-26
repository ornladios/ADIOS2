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
namespace helper
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
    const uint64_t element64 = static_cast<uint64_t>(element);
    InsertToBuffer(buffer, &element64, 1);
}

template <class T>
T ReadValue(const std::vector<char> &buffer, size_t &position) noexcept
{
    T value;
    CopyFromBuffer(buffer, position, &value);
    return value;
}

template <class T>
void ClipContiguousMemory(T *dest, const Dims &destStart, const Dims &destCount,
                          const std::vector<char> &contiguousMemory,
                          const Box<Dims> &blockBox,
                          const Box<Dims> &intersectionBox,
                          const bool isRowMajor, const bool reverseDimensions)
{
    auto lf_ClipRowMajor = [](
        T *dest, const Dims &destStart, const Dims &destCount,
        const std::vector<char> &contiguousMemory, const Box<Dims> &blockBox,
        const Box<Dims> &intersectionBox, const bool isRowMajor,
        const bool reverseDimensions) {

        const Dims &start = intersectionBox.first;
        const Dims &end = intersectionBox.second;
        const size_t stride = (end.back() - start.back() + 1) * sizeof(T);

        Dims currentPoint(start); // current point for memory copy
        const Box<Dims> selectionBox =
            helper::StartEndBox(destStart, destCount, reverseDimensions);

        const size_t dimensions = start.size();
        bool run = true;

        const size_t intersectionStart =
            helper::LinearIndex(blockBox, intersectionBox.first, true) *
            sizeof(T);

        while (run)
        {
            // here copy current linear memory between currentPoint and end
            const size_t contiguousStart =
                helper::LinearIndex(blockBox, currentPoint, true) * sizeof(T) -
                intersectionStart;

            const size_t variableStart =
                helper::LinearIndex(selectionBox, currentPoint, true) *
                sizeof(T);

            char *rawVariableData = reinterpret_cast<char *>(dest);

            std::copy(contiguousMemory.begin() + contiguousStart,
                      contiguousMemory.begin() + contiguousStart + stride,
                      rawVariableData + variableStart);

            // here update each index recursively, always starting from the 2nd
            // fastest changing index, since fastest changing index is the
            // continuous part in the previous std::copy
            size_t p = dimensions - 2;
            while (true)
            {
                ++currentPoint[p];
                if (currentPoint[p] > end[p])
                {
                    if (p == 0)
                    {
                        run = false; // we are done
                        break;
                    }
                    else
                    {
                        currentPoint[p] = start[p];
                        --p;
                    }
                }
                else
                {
                    break; // break inner p loop
                }
            } // dimension index update
        }     // run

    };

    auto lf_ClipColumnMajor =
        [](T *dest, const Dims &destStart, const Dims &destCount,
           const std::vector<char> &contiguousMemory, const Box<Dims> &blockBox,
           const Box<Dims> &intersectionBox, const bool isRowMajor,
           const bool reverseDimensions)

    {
        const Dims &start = intersectionBox.first;
        const Dims &end = intersectionBox.second;
        const size_t stride = (end.front() - start.front() + 1) * sizeof(T);

        Dims currentPoint(start); // current point for memory copy

        const Box<Dims> selectionBox =
            helper::StartEndBox(destStart, destCount, reverseDimensions);

        const size_t dimensions = start.size();
        bool run = true;

        const size_t intersectionStart =
            helper::LinearIndex(blockBox, intersectionBox.first, false) *
            sizeof(T);

        while (run)
        {
            // here copy current linear memory between currentPoint and end
            const size_t contiguousStart =
                helper::LinearIndex(blockBox, currentPoint, false) * sizeof(T) -
                intersectionStart;

            const size_t variableStart =
                helper::LinearIndex(selectionBox, currentPoint, false) *
                sizeof(T);

            char *rawVariableData = reinterpret_cast<char *>(dest);

            std::copy(contiguousMemory.begin() + contiguousStart,
                      contiguousMemory.begin() + contiguousStart + stride,
                      rawVariableData + variableStart);

            // here update each index recursively, always starting from the 2nd
            // fastest changing index, since fastest changing index is the
            // continuous part in the previous std::copy
            size_t p = 1;
            while (true)
            {
                ++currentPoint[p];
                if (currentPoint[p] > end[p])
                {
                    if (p == dimensions - 1)
                    {
                        run = false; // we are done
                        break;
                    }
                    currentPoint[p] = start[p];
                    ++p;
                }
                else
                {
                    break; // break inner p loop
                }
            } // dimension index update
        }

    };

    const Dims &start = intersectionBox.first;
    if (start.size() == 1) // 1D copy memory
    {
        // normalize intersection start with variable.m_Start
        const size_t normalizedStart =
            (start.front() - destStart.front()) * sizeof(T);
        char *rawVariableData = reinterpret_cast<char *>(dest);

        std::copy(contiguousMemory.begin(), contiguousMemory.end(),
                  rawVariableData + normalizedStart);
        return;
    }

    if (isRowMajor) // stored with C, C++, Python
    {
        lf_ClipRowMajor(dest, destStart, destCount, contiguousMemory, blockBox,
                        intersectionBox, isRowMajor, reverseDimensions);
    }
    else // stored with Fortran, R
    {
        lf_ClipColumnMajor(dest, destStart, destCount, contiguousMemory,
                           blockBox, intersectionBox, isRowMajor,
                           reverseDimensions);
    }
}

template <class T>
void Resize(std::vector<T> &vec, const size_t dataSize, const bool debugMode,
            const std::string hint, T value)
{
    if (debugMode)
    {
        try
        {
            // avoid power of 2 capacity growth
            vec.reserve(dataSize);
            vec.resize(dataSize, value);
        }
        catch (...)
        {
            std::throw_with_nested(std::runtime_error(
                "ERROR: buffer overflow when resizing to " +
                std::to_string(dataSize) + " bytes, " + hint + "\n"));
        }
    }
    else
    {
        vec.reserve(dataSize);
        vec.resize(dataSize, value);
    }
}

} // end namespace helper
} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSMEMORY_INL_ */
