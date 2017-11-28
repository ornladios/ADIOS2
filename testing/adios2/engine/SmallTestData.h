/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#ifndef TESTING_ADIOS2_ENGINE_SMALLTESTDATA_H_
#define TESTING_ADIOS2_ENGINE_SMALLTESTDATA_H_

#include <cstdint>

#include <array>
#include <limits>
#include <string>

#ifdef WIN32
#define NOMINMAX
#endif

// Test data for each type.  Make sure our values exceed the range of the
// previous size to make sure we all bytes for each element
struct SmallTestData
{
    std::string S1 = "Testing ADIOS2 String type";
    std::array<std::string, 3> S3 = {{"one", "two", "three"}};

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

SmallTestData generateNewSmallTestData(SmallTestData in, int step, int rank,
                                       int size)
{
    int j = rank + 1 + step * size;
    std::for_each(in.I8.begin(), in.I8.end(), [&](int8_t &v) { v += j; });
    std::for_each(in.I16.begin(), in.I16.end(), [&](int16_t &v) { v += j; });
    std::for_each(in.I32.begin(), in.I32.end(), [&](int32_t &v) { v += j; });
    std::for_each(in.I64.begin(), in.I64.end(), [&](int64_t &v) { v += j; });
    std::for_each(in.U8.begin(), in.U8.end(), [&](uint8_t &v) { v += j; });
    std::for_each(in.U16.begin(), in.U16.end(), [&](uint16_t &v) { v += j; });
    std::for_each(in.U32.begin(), in.U32.end(), [&](uint32_t &v) { v += j; });
    std::for_each(in.U64.begin(), in.U64.end(), [&](uint64_t &v) { v += j; });
    std::for_each(in.R32.begin(), in.R32.end(), [&](float &v) { v += j; });
    std::for_each(in.R64.begin(), in.R64.end(), [&](double &v) { v += j; });

    return in;
}

void UpdateSmallTestData(SmallTestData &in, int step, int rank, int size)
{
    int j = rank + 1 + step * size;
    std::for_each(in.I8.begin(), in.I8.end(), [&](int8_t &v) { v += j; });
    std::for_each(in.I16.begin(), in.I16.end(), [&](int16_t &v) { v += j; });
    std::for_each(in.I32.begin(), in.I32.end(), [&](int32_t &v) { v += j; });
    std::for_each(in.I64.begin(), in.I64.end(), [&](int64_t &v) { v += j; });
    std::for_each(in.U8.begin(), in.U8.end(), [&](uint8_t &v) { v += j; });
    std::for_each(in.U16.begin(), in.U16.end(), [&](uint16_t &v) { v += j; });
    std::for_each(in.U32.begin(), in.U32.end(), [&](uint32_t &v) { v += j; });
    std::for_each(in.U64.begin(), in.U64.end(), [&](uint64_t &v) { v += j; });
    std::for_each(in.R32.begin(), in.R32.end(), [&](float &v) { v += j; });
    std::for_each(in.R64.begin(), in.R64.end(), [&](double &v) { v += j; });
}

#endif // TESTING_ADIOS2_ENGINE_SMALLTESTDATA_H_
