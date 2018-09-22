/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#ifndef TESTING_ADIOS2_ENGINE_SST_TESTDATA_H_
#define TESTING_ADIOS2_ENGINE_SST_TESTDATA_H_

#include <cstdint>

#include <array>
#include <limits>
#include <string>

#ifdef WIN32
#define NOMINMAX
#endif

// Number of rows

const std::size_t Nx = 10;

std::array<int8_t, 10> data_I8;
std::array<int16_t, 10> data_I16;
std::array<int32_t, 10> data_I32;
std::array<int64_t, 10> data_I64;
std::array<float, 10> data_R32;
std::array<double, 10> data_R64;
std::array<std::complex<float>, 10> data_C32;
std::array<std::complex<double>, 10> data_C64;
double data_R64_2d[10][2];
double data_R64_2d_rev[2][10];

std::vector<int8_t> in_I8;
std::vector<int16_t> in_I16;
std::vector<int32_t> in_I32;
std::vector<int64_t> in_I64;
std::vector<float> in_R32;
std::vector<double> in_R64;
std::vector<std::complex<float>> in_C32;
std::vector<std::complex<double>> in_C64;
std::vector<double> in_R64_2d;
std::vector<double> in_R64_2d_rev;

int8_t in_scalar_I8;
int16_t in_scalar_I16;
int32_t in_scalar_I32;
int64_t in_scalar_I64;
float in_scalar_R32;
double in_scalar_R64;
std::complex<float> in_scalar_C32;
std::complex<double> in_scalar_C64;

void generateSstTestData(int step, int rank, int size)
{
    int64_t j = rank * Nx * 10 + step;
    for (int i = 0; i < sizeof(data_I8); i++)
    {
        data_I8[i] = (int8_t)(j + 10 * i);
        data_I16[i] = (int16_t)(j + 10 * i);
        data_I32[i] = (int32_t)(j + 10 * i);
        data_I64[i] = (int64_t)(j + 10 * i);
        data_R32[i] = j + 10 * i;
        data_R64[i] = j + 10 * i;
        data_C32[i].imag(j + 10 * i);
        data_C32[i].real(-(j + 10 * i));
        data_C64[i].imag(j + 10 * i);
        data_C64[i].real(-(j + 10 * i));
        data_R64_2d[i][0] = j + 10 * i;
        data_R64_2d[i][1] = 10000 + j + 10 * i;
        data_R64_2d_rev[0][i] = j + 10 * i;
        data_R64_2d_rev[1][i] = 10000 + j + 10 * i;
    }
}

int validateSstTestData(int start, int length, int step)
{
    int failures = 0;
    if (in_scalar_R64 != 1.5 * (step + 1))
        {
            std::cout << "Expected " << 1.5*(step+1) << ", got " 
                      << in_scalar_R64 << " for in_scalar_R64, timestep " << step << std::endl;
            failures++;
        }
    for (int i = 0; i < length; i++)
    {
        if (in_I8[i] != (int8_t)((i + start) * 10 + step))
        {
            std::cout << "Expected 0x" << std::hex
                      << (int8_t)((i + start) * 10 + step) << ", got 0x"
                      << std::hex << in_I8[i] << " for in_I8[" << i
                      << "](global[" << i + start << "])" << std::endl;
            failures++;
        }
        if (in_I16[i] != (int16_t)((i + start) * 10 + step))
        {
            std::cout << "Expected 0x" << std::hex
                      << (int16_t)((i + start) * 10 + step) << ", got 0x"
                      << std::hex << in_I16[i] << " for in_I16[" << i
                      << "](global[" << i + start << "])" << std::endl;
            failures++;
        }
        if (in_I32[i] != (int32_t)((i + start) * 10 + step))
        {
            std::cout << "Expected 0x" << std::hex
                      << (int32_t)((i + start) * 10 + step) << ", got 0x"
                      << std::hex << in_I32[i] << " for in_I32[" << i
                      << "](global[" << i + start << "])" << std::endl;
            failures++;
        }
        if (in_I64[i] != (int64_t)((i + start) * 10 + step))
        {
            std::cout << "Expected 0x" << std::hex
                      << (int64_t)((i + start) * 10 + step) << ", got 0x"
                      << std::hex << in_I64[i] << " for in_I64[" << i
                      << "](global[" << i + start << "])" << std::endl;
            failures++;
        }

        if (in_R32[i] != (float)((i + start) * 10 + step))
        {
            std::cout << "Expected " << (float)((i + start) * 10 + step)
                      << ", got " << in_R32[i] << " for in_R32[" << i
                      << "](global[" << i + start << "])" << std::endl;
            failures++;
        }
        if (in_R64[i] != (double)((i + start) * 10 + step))
        {
            std::cout << "Expected " << (double)((i + start) * 10 + step)
                      << ", got " << in_R64[i] << " for in_R64[" << i
                      << "](global[" << i + start << "])" << std::endl;
            failures++;
        }
        if ((in_C32[i].imag() != (float)((i + start) * 10 + step)) ||
            (in_C32[i].real() != -(float)((i + start) * 10 + step)))
        {
            std::cout << "Expected [" << (float)((i + start) * 10 + step)
                      << ", " << -(float)((i + start) * 10 + step) << "], got "
                      << in_C32[i] << " for in_C32[" << i << "](global["
                      << i + start << "])" << std::endl;
            failures++;
        }
        if ((in_C64[i].imag() != (double)((i + start) * 10 + step)) ||
            (in_C64[i].real() != (-(double)((i + start) * 10 + step))))
        {
            std::cout << "Expected [" << (double)((i + start) * 10 + step)
                      << ", " << -(double)((i + start) * 10 + step) << "], got "
                      << in_C64[i] << " for in_C64[" << i << "](global["
                      << i + start << "])" << std::endl;
            failures++;
        }
        if (in_R64_2d[2 * i] != (double)((i + start) * 10 + step))
        {
            std::cout << "Expected " << (double)((i + start) * 10 + step)
                      << ", got " << in_R64_2d[i] << " for in_R64_2d[" << i
                      << "][0](global[" << i + start << "][0])" << std::endl;
            failures++;
        }
        if (in_R64_2d[2 * i + 1] != (double)(10000 + (i + start) * 10 + step))
        {
            std::cout << "Expected "
                      << (double)(10000 + (i + start) * 10 + step) << ", got "
                      << in_R64_2d[i] << " for in_R64_2d[" << i
                      << "][1](global[" << i + start << "][1])" << std::endl;
            failures++;
        }
        if (in_R64_2d_rev[i] != (double)((i + start) * 10 + step))
        {
            std::cout << "Expected " << (double)((i + start) * 10 + step)
                      << ", got " << in_R64_2d_rev[i]
                      << " for in_R64_2d_rev[0][" << i << "](global[0]["
                      << i + start << "])" << std::endl;
            failures++;
        }
        if (in_R64_2d_rev[i + length] !=
            (double)(10000 + (i + start) * 10 + step))
        {
            std::cout << "Expected "
                      << (double)(10000 + (i + start) * 10 + step) << ", got "
                      << in_R64_2d_rev[i + length] << " for in_R64_2d_rev[1]["
                      << i << "](global[1][" << i + start << "])" << std::endl;
            failures++;
        }
    }
    return failures;
}

#endif // TESTING_ADIOS2_ENGINE_SST_TESTDATA_H_
