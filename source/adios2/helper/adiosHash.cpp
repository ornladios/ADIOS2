/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adiosHash.h"

#define XXH_INLINE_ALL
#include <xxhash.h>

namespace adios2
{
namespace helper
{

std::array<uint8_t, 16> Hash128(const void *data, const size_t size) noexcept
{
    XXH128_canonical_t canonical;
    XXH128_canonicalFromHash(&canonical, XXH3_128bits(data, size));
    std::array<uint8_t, 16> hash;
    for (size_t i = 0; i < hash.size(); ++i)
    {
        hash[i] = canonical.digest[i];
    }
    return hash;
}

std::string Hash128ToHex(const std::array<uint8_t, 16> &hash)
{
    static const char hex[] = "0123456789abcdef";
    std::string result(32, '0');
    for (size_t i = 0; i < hash.size(); ++i)
    {
        result[2 * i] = hex[hash[i] >> 4];
        result[2 * i + 1] = hex[hash[i] & 0xf];
    }
    return result;
}

} // end namespace helper
} // end namespace adios2
