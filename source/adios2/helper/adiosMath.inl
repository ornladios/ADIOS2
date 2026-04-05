/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
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

#ifdef __aarch64__
#include <arm_neon.h>
#elif defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#endif

#include "adios2/common/ADIOSMacros.h"
#include "adiosLog.h"

namespace adios2
{
namespace helper
{

template <class T>
void GetMinMaxSelection(const T *values, const Dims &shape, const Dims &start, const Dims &count,
                        const bool isRowMajor, T &min, T &max, const MemorySpace memSpace) noexcept
{
    auto lf_MinMaxRowMajor = [](const T *values, const Dims &shape, const Dims &start,
                                const Dims &count, T &min, T &max, const MemorySpace memSpace) {
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
            const size_t startOffset =
                helper::LinearIndex(Dims(shape.size(), 0), shape, currentPoint, true);

            T minStride, maxStride;
            GetMinMax(values + startOffset, stride, minStride, maxStride, memSpace);

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

    auto lf_MinMaxColumnMajor = [](const T *values, const Dims &shape, const Dims &start,
                                   const Dims &count, T &min, T &max, const MemorySpace memSpace) {
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
            const size_t startOffset =
                helper::LinearIndex(Dims(shape.size(), 0), shape, currentPoint, false);

            T minStride, maxStride;
            GetMinMax(values + startOffset, stride, minStride, maxStride, memSpace);

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
        const size_t startOffset = helper::LinearIndex(Dims(1, 0), shape, start, isRowMajor);
        const size_t totalSize = helper::GetTotalSize(count);
        GetMinMax(values + startOffset, totalSize, min, max, memSpace);
        return;
    }

    if (isRowMajor)
    {
        lf_MinMaxRowMajor(values, shape, start, count, min, max, memSpace);
    }
    else
    {
        lf_MinMaxColumnMajor(values, shape, start, count, min, max, memSpace);
    }
}

template <class T>
inline void GetMinMax(const T *values, const size_t size, T &min, T &max,
                      const MemorySpace memSpace) noexcept
{
#ifdef ADIOS2_HAVE_GPU_SUPPORT
    if (memSpace == MemorySpace::GPU)
    {
        GetGPUMinMax(values, size, min, max);
        return;
    }
#endif
    auto bounds = std::minmax_element(values, values + size);
    min = *bounds.first;
    max = *bounds.second;
}

// SIMD specializations for min/max. std::minmax_element cannot be vectorized because it
// tracks element positions. These value-only reductions use platform SIMD intrinsics and
// run at memory bandwidth. When porting to a new architecture (e.g. Power/VSX, RISC-V V),
// add a specialization here with the appropriate intrinsics; the #else scalar fallback
// will be used otherwise.
//
// Covered types: float, double, int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t.
// Not covered (use scalar fallback): int64_t, uint64_t (no SIMD min/max before AVX-512),
// long double (80-bit, no SIMD), complex (ordering by std::norm).

// Helper: scalar min/max loop used by SIMD tail and scalar-only fallback.
template <class T>
inline void GetMinMaxScalar(const T *values, size_t start, const size_t size, T &min,
                            T &max) noexcept
{
    for (size_t i = start; i < size; ++i)
    {
        if (values[i] < min)
            min = values[i];
        if (values[i] > max)
            max = values[i];
    }
}

#ifdef __aarch64__
// clang-format off
// NEON macro: generates a GetMinMax specialization for an integer type.
// VTYPE: NEON vector type, LOAD: load intrinsic, MIN/MAX: min/max intrinsics,
// DUP: broadcast intrinsic, REDUCE_MIN/MAX: horizontal reduce intrinsics,
// COUNT: elements per vector pair iteration.
#define ADIOS2_NEON_MINMAX_INT(CTYPE, VTYPE, LOAD, MIN, MAX, DUP, REDUCE_MIN, REDUCE_MAX, COUNT)  \
    template <>                                                                                    \
    inline void GetMinMax(const CTYPE *values, const size_t size, CTYPE &min, CTYPE &max,         \
                          const MemorySpace) noexcept                                              \
    {                                                                                              \
        if (size == 0) return;                                                                     \
        size_t i = 0;                                                                              \
        VTYPE vmin = DUP(values[0]);                                                               \
        VTYPE vmax = DUP(values[0]);                                                               \
        VTYPE vmin2 = vmin, vmax2 = vmax;                                                          \
        const size_t endN = size - (size % (COUNT));                                               \
        for (; i < endN; i += (COUNT))                                                             \
        {                                                                                          \
            VTYPE a = LOAD(values + i);                                                            \
            VTYPE b = LOAD(values + i + (COUNT) / 2);                                              \
            vmin = MIN(vmin, a); vmax = MAX(vmax, a);                                              \
            vmin2 = MIN(vmin2, b); vmax2 = MAX(vmax2, b);                                          \
        }                                                                                          \
        vmin = MIN(vmin, vmin2); vmax = MAX(vmax, vmax2);                                          \
        min = REDUCE_MIN(vmin); max = REDUCE_MAX(vmax);                                            \
        GetMinMaxScalar(values, i, size, min, max);                                                \
    }

ADIOS2_NEON_MINMAX_INT(int8_t,   int8x16_t,  vld1q_s8,  vminq_s8,  vmaxq_s8,  vdupq_n_s8,  vminvq_s8,  vmaxvq_s8,  32)
ADIOS2_NEON_MINMAX_INT(uint8_t,  uint8x16_t, vld1q_u8,  vminq_u8,  vmaxq_u8,  vdupq_n_u8,  vminvq_u8,  vmaxvq_u8,  32)
ADIOS2_NEON_MINMAX_INT(int16_t,  int16x8_t,  vld1q_s16, vminq_s16, vmaxq_s16, vdupq_n_s16, vminvq_s16, vmaxvq_s16, 16)
ADIOS2_NEON_MINMAX_INT(uint16_t, uint16x8_t, vld1q_u16, vminq_u16, vmaxq_u16, vdupq_n_u16, vminvq_u16, vmaxvq_u16, 16)
ADIOS2_NEON_MINMAX_INT(int32_t,  int32x4_t,  vld1q_s32, vminq_s32, vmaxq_s32, vdupq_n_s32, vminvq_s32, vmaxvq_s32, 8)
ADIOS2_NEON_MINMAX_INT(uint32_t, uint32x4_t, vld1q_u32, vminq_u32, vmaxq_u32, vdupq_n_u32, vminvq_u32, vmaxvq_u32, 8)
#undef ADIOS2_NEON_MINMAX_INT
// clang-format on

#elif defined(__x86_64__) || defined(_M_X64)
// SSE2 integer min/max via compare+select (blendv requires SSE4.1, so we use and/andnot/or).
// mask = cmpgt(a,b) selects: min = (b & mask) | (a & ~mask), max = (a & mask) | (b & ~mask)
#define ADIOS2_SSE2_MINMAX_INT(CTYPE, VTYPE, LOAD, STORE, SET1, CMPGT, AND, ANDNOT, OR, COUNT)     \
    template <>                                                                                    \
    inline void GetMinMax(const CTYPE *values, const size_t size, CTYPE &min, CTYPE &max,          \
                          const MemorySpace) noexcept                                              \
    {                                                                                              \
        if (size == 0)                                                                             \
            return;                                                                                \
        size_t i = 0;                                                                              \
        VTYPE vmin = SET1(values[0]);                                                              \
        VTYPE vmax = SET1(values[0]);                                                              \
        const size_t endN = size - (size % (COUNT));                                               \
        for (; i < endN; i += (COUNT))                                                             \
        {                                                                                          \
            VTYPE v = LOAD(reinterpret_cast<const __m128i *>(values + i));                         \
            VTYPE gt = CMPGT(v, vmax);                                                             \
            vmax = OR(AND(v, gt), ANDNOT(gt, vmax));                                               \
            VTYPE lt = CMPGT(vmin, v);                                                             \
            vmin = OR(AND(v, lt), ANDNOT(lt, vmin));                                               \
        }                                                                                          \
        CTYPE tmin[16 / sizeof(CTYPE)], tmax[16 / sizeof(CTYPE)];                                  \
        STORE(reinterpret_cast<__m128i *>(tmin), vmin);                                            \
        STORE(reinterpret_cast<__m128i *>(tmax), vmax);                                            \
        min = tmin[0];                                                                             \
        max = tmax[0];                                                                             \
        for (int j = 1; j < (int)(16 / sizeof(CTYPE)); ++j)                                        \
        {                                                                                          \
            if (tmin[j] < min)                                                                     \
                min = tmin[j];                                                                     \
            if (tmax[j] > max)                                                                     \
                max = tmax[j];                                                                     \
        }                                                                                          \
        GetMinMaxScalar(values, i, size, min, max);                                                \
    }

// Signed types use _mm_cmpgt_epiN directly.
ADIOS2_SSE2_MINMAX_INT(int8_t, __m128i, _mm_loadu_si128, _mm_storeu_si128, _mm_set1_epi8,
                       _mm_cmpgt_epi8, _mm_and_si128, _mm_andnot_si128, _mm_or_si128, 16)
ADIOS2_SSE2_MINMAX_INT(int16_t, __m128i, _mm_loadu_si128, _mm_storeu_si128, _mm_set1_epi16,
                       _mm_cmpgt_epi16, _mm_and_si128, _mm_andnot_si128, _mm_or_si128, 8)
ADIOS2_SSE2_MINMAX_INT(int32_t, __m128i, _mm_loadu_si128, _mm_storeu_si128, _mm_set1_epi32,
                       _mm_cmpgt_epi32, _mm_and_si128, _mm_andnot_si128, _mm_or_si128, 4)

// Unsigned types: SSE2 only has signed cmpgt. XOR with sign bit to convert unsigned
// comparison to signed, then use signed cmpgt.
namespace detail
{
inline __m128i unsigned_cmpgt_epi8(__m128i a, __m128i b)
{
    const __m128i bias = _mm_set1_epi8(static_cast<char>(0x80));
    return _mm_cmpgt_epi8(_mm_xor_si128(a, bias), _mm_xor_si128(b, bias));
}
inline __m128i unsigned_cmpgt_epi16(__m128i a, __m128i b)
{
    const __m128i bias = _mm_set1_epi16(static_cast<short>(0x8000));
    return _mm_cmpgt_epi16(_mm_xor_si128(a, bias), _mm_xor_si128(b, bias));
}
inline __m128i unsigned_cmpgt_epi32(__m128i a, __m128i b)
{
    const __m128i bias = _mm_set1_epi32(static_cast<int>(0x80000000));
    return _mm_cmpgt_epi32(_mm_xor_si128(a, bias), _mm_xor_si128(b, bias));
}
} // namespace detail

ADIOS2_SSE2_MINMAX_INT(uint8_t, __m128i, _mm_loadu_si128, _mm_storeu_si128, _mm_set1_epi8,
                       detail::unsigned_cmpgt_epi8, _mm_and_si128, _mm_andnot_si128, _mm_or_si128,
                       16)
ADIOS2_SSE2_MINMAX_INT(uint16_t, __m128i, _mm_loadu_si128, _mm_storeu_si128, _mm_set1_epi16,
                       detail::unsigned_cmpgt_epi16, _mm_and_si128, _mm_andnot_si128, _mm_or_si128,
                       8)
ADIOS2_SSE2_MINMAX_INT(uint32_t, __m128i, _mm_loadu_si128, _mm_storeu_si128, _mm_set1_epi32,
                       detail::unsigned_cmpgt_epi32, _mm_and_si128, _mm_andnot_si128, _mm_or_si128,
                       4)

#undef ADIOS2_SSE2_MINMAX_INT
#endif
template <>
inline void GetMinMax(const double *values, const size_t size, double &min, double &max,
                      const MemorySpace memSpace) noexcept
{
    if (size == 0)
        return;
#ifdef ADIOS2_HAVE_GPU_SUPPORT
    if (memSpace == MemorySpace::GPU)
    {
        GetGPUMinMax(values, size, min, max);
        return;
    }
#endif
    size_t i = 0;
#ifdef __aarch64__
    float64x2_t vmin = vdupq_n_f64(values[0]);
    float64x2_t vmax = vdupq_n_f64(values[0]);
    float64x2_t vmin2 = vmin, vmax2 = vmax;
    const size_t end4 = size - (size % 4);
    for (; i < end4; i += 4)
    {
        float64x2_t a = vld1q_f64(values + i);
        float64x2_t b = vld1q_f64(values + i + 2);
        vmin = vminq_f64(vmin, a);
        vmax = vmaxq_f64(vmax, a);
        vmin2 = vminq_f64(vmin2, b);
        vmax2 = vmaxq_f64(vmax2, b);
    }
    vmin = vminq_f64(vmin, vmin2);
    vmax = vmaxq_f64(vmax, vmax2);
    min = vminvq_f64(vmin);
    max = vmaxvq_f64(vmax);
#elif defined(__x86_64__) || defined(_M_X64)
    __m128d vmin = _mm_set1_pd(values[0]);
    __m128d vmax = _mm_set1_pd(values[0]);
    __m128d vmin2 = vmin, vmax2 = vmax;
    const size_t end4 = size - (size % 4);
    for (; i < end4; i += 4)
    {
        __m128d a = _mm_loadu_pd(values + i);
        __m128d b = _mm_loadu_pd(values + i + 2);
        vmin = _mm_min_pd(vmin, a);
        vmax = _mm_max_pd(vmax, a);
        vmin2 = _mm_min_pd(vmin2, b);
        vmax2 = _mm_max_pd(vmax2, b);
    }
    vmin = _mm_min_pd(vmin, vmin2);
    vmax = _mm_max_pd(vmax, vmax2);
    double tmin[2], tmax[2];
    _mm_storeu_pd(tmin, vmin);
    _mm_storeu_pd(tmax, vmax);
    min = (std::min)(tmin[0], tmin[1]);
    max = (std::max)(tmax[0], tmax[1]);
#else
    min = values[0];
    max = values[0];
#endif
    GetMinMaxScalar(values, i, size, min, max);
}

template <>
inline void GetMinMax(const float *values, const size_t size, float &min, float &max,
                      const MemorySpace memSpace) noexcept
{
    if (size == 0)
        return;
#ifdef ADIOS2_HAVE_GPU_SUPPORT
    if (memSpace == MemorySpace::GPU)
    {
        GetGPUMinMax(values, size, min, max);
        return;
    }
#endif
    size_t i = 0;
#ifdef __aarch64__
    float32x4_t vmin = vdupq_n_f32(values[0]);
    float32x4_t vmax = vdupq_n_f32(values[0]);
    float32x4_t vmin2 = vmin, vmax2 = vmax;
    const size_t end8 = size - (size % 8);
    for (; i < end8; i += 8)
    {
        float32x4_t a = vld1q_f32(values + i);
        float32x4_t b = vld1q_f32(values + i + 4);
        vmin = vminq_f32(vmin, a);
        vmax = vmaxq_f32(vmax, a);
        vmin2 = vminq_f32(vmin2, b);
        vmax2 = vmaxq_f32(vmax2, b);
    }
    vmin = vminq_f32(vmin, vmin2);
    vmax = vmaxq_f32(vmax, vmax2);
    min = vminvq_f32(vmin);
    max = vmaxvq_f32(vmax);
#elif defined(__x86_64__) || defined(_M_X64)
    __m128 vmin = _mm_set1_ps(values[0]);
    __m128 vmax = _mm_set1_ps(values[0]);
    __m128 vmin2 = vmin, vmax2 = vmax;
    const size_t end8 = size - (size % 8);
    for (; i < end8; i += 8)
    {
        __m128 a = _mm_loadu_ps(values + i);
        __m128 b = _mm_loadu_ps(values + i + 4);
        vmin = _mm_min_ps(vmin, a);
        vmax = _mm_max_ps(vmax, a);
        vmin2 = _mm_min_ps(vmin2, b);
        vmax2 = _mm_max_ps(vmax2, b);
    }
    vmin = _mm_min_ps(vmin, vmin2);
    vmax = _mm_max_ps(vmax, vmax2);
    float tmin[4], tmax[4];
    _mm_storeu_ps(tmin, vmin);
    _mm_storeu_ps(tmax, vmax);
    min = (std::min)({tmin[0], tmin[1], tmin[2], tmin[3]});
    max = (std::max)({tmax[0], tmax[1], tmax[2], tmax[3]});
#else
    min = values[0];
    max = values[0];
#endif
    GetMinMaxScalar(values, i, size, min, max);
}

template <>
inline void GetMinMax(const std::complex<float> *values, const size_t size,
                      std::complex<float> &min, std::complex<float> &max,
                      const MemorySpace memSpace) noexcept
{
#ifdef ADIOS2_HAVE_GPU_SUPPORT
    if (memSpace == MemorySpace::GPU)
    {
        helper::Throw<std::invalid_argument>(
            "Helper", "MathFunctions", "GetMinMaxThreads",
            "Computing MinMax for GPU arrays of complex numbers is not "
            "supported");
    }
#endif
    GetMinMaxComplex(values, size, min, max);
}

template <>
inline void GetMinMax(const std::complex<double> *values, const size_t size,
                      std::complex<double> &min, std::complex<double> &max,
                      const MemorySpace memSpace) noexcept
{
#ifdef ADIOS2_HAVE_GPU_SUPPORT
    if (memSpace == MemorySpace::GPU)
    {
        helper::Throw<std::invalid_argument>(
            "Helper", "MathFunctions", "GetMinMaxThreads",
            "Computing MinMax for GPU arrays of complex numbers is not "
            "supported");
    }
#endif
    GetMinMaxComplex(values, size, min, max);
}

template <class T>
void GetMinMaxComplex(const std::complex<T> *values, const size_t size, std::complex<T> &min,
                      std::complex<T> &max) noexcept
{
    min = values[0];
    max = values[0];

    T minNorm = std::norm(values[0]);
    T maxNorm = minNorm;

    for (size_t i = 1; i < size; ++i)
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
                      const unsigned int threads, const MemorySpace memSpace) noexcept
{
    if (size == 0)
    {
        return;
    }

    if (threads == 1 || size < 1000000)
    {
        GetMinMax(values, size, min, max, memSpace);
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
            getMinMaxThreads.push_back(std::thread(GetMinMax<T>, &values[position], last,
                                                   std::ref(mins[t]), std::ref(maxs[t]), memSpace));
        }
        else
        {
            getMinMaxThreads.push_back(std::thread(GetMinMax<T>, &values[position], stride,
                                                   std::ref(mins[t]), std::ref(maxs[t]), memSpace));
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
void GetMinMaxThreads(const std::complex<T> *values, const size_t size, std::complex<T> &min,
                      std::complex<T> &max, const unsigned int threads,
                      MemorySpace memSpace) noexcept
{
#ifdef ADIOS2_HAVE_GPU_SUPPORT
    if (memSpace == MemorySpace::GPU)
    {
        helper::Throw<std::invalid_argument>(
            "Helper", "MathFunctions", "GetMinMaxThreads",
            "Computing MinMax for GPU arrays of complex numbers is not "
            "supported");
    }
#endif

    if (size == 0)
    {
        return;
    }

    if (threads == 1 || size < 1000000)
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
            getMinMaxThreads.push_back(std::thread(GetMinMaxComplex<T>, &values[position], last,
                                                   std::ref(mins[t]), std::ref(maxs[t])));
        }
        else
        {
            getMinMaxThreads.push_back(std::thread(GetMinMaxComplex<T>, &values[position], stride,
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

template <class T>
void GetMinMaxSubblocks(const T *values, const Dims &count, const BlockDivisionInfo &info,
                        std::vector<T> &MinMaxs, T &bmin, T &bmax, const unsigned int threads,
                        const MemorySpace memSpace) noexcept
{
    const int ndim = static_cast<int>(count.size());
    const size_t nElems = helper::GetTotalSize(count);
    if (info.NBlocks <= 1)
    {
        MinMaxs.resize(2);
        // account for span
        if (values == nullptr)
        {
            return;
        }
        GetMinMaxThreads(values, nElems, bmin, bmax, threads, memSpace);
        MinMaxs[0] = bmin;
        MinMaxs[1] = bmax;
    }
    else
    {
        MinMaxs.resize(2 * info.NBlocks);
        // account for span
        if (values == nullptr)
        {
            return;
        }

        // Calculate min/max for each block separately
        for (int b = 0; b < info.NBlocks; ++b)
        {
            const Box<Dims> box = GetSubBlock(count, info, b);
            // calculate start position of this subblock in values array
            size_t pos = 0;
            size_t prod = 1;
            for (int d = ndim - 1; d >= 0; --d)
            {
                pos += box.first[d] * prod;
                prod *= count[d];
            }
            T vmin, vmax;
            const size_t nElemsSub = helper::GetTotalSize(box.second);
            GetMinMax(values + pos, nElemsSub, vmin, vmax, memSpace);
            MinMaxs[2 * b] = vmin;
            MinMaxs[2 * b + 1] = vmax;
            if (b == 0)
            {
                bmin = vmin;
                bmax = vmax;
            }
            else
            {
                if (LessThan(vmin, bmin))
                {
                    bmin = vmin;
                }
                if (GreaterThan(vmax, bmax))
                {
                    bmax = vmax;
                }
            }
        }
    }
}

#if 0
template <class T>
void GetMinMaxSubblocks(const T *values, const Dims &count,
                        const size_t subblockSize, std::vector<T> &MinMaxs,
                        std::vector<uint16_t> &SubblockDivs,
                        const unsigned int threads) noexcept
{
    const size_t ndim = count.size();
    const size_t nElems = helper::GetTotalSize(count);
    size_t nBlocks64 = nElems / subblockSize;
    if (nBlocks64 > 4096)
    {
        std::cerr
            << "ADIOS WARNING: The StatsBlockSize parameter is causing a "
               "data block to be divided up to more than 4096 sub-blocks. "
               " This is an artificial limit to avoid metadata explosion."
            << std::endl;
        nBlocks64 = 4096;
    }

    uint16_t nBlocks = static_cast<uint16_t>(nBlocks64);
    if (nBlocks <= 1)
    {
        SubblockDivs.resize(ndim, 1);
        T vmin, vmax;
        GetMinMaxThreads(values, nElems, vmin, vmax, threads);
        MinMaxs.resize(2);
        MinMaxs[0] = vmin;
        MinMaxs[1] = vmax;
    }
    else
    {
        /* Split the block into 'nBlocks' subblocks */
        /* FIXME: What about column-major dimension order here? */
        SubblockDivs.resize(ndim, 1);
        int i = 0;
        uint16_t n = nBlocks;
        size_t dim = count[0];
        uint16_t div = 1;
        // size_t rem = 0;
        while (n > 1 && i < ndim)
        {
            if (n < dim)
            {
                div = n;
                n = 1;
            }
            else
            {
                div = static_cast<uint16_t>(dim);
                // rem = dim % n;
                n = n / dim;
            }
            SubblockDivs[i] = div;
            ++i;
        }

        /* Min/Max per subblock */

        // remainders calculation
        std::vector<uint16_t> rem(ndim, 0);
        for (int j = 0; j < ndim; ++j)
        {
            rem[j] = count[j] % SubblockDivs[j];
        }

        // division vector for calculating N-dim blockIDs from blockID
        std::vector<uint16_t> blockIdDivs(ndim, 0); // blockID in N-dim
        uint16_t d = 1; // div[n-2] * div[n-3] * ... div[0]
        for (int j = ndim - 1; j >= 0; --j)
        {
            blockIdDivs[j] = d;
            d = d * SubblockDivs[j];
        }

        std::vector<uint16_t> blockIds(ndim, 0); // blockID in N-dim
        for (uint16_t b = 0; b < nBlocks; b++)
        {
            // calculate N-dim blockIDs from b
            for (int j = 0; j < ndim; ++j)
            {
                blockIds[j] = b / blockIdDivs[j];
                if (j > 0)
                {
                    blockIds[j] = blockIds[j] % SubblockDivs[j];
                }
            }
            // calcute b-th subblock start/count
            Dims sbCount(ndim, 1);
            Dims sbStart(ndim, 0);
            for (int j = 0; j < ndim; ++j)
            {
                sbCount[j] = count[j] / SubblockDivs[j];
                sbStart[j] = sbCount[j] * blockIds[j];
                if (blockIds[j] < rem[j])
                {
                    sbCount[j] += blockIds[j] + 1;
                    sbStart[j] += blockIds[j];
                }
                else
                {
                    sbStart[j] += rem[j];
                }
            }
        }
    }
}
#endif

#define declare_template_instantiation(T)                                                          \
    template <>                                                                                    \
    inline bool LessThan<std::complex<T>>(const std::complex<T> input1,                            \
                                          const std::complex<T> input2) noexcept                   \
    {                                                                                              \
        if (std::norm(input1) < std::norm(input2))                                                 \
        {                                                                                          \
            return true;                                                                           \
        }                                                                                          \
        return false;                                                                              \
    }

ADIOS2_FOREACH_COMPLEX_PRIMITIVE_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

template <class T>
constexpr bool LessThan(const T input1, const T input2) noexcept
{
    if (input1 < input2)
    {
        return true;
    }
    return false;
}

#define declare_template_instantiation(T)                                                          \
    template <>                                                                                    \
    inline bool GreaterThan<std::complex<T>>(const std::complex<T> input1,                         \
                                             const std::complex<T> input2) noexcept                \
    {                                                                                              \
        if (std::norm(input1) > std::norm(input2))                                                 \
        {                                                                                          \
            return true;                                                                           \
        }                                                                                          \
        return false;                                                                              \
    }

ADIOS2_FOREACH_COMPLEX_PRIMITIVE_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

template <class T>
constexpr bool GreaterThan(const T input1, const T input2) noexcept
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
    std::transform(vector1.begin(), vector1.end(), vector2.begin(), result.begin(), op);
    return result;
}

template <class T>
constexpr T SetWithinLimit(const T value, const T minValue, const T maxValue)
{
    T v = (value < minValue ? minValue : value);
    v = (v > maxValue ? maxValue : v);
    return v;
}

} // end namespace helper
} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSMATH_INL_ */
