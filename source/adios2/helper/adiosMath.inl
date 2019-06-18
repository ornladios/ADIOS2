/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosMath.inl
 *
 *  Created on: May 17, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_HELPER_ADIOSMATH_INL_
#define ADIOS2_HELPER_ADIOSMATH_INL_
#ifndef ADIOS2_HELPER_ADIOSMATH_H_
#error "Inline file should only be included from it's header, never on it's own"
#endif

#include <algorithm> // std::minmax_element, std::min_element, std::max_element
                     // std::transform
#include <limits>    //std::numeri_limits
#include <thread>

#include "adios2/common/ADIOSMacros.h"

namespace adios2
{
namespace helper
{

template <class T>
void GetMinMaxSelection(const T *values, const Dims &shape, const Dims &start,
                        const Dims &count, const bool isRowMajor, T &min,
                        T &max) noexcept
{
    auto lf_MinMaxRowMajor = [](const T *values, const Dims &shape,
                                const Dims &start, const Dims &count, T &min,
                                T &max) {
        // loop through selection box contiguous part
        const size_t dimensions = shape.size();
        const size_t stride = count.back();
        const size_t startCoord = dimensions - 2;

        Dims currentPoint(start); // current point for contiguous memory
        bool run = true;
        bool firstStep = true;

        while (run)
        {
            // here copy current linear memory between currentPoint and end
            const size_t startOffset = helper::LinearIndex(
                Dims(shape.size(), 0), shape, currentPoint, true);

            T minStride, maxStride;
            GetMinMax(values + startOffset, stride, minStride, maxStride);

            if (firstStep)
            {
                min = minStride;
                max = maxStride;
                firstStep = false;
            }
            else
            {
                if (LessThan(minStride, min))
                {
                    min = minStride;
                }

                if (GreaterThan(maxStride, max))
                {
                    max = maxStride;
                }
            }

            size_t p = startCoord;
            while (true)
            {
                ++currentPoint[p];
                if (currentPoint[p] > start[p] + count[p] - 1)
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
        }     // end while stride loop
    };

    auto lf_MinMaxColumnMajor = [](const T *values, const Dims &shape,
                                   const Dims &start, const Dims &count, T &min,
                                   T &max) {
        // loop through selection box contiguous part
        const size_t dimensions = shape.size();
        const size_t stride = count.front();
        const size_t startCoord = 1;

        Dims currentPoint(start); // current point for contiguous memory
        bool run = true;
        bool firstStep = true;

        while (run)
        {
            // here copy current linear memory between currentPoint and end
            const size_t startOffset = helper::LinearIndex(
                Dims(shape.size(), 0), shape, currentPoint, false);

            T minStride, maxStride;
            GetMinMax(values + startOffset, stride, minStride, maxStride);

            if (firstStep)
            {
                min = minStride;
                max = maxStride;
                firstStep = false;
            }
            else
            {
                if (LessThan(minStride, min))
                {
                    min = minStride;
                }

                if (GreaterThan(maxStride, max))
                {
                    max = maxStride;
                }
            }

            size_t p = startCoord;

            while (true)
            {
                ++currentPoint[p];
                if (currentPoint[p] > start[p] + count[p] - 1)
                {
                    if (p == dimensions - 1)
                    {
                        run = false; // we are done
                        break;
                    }
                    else
                    {
                        currentPoint[p] = start[p];
                        ++p;
                    }
                }
                else
                {
                    break; // break inner p loop
                }
            } // dimension index update

        } // end while stride loop
    };

    if (shape.size() == 1)
    {
        const size_t startOffset =
            helper::LinearIndex(Dims(1, 0), shape, start, isRowMajor);
        const size_t totalSize = helper::GetTotalSize(count);
        GetMinMax(values + startOffset, totalSize, min, max);
        return;
    }

    if (isRowMajor)
    {
        lf_MinMaxRowMajor(values, shape, start, count, min, max);
    }
    else
    {
        lf_MinMaxColumnMajor(values, shape, start, count, min, max);
    }
}

template <class T>
inline void GetMinMax(const T *values, const size_t size, T &min,
                      T &max) noexcept
{
    auto bounds = std::minmax_element(values, values + size);
    min = *bounds.first;
    max = *bounds.second;
}

template <>
inline void GetMinMax(const std::complex<float> *values, const size_t size,
                      std::complex<float> &min,
                      std::complex<float> &max) noexcept
{
    GetMinMaxComplex(values, size, min, max);
}

template <>
inline void GetMinMax(const std::complex<double> *values, const size_t size,
                      std::complex<double> &min,
                      std::complex<double> &max) noexcept
{
    GetMinMaxComplex(values, size, min, max);
}

template <class T>
void GetMinMaxComplex(const std::complex<T> *values, const size_t size,
                      std::complex<T> &min, std::complex<T> &max) noexcept
{
    min = values[0];
    max = values[0];

    T minNorm = std::norm(values[0]);
    T maxNorm = minNorm;

    for (auto i = 1; i < size; ++i)
    {
        T norm = std::norm(values[i]);

        if (norm < minNorm)
        {
            minNorm = norm;
            min = values[i];
            continue;
        }

        if (norm > maxNorm)
        {
            maxNorm = norm;
            max = values[i];
        }
    }
}

template <class T>
void GetMinMaxThreads(const T *values, const size_t size, T &min, T &max,
                      const unsigned int threads) noexcept
{
    if (size == 0)
    {
        return;
    }

    if (threads == 1 || threads > size)
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
                std::thread(GetMinMax<T>, &values[position], last,
                            std::ref(mins[t]), std::ref(maxs[t])));
        }
        else
        {
            getMinMaxThreads.push_back(
                std::thread(GetMinMax<T>, &values[position], stride,
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

template <class T>
void GetMinMaxThreads(const std::complex<T> *values, const size_t size,
                      std::complex<T> &min, std::complex<T> &max,
                      const unsigned int threads) noexcept
{
    if (size == 0)
    {
        return;
    }

    if (threads == 1)
    {
        GetMinMaxComplex(values, size, min, max);
        return;
    }

    const size_t stride = size / threads;    // elements per thread
    const size_t remainder = size % threads; // remainder if not aligned
    const size_t last = stride + remainder;

    std::vector<std::complex<T>> mins(threads); // zero init
    std::vector<std::complex<T>> maxs(threads); // zero init

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

    std::complex<T> minTemp;
    std::complex<T> maxTemp;

    GetMinMaxComplex(mins.data(), mins.size(), min, maxTemp);
    GetMinMaxComplex(maxs.data(), maxs.size(), minTemp, max);
}

#define declare_template_instantiation(T)                                      \
    template <>                                                                \
    inline bool LessThan<std::complex<T>>(                                     \
        const std::complex<T> input1, const std::complex<T> input2) noexcept   \
    {                                                                          \
        if (std::norm(input1) < std::norm(input2))                             \
        {                                                                      \
            return true;                                                       \
        }                                                                      \
        return false;                                                          \
    }

ADIOS2_FOREACH_COMPLEX_PRIMITIVE_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

template <class T>
inline bool LessThan(const T input1, const T input2) noexcept
{
    if (input1 < input2)
    {
        return true;
    }
    return false;
}

#define declare_template_instantiation(T)                                      \
    template <>                                                                \
    inline bool GreaterThan<std::complex<T>>(                                  \
        const std::complex<T> input1, const std::complex<T> input2) noexcept   \
    {                                                                          \
        if (std::norm(input1) > std::norm(input2))                             \
        {                                                                      \
            return true;                                                       \
        }                                                                      \
        return false;                                                          \
    }

ADIOS2_FOREACH_COMPLEX_PRIMITIVE_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

template <class T>
inline bool GreaterThan(const T input1, const T input2) noexcept
{
    if (input1 > input2)
    {
        return true;
    }
    return false;
}

template <class T>
Dims PayloadDims(const Dims &dimensions, const bool isRowMajor) noexcept
{
    if (dimensions.empty())
    {
        return dimensions;
    }

    Dims payloadDims = dimensions;
    if (isRowMajor)
    {
        payloadDims.back() *= sizeof(T);
    }
    else
    {
        payloadDims.front() *= sizeof(T);
    }
    return payloadDims;
}

template <class T, class BinaryOperation>
std::vector<T> VectorsOp(BinaryOperation op, const std::vector<T> &vector1,
                         const std::vector<T> &vector2) noexcept
{
    std::vector<T> result(vector1.size());
    std::transform(vector1.begin(), vector1.end(), vector2.begin(),
                   result.begin(), op);
    return result;
}

} // end namespace helper
} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSMATH_INL_ */
