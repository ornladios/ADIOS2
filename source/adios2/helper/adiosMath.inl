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

#include <algorithm> //std::minmax_element, std::min_element, std::max_element
#include <thread>

namespace adios2
{

template <class T>
void GetMinMax(const T *values, const size_t size, T &min, T &max) noexcept
{
    auto bounds = std::minmax_element(values, values + size);
    min = *bounds.first;
    max = *bounds.second;
}

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
void GetMinMaxThreads(const T *values, const size_t size, T &min, T &max,
                      const unsigned int threads) noexcept
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
                std::thread(adios2::GetMinMax<T>, &values[position], last,
                            std::ref(mins[t]), std::ref(maxs[t])));
        }
        else
        {
            getMinMaxThreads.push_back(
                std::thread(adios2::GetMinMax<T>, &values[position], stride,
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
void GetMinMaxThreads(const std::complex<T> *values, const size_t size, T &min,
                      T &max, const unsigned int threads) noexcept
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

} // end namespace adios

#endif /* ADIOS2_HELPER_ADIOSMATH_INL_ */
