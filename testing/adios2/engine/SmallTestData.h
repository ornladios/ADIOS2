/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#ifndef TESTING_ADIOS2_ENGINE_SMALLTESTDATA_H_
#define TESTING_ADIOS2_ENGINE_SMALLTESTDATA_H_

#include <cstdint>

#include <array>
#include <limits>

#ifdef WIN32
#define NOMINMAX
#endif

// Test data for each type.  Make sure our values exceed the range of the
// previous size to make sure we all bytes for each element
struct SmallTestData
{
    // TODO: Fix the right initial value for char array
    std::array<int8_t, 10> I8 = {{0, 1, -2, 3, -4, 5, -6, 7, -8, 9}};
    std::array<int16_t, 10> I16 = {
        {512, 513, -510, 515, -508, 517, -506, 519, -504, 521}};
    std::array<int32_t, 10> I32 = {{131072, 131073, -131070, 131075, -131068,
                                    131077, -131066, 131079, -131064, 131081}};
    std::array<int64_t, 10> I64 = {
        {8589934592, 8589934593, -8589934590, 8589934595, -8589934588,
         8589934597, -8589934586, 8589934599, -8589934584, 8589934601}};
    std::array<uint8_t, 10> U8 = {
        {128, 129, 130, 131, 132, 133, 134, 135, 136, 137}};
    std::array<uint16_t, 10> U16 = {
        {32768, 32769, 32770, 32771, 32772, 32773, 32774, 32775, 32776, 32777}};
    std::array<uint32_t, 10> U32 = {
        {2147483648, 2147483649, 2147483650, 2147483651, 2147483652, 2147483653,
         2147483654, 2147483655, 2147483656, 2147483657}};
    std::array<uint64_t, 10> U64 = {
        {9223372036854775808UL, 9223372036854775809UL, 9223372036854775810UL,
         9223372036854775811UL, 9223372036854775812UL, 9223372036854775813UL,
         9223372036854775814UL, 9223372036854775815UL, 9223372036854775816UL,
         9223372036854775817UL}};
    std::array<float, 10> R32 = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}};
    std::array<double, 10> R64 = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}};
};

// Utility function for generateNewSmallTestData
template <typename T>
T clip(const T &n, const T &lower, const T &upper)
{
    return std::max(lower, std::min(n, upper));
}

SmallTestData generateNewSmallTestData(SmallTestData input, int step, int rank,
                                       int size)
{
    rank++; // Make rank to be 1 based index
    for (int i = 0; i < 10; i++)
    { // Make sure that data is within the range
        int jump = rank + step * size;
        input.I8[i] = clip(
            static_cast<char>(input.I8[i] + static_cast<char>(jump)),
            std::numeric_limits<char>::min(), std::numeric_limits<char>::max());
        input.SI8[i] = clip(static_cast<signed char>(
                                input.SI8[i] + static_cast<signed char>(jump)),
                            std::numeric_limits<signed char>::min(),
                            std::numeric_limits<signed char>::max());
        input.I16[i] =
            clip(static_cast<short>(input.I16[i] + static_cast<short>(jump)),
                 std::numeric_limits<short>::min(),
                 std::numeric_limits<short>::max());
        input.I32[i] = clip(
            static_cast<int>(input.I32[i] + static_cast<int>(jump)),
            std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
        input.I64[i] = clip(
            static_cast<long>(input.I64[i] + static_cast<long>(jump)),
            std::numeric_limits<long>::min(), std::numeric_limits<long>::max());

        input.U8[i] = clip(static_cast<unsigned char>(
                               input.U8[i] + static_cast<unsigned char>(jump)),
                           std::numeric_limits<unsigned char>::min(),
                           std::numeric_limits<unsigned char>::max());
        input.U16[i] =
            clip(static_cast<unsigned short>(input.U16[i] +
                                             static_cast<unsigned short>(jump)),
                 std::numeric_limits<unsigned short>::min(),
                 std::numeric_limits<unsigned short>::max());
        input.U32[i] = clip(static_cast<unsigned int>(
                                input.U32[i] + static_cast<unsigned int>(jump)),
                            std::numeric_limits<unsigned int>::min(),
                            std::numeric_limits<unsigned int>::max());
        input.U64[i] =
            clip(static_cast<unsigned long int>(
                     input.U64[i] + static_cast<unsigned long int>(jump)),
                 std::numeric_limits<unsigned long int>::min(),
                 std::numeric_limits<unsigned long int>::max());

        input.R32[i] =
            clip(static_cast<float>(input.R32[i] + static_cast<float>(jump)),
                 -std::numeric_limits<float>::max(),
                 std::numeric_limits<float>::max());
        input.R64[i] =
            clip(static_cast<double>(input.R64[i] + static_cast<double>(jump)),
                 -std::numeric_limits<double>::max(),
                 std::numeric_limits<double>::max());
    }

    return input;
}
#endif // TESTING_ADIOS2_ENGINE_SMALLTESTDATA_H_
