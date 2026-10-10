/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_HELPER_ADIOSHASH_H_
#define ADIOS2_HELPER_ADIOSHASH_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace adios2
{
namespace helper
{

/** Non-cryptographic 128-bit hash (XXH3) of a block of memory, big-endian byte order */
std::array<uint8_t, 16> Hash128(const void *data, const size_t size) noexcept;

/** Convert a 16-byte hash to a 32 character lowercase hexadecimal string */
std::string Hash128ToHex(const std::array<uint8_t, 16> &hash);

} // end namespace helper
} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSHASH_H_ */
