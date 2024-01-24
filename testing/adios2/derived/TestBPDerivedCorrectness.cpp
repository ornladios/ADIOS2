#include <cstdint>
#include <cstring>

#include <cmath>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

#include <adios2.h>
#include <gtest/gtest.h>

TEST(DerivedCorrectness, AddCorrectnessTest)
{
    const size_t Nx = 10, Ny = 3, Nz = 6;
    const size_t steps = 2;
    /** Application variable */
    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution(0.0, 10.0);

    std::vector<float> simArray1(Nx * Ny * Nz);
    std::vector<float> simArray2(Nx * Ny * Nz);
    std::vector<float> simArray3(Nx * Ny * Nz);
    for (size_t i = 0; i < Nx * Ny * Nz; ++i)
    {
        simArray1[i] = distribution(generator);
        simArray2[i] = distribution(generator);
        simArray3[i] = distribution(generator);
    }

    adios2::ADIOS adios;

    adios2::IO bpOut = adios.DeclareIO("BPWriteAddExpression");

    std::vector<std::string> varname = {"sim1/Ux", "sim1/Uy", "sim1/Uz"};
    std::string derivedname = "derived/addU";

    auto Ux = bpOut.DefineVariable<float>(varname[0], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    auto Uy = bpOut.DefineVariable<float>(varname[1], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    auto Uz = bpOut.DefineVariable<float>(varname[2], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    // clang-format off
    auto addU = bpOut.DefineDerivedVariable(derivedname,
                                            "x:" + varname[0] + " \n"
                                            "y:" + varname[1] + " \n"
                                            "z:" + varname[2] + " \n"
                                            "x+y+z",
                                            adios2::DerivedVarType::StoreData);
    // clang-format on
    std::string filename = "expAdd.bp";
    adios2::Engine bpFileWriter = bpOut.Open(filename, adios2::Mode::Write);

    for (int i = 0; i < steps; i++)
    {
        bpFileWriter.BeginStep();
        bpFileWriter.Put(Ux, simArray1.data());
        bpFileWriter.Put(Uy, simArray2.data());
        bpFileWriter.Put(Uz, simArray3.data());
        bpFileWriter.EndStep();
    }
    bpFileWriter.Close();

    adios2::IO bpIn = adios.DeclareIO("BPReadExpression");
    adios2::Engine bpFileReader = bpIn.Open(filename, adios2::Mode::Read);

    std::vector<float> readUx;
    std::vector<float> readUy;
    std::vector<float> readUz;
    std::vector<float> readAdd;

    float calcA;
    float epsilon = 0.01;
    for (int i = 0; i < steps; i++)
    {
        bpFileReader.BeginStep();
        bpFileReader.Get(varname[0], readUx);
        bpFileReader.Get(varname[1], readUy);
        bpFileReader.Get(varname[2], readUz);
        bpFileReader.Get(derivedname, readAdd);
        bpFileReader.EndStep();

        for (size_t ind = 0; ind < Nx * Ny * Nz; ++ind)
        {
            calcA = readUx[ind] + readUy[ind] + readUz[ind];
            EXPECT_TRUE(fabs(calcA - readAdd[ind]) < epsilon);
        }
    }
    bpFileReader.Close();
}

TEST(DerivedCorrectness, MagCorrectnessTest)
{
    const size_t Nx = 2, Ny = 3, Nz = 10;
    const size_t steps = 2;
    // Application variable
    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution(0.0, 10.0);

    std::vector<float> simArray1(Nx * Ny * Nz);
    std::vector<float> simArray2(Nx * Ny * Nz);
    std::vector<float> simArray3(Nx * Ny * Nz);
    for (size_t i = 0; i < Nx * Ny * Nz; ++i)
    {
        simArray1[i] = distribution(generator);
        simArray2[i] = distribution(generator);
        simArray3[i] = distribution(generator);
    }

    adios2::ADIOS adios;
    adios2::IO bpOut = adios.DeclareIO("BPWriteExpression");
    std::vector<std::string> varname = {"sim2/Ux", "sim2/Uy", "sim2/Uz"};
    std::string derivedname = "derived/magU";

    auto Ux = bpOut.DefineVariable<float>(varname[0], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    auto Uy = bpOut.DefineVariable<float>(varname[1], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    auto Uz = bpOut.DefineVariable<float>(varname[2], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    // clang-format off
    auto magU = bpOut.DefineDerivedVariable(derivedname,
                                            "x:" + varname[0] + " \n"
                                            "y:" + varname[1] + " \n"
                                            "z:" + varname[2] + " \n"
                                            "magnitude(x,y,z)",
                                            adios2::DerivedVarType::StoreData);
    // clang-format on
    std::string filename = "expMagnitude.bp";
    adios2::Engine bpFileWriter = bpOut.Open(filename, adios2::Mode::Write);

    for (int i = 0; i < steps; i++)
    {
        bpFileWriter.BeginStep();
        bpFileWriter.Put(Ux, simArray1.data());
        bpFileWriter.Put(Uy, simArray2.data());
        bpFileWriter.Put(Uz, simArray3.data());
        bpFileWriter.EndStep();
    }
    bpFileWriter.Close();

    adios2::IO bpIn = adios.DeclareIO("BPReadMagExpression");
    adios2::Engine bpFileReader = bpIn.Open(filename, adios2::Mode::Read);

    std::vector<float> readUx;
    std::vector<float> readUy;
    std::vector<float> readUz;
    std::vector<float> readMag;

    float calcM;
    float epsilon = 0.01;
    for (int i = 0; i < steps; i++)
    {
        bpFileReader.BeginStep();
        auto varx = bpIn.InquireVariable<float>(varname[0]);
        auto vary = bpIn.InquireVariable<float>(varname[1]);
        auto varz = bpIn.InquireVariable<float>(varname[2]);
        auto varmag = bpIn.InquireVariable<float>(derivedname);

        bpFileReader.Get(varx, readUx);
        bpFileReader.Get(vary, readUy);
        bpFileReader.Get(varz, readUz);
        bpFileReader.Get(varmag, readMag);
        bpFileReader.EndStep();

        for (size_t ind = 0; ind < Nx * Ny * Nz; ++ind)
        {
            calcM = sqrt(pow(readUx[ind], 2) + pow(readUy[ind], 2) + pow(readUz[ind], 2));
            EXPECT_TRUE(fabs(calcM - readMag[ind]) < epsilon);
        }
    }
    bpFileReader.Close();
}

/*
 * Linear Interpolation - average difference around point "index"
 *
 */
// T linear_interp (T* data, size_t index, size_t count, size_t stride, size_t margin, bool center)
float linear_interp (std::vector<float> data, size_t index, size_t count, size_t stride)
{
    size_t ind1 = index - stride;
    size_t ind2 = index + stride;
    bool boundary = false;
    if (index < stride)
      {
	ind1 = index;
	boundary = true;
      }
    if (count - index <= stride)
      {
	ind2 = index;
	boundary = true;
      }
     // If stride is out of bounds in both directions, ind1 = ind2 = index
    // return 0
    return (data[ind2] - data[ind1]) / (boundary? 1: 2);
}

/*
 * Input: 3D vector field F(x,y,z)= {F1(x,y,z), F2(x,y,z), F3(x,y,z)}
 *
 *     inputData - (3) components of 3D vector field
 *     margin - how many elements to each size will be used in approximating partial derivatives
 *     center - include point (x,y,z) in approximating of partial derivative at that point
 *
 * Computation:
 *     curl(F(x,y,z)) = (partial(F3,y) - partial(F2,z))i
 *                    + (partial(F1,z) - partial(F3,x))j
 *                    + (partial(F2,x) - partial(F1,y))k
 * 
 *     boundaries are calculated only with data in block
 *         (ex: partial derivatives in x direction at point (0,0,0)
 *              only use data from (1,0,0), etc )
 *
 * Return: 
 *     (3) components of curl
 */
//std::vector<T*> computecurl3D
//const std::vector<DerivedData> inputData, size_t xcount, size_t ycount, size_t zcount, size_t margin, bool center, std::function<T(T*, size_t, size_t, size_t, size_t, bool)> pdcomp)
std::vector<std::vector<float>> computecurl3D (std::vector<float> inputX, std::vector<float> inputY, std::vector<float> inputZ, size_t xcount, size_t ycount, size_t zcount)
{
    // ToDo - verify how to navigate over the inputData spaces
    size_t dataSize = xcount * ycount * zcount;
    size_t xstride = ycount * zcount;
    size_t ystride = zcount;
    size_t zstride = 1;

    std::vector<float> curlx(dataSize);
    std::vector<float> curly(dataSize);
    std::vector<float> curlz(dataSize);
    std::vector<std::vector<float>> curl = {curlx, curly, curlz};
    // std::vector<T*> curl = {(T*)malloc(xcount * ycount * zcount * sizeof(T)), (T*)malloc(xcount * ycount * zcount * sizeof(T)), (T*)malloc(xcount * ycount * zcount * sizeof(T))};

    for (size_t i = 0; i < xcount; ++i)
    {
        for (size_t j = 0; j < ycount; ++j)
        {
            for (size_t k = 0; k < zcount; ++k)
            {
                size_t index = (i * xstride) + (j * ystride) + (k * zstride);
                curl[0][index] = linear_interp(inputZ, index, dataSize, ystride) - linear_interp(inputY, index, dataSize, zstride);
                curl[1][index] = linear_interp(inputX, index, dataSize, zstride) - linear_interp(inputZ, index, dataSize, xstride);
                curl[2][index] = linear_interp(inputY, index, dataSize, xstride) - linear_interp(inputX, index, dataSize, ystride);
            }
        }
    }

    return curl;
}

TEST(DerivedCorrectness, CurlCorrectnessTest)
{
    const size_t Nx = 2, Ny = 3, Nz = 10;
    const size_t steps = 2;
    // Application variable
    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution(0.0, 10.0);

    std::vector<float> simArray1(Nx * Ny * Nz);
    std::vector<float> simArray2(Nx * Ny * Nz);
    std::vector<float> simArray3(Nx * Ny * Nz);
    for (size_t i = 0; i < Nx * Ny * Nz; ++i)
    {
        simArray1[i] = distribution(generator);
        simArray2[i] = distribution(generator);
        simArray3[i] = distribution(generator);
    }

    adios2::ADIOS adios;
    adios2::IO bpOut = adios.DeclareIO("BPWriteExpression");
    std::vector<std::string> varname = {"sim3/VX", "sim3/VY", "sim3/VZ"};
    std::string derivedname = "derived/curlV";

    auto VX = bpOut.DefineVariable<float>(varname[0], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    auto VY = bpOut.DefineVariable<float>(varname[1], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    auto VZ = bpOut.DefineVariable<float>(varname[2], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    // clang-format off
    auto curlV = bpOut.DefineDerivedVariable(derivedname,
                                            "Vx:" + varname[0] + " \n"
                                            "Vy:" + varname[1] + " \n"
                                            "Vz:" + varname[2] + " \n"
                                            "curl(Vx,Vy,Vz)",
                                            adios2::DerivedVarType::StoreData);
    // clang-format on
    std::string filename = "expCurl.bp";
    adios2::Engine bpFileWriter = bpOut.Open(filename, adios2::Mode::Write);

    for (int i = 0; i < steps; i++)
    {
        bpFileWriter.BeginStep();
        bpFileWriter.Put(VX, simArray1.data());
        bpFileWriter.Put(VY, simArray2.data());
        bpFileWriter.Put(VZ, simArray3.data());
        bpFileWriter.EndStep();
    }
    bpFileWriter.Close();

    adios2::IO bpIn = adios.DeclareIO("BPReadCurlExpression");
    adios2::Engine bpFileReader = bpIn.Open(filename, adios2::Mode::Read);

    std::vector<float> readVX;
    std::vector<float> readVY;
    std::vector<float> readVZ;
    // TODO/DEBUG - VERIFY DATATYPE
    std::vector<float> readCurl;

    std::vector<std::vector<float>> calcCurl;
    float epsilon = 0.01;
    for (int i = 0; i < steps; i++)
      {
        bpFileReader.BeginStep();
        auto varVX = bpIn.InquireVariable<float>(varname[0]);
        auto varVY = bpIn.InquireVariable<float>(varname[1]);
        auto varVZ = bpIn.InquireVariable<float>(varname[2]);
        auto varCurl = bpIn.InquireVariable<float>(derivedname);

        bpFileReader.Get(varVX, readVX);
        bpFileReader.Get(varVY, readVY);
        bpFileReader.Get(varVZ, readVZ);
        bpFileReader.Get(varCurl, readCurl);
        bpFileReader.EndStep();

	calcCurl = computecurl3D (readVX, readVY, readVZ, Nx, Ny, Nz);
        for (size_t ind = 0; ind < Nx * Ny * Nz; ++ind)
        {
            EXPECT_TRUE(fabs(calcCurl[0][ind] - readCurl[3 * ind]) < epsilon);
            EXPECT_TRUE(fabs(calcCurl[1][ind] - readCurl[3 * ind + 1]) < epsilon);
            EXPECT_TRUE(fabs(calcCurl[2][ind] - readCurl[3 * ind + 2]) < epsilon);
        }
    }
    bpFileReader.Close();
}

int main(int argc, char **argv)
{
    int result;
    ::testing::InitGoogleTest(&argc, argv);

    result = RUN_ALL_TESTS();

    return result;
}
