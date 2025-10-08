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

class DerivedCorrectness : public ::testing::Test
{
public:
    DerivedCorrectness() = default;
};

class DerivedCorrectnessP : public DerivedCorrectness,
                            public ::testing::WithParamInterface<adios2::DerivedVarType>
{
protected:
    adios2::DerivedVarType GetThreads() { return GetParam(); };
};

TEST_P(DerivedCorrectnessP, ErrorTest)
{
    adios2::DerivedVarType mode = GetParam();
    adios2::ADIOS adios;
    adios2::IO bpOut = adios.DeclareIO("BPDerivedError");
    EXPECT_THROW(bpOut.DefineDerivedVariable("derived", "x= var1 \n nofunction(x)", mode),
                 std::invalid_argument);
    EXPECT_THROW(bpOut.DefineDerivedVariable("derived", "x= var1 \n sqrt(x)", mode),
                 std::invalid_argument);
    // check for too many constants
    EXPECT_THROW(bpOut.DefineDerivedVariable("derived", "x= var1 \n pow(x, 3, 2)", mode),
                 std::invalid_argument);
}

TEST_P(DerivedCorrectnessP, ComposedExpressionTest)
{
    adios2::DerivedVarType mode = GetParam();
    adios2::ADIOS adios;
    adios2::IO bpOut = adios.DeclareIO("BPComposedWrite");

    const size_t N = 10;
    std::default_random_engine generator;
    std::uniform_real_distribution<double> distribution(0.0, 10.0);
    std::vector<double> simArray(N);
    for (size_t i = 0; i < N; ++i)
        simArray[i] = distribution(generator);

    auto U = bpOut.DefineVariable<double>("var1", {N}, {0}, {N});
    bpOut.DefineDerivedVariable("derived", "x= var1 \n sqrt(sum(pow(x), x, 2))", mode);
    adios2::Engine bpFileWriter = bpOut.Open("BPDeriveCompose.bp", adios2::Mode::Write);

    bpFileWriter.BeginStep();
    bpFileWriter.Put(U, simArray.data());
    bpFileWriter.EndStep();
    bpFileWriter.Close();

    double epsilon = (double)0.01;
    adios2::IO bpIn = adios.DeclareIO("BPComposedRead");
    adios2::Engine bpFileReader = bpIn.Open("BPDeriveCompose.bp", adios2::Mode::Read);
    bpFileReader.BeginStep();
    auto derVar = bpIn.InquireVariable<double>("derived");
    EXPECT_EQ(derVar.Shape().size(), 1);
    EXPECT_EQ(derVar.Shape()[0], N);

    std::vector<double> readCom;
    bpFileReader.Get(derVar, readCom);
    bpFileReader.EndStep();

    for (size_t ind = 0; ind < N; ++ind)
    {
        double calcDerived = (double)sqrt(pow(simArray[ind], 2) + simArray[ind] + 2);
        EXPECT_TRUE(fabs(calcDerived - readCom[ind]) < epsilon);
    }

    bpFileReader.Close();
}

TEST_P(DerivedCorrectnessP, BasicCorrectnessTest)
{
    adios2::DerivedVarType mode = GetParam();
    adios2::ADIOS adios;
    adios2::IO bpOut = adios.DeclareIO("BPNoData");

    const size_t N = 10;
    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution(0.0, 10.0);
    std::vector<float> simArray1(N);
    std::vector<float> simArray2(N);
    for (size_t i = 0; i < N; ++i)
        simArray1[i] = distribution(generator);

    auto U = bpOut.DefineVariable<float>("var1", {N}, {0}, {N});
    EXPECT_TRUE(U); // needed to not have a warning for not using U
    auto V = bpOut.DefineVariable<float>("var2", {N}, {0}, {N});
    bpOut.DefineDerivedVariable("derived", "x= var1 \n sqrt(x)", mode);
    adios2::Engine bpFileWriter = bpOut.Open("BPNoData.bp", adios2::Mode::Write);

    bpFileWriter.BeginStep();
    bpFileWriter.Put(V, simArray1.data());
    bpFileWriter.EndStep();
    bpFileWriter.Close();

    // check that no derived data was written
    adios2::IO bpIn = adios.DeclareIO("BPReadExpression");
    adios2::Engine bpFileReader = bpIn.Open("BPNoData.bp", adios2::Mode::Read);
    bpFileReader.BeginStep();
    auto derVar = bpIn.InquireVariable<float>("derived");
    EXPECT_FALSE(derVar);
    bpFileReader.EndStep();
    bpFileReader.Close();
}

TEST_P(DerivedCorrectnessP, VectorCorrectnessTest)
{
    const size_t Nx = 2, Ny = 3, Nz = 10;
    const size_t steps = 2;
    // Application variable
    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution(0.0, 10.0);
    adios2::DerivedVarType mode = GetParam();

    std::vector<float> simArray1(Nx * Ny * Nz);
    std::vector<float> simArray2(Nx * Ny * Nz);
    std::vector<float> simArray3(Nx * Ny * Nz);
    std::vector<float> simArray4(Nx * Ny * Nz);
    std::vector<float> simArray5(Nx * Ny * Nz);
    std::vector<float> simArray6(Nx * Ny * Nz);
    for (size_t i = 0; i < Nx * Ny * Nz; ++i)
    {
        simArray1[i] = distribution(generator);
        simArray2[i] = distribution(generator);
        simArray3[i] = distribution(generator);
        simArray4[i] = distribution(generator);
        simArray5[i] = distribution(generator);
        simArray6[i] = distribution(generator);
    }

    adios2::ADIOS adios;
    adios2::IO bpOut = adios.DeclareIO("BPWriteExpression");
    std::vector<std::string> varname = {"sim2/Ux", "sim2/Uy", "sim2/Uz",
                                        "sim2/Vx", "sim2/Vy", "sim2/Vz"};
    const std::string derMagName = "derived/magU";
    const std::string derCrossName = "derived/crossU";

    auto Ux = bpOut.DefineVariable<float>(varname[0], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    auto Uy = bpOut.DefineVariable<float>(varname[1], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    auto Uz = bpOut.DefineVariable<float>(varname[2], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    auto Vx = bpOut.DefineVariable<float>(varname[3], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    auto Vy = bpOut.DefineVariable<float>(varname[4], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    auto Vz = bpOut.DefineVariable<float>(varname[5], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    // clang-format off
    bpOut.DefineDerivedVariable(derMagName,
                                "x =" + varname[0] + " \n"
                                "y =" + varname[1] + " \n"
                                "z =" + varname[2] + " \n"
                                "magnitude(x,y,z)",
                                mode);
    bpOut.DefineDerivedVariable(derCrossName,
                                "Ux =" + varname[0] + " \n"
                                "Uy =" + varname[1] + " \n"
                                "Uz =" + varname[2] + " \n"
                                "Vx =" + varname[3] + " \n"
                                "Vy =" + varname[4] + " \n"
                                "Vz =" + varname[5] + " \n"
                                "cross(Ux, Uy, Uz, Vx, Vy, Vz)",
                                mode);
    // clang-format on
    std::string filename = "ADIOS2BPWriteDerivedVector.bp";
    adios2::Engine bpFileWriter = bpOut.Open(filename, adios2::Mode::Write);

    for (size_t i = 0; i < steps; i++)
    {
        bpFileWriter.BeginStep();
        bpFileWriter.Put(Ux, simArray1.data());
        bpFileWriter.Put(Uy, simArray2.data());
        bpFileWriter.Put(Uz, simArray3.data());
        bpFileWriter.Put(Vx, simArray4.data());
        bpFileWriter.Put(Vy, simArray5.data());
        bpFileWriter.Put(Vz, simArray6.data());
        bpFileWriter.EndStep();
    }
    bpFileWriter.Close();

    adios2::IO bpIn = adios.DeclareIO("BPReadMagExpression");
    adios2::Engine bpFileReader = bpIn.Open(filename, adios2::Mode::Read);

    std::vector<float> readUx;
    std::vector<float> readUy;
    std::vector<float> readUz;
    std::vector<float> readVx;
    std::vector<float> readVy;
    std::vector<float> readVz;
    std::vector<float> readMag;
    std::vector<float> readCross;

    float calcDerived;
    float epsilon = (float)0.01;
    for (size_t i = 0; i < steps; i++)
    {
        bpFileReader.BeginStep();
        auto varUx = bpIn.InquireVariable<float>(varname[0]);
        auto varUy = bpIn.InquireVariable<float>(varname[1]);
        auto varUz = bpIn.InquireVariable<float>(varname[2]);
        auto varVx = bpIn.InquireVariable<float>(varname[3]);
        auto varVy = bpIn.InquireVariable<float>(varname[4]);
        auto varVz = bpIn.InquireVariable<float>(varname[5]);
        auto varmag = bpIn.InquireVariable<float>(derMagName);
        auto varcross = bpIn.InquireVariable<float>(derCrossName);

        EXPECT_EQ(varmag.Shape().size(), 3);
        EXPECT_EQ(varmag.Shape()[0], Nx);
        EXPECT_EQ(varmag.Shape()[1], Ny);
        EXPECT_EQ(varmag.Shape()[2], Nz);

        EXPECT_EQ(varcross.Shape().size(), 4);
        EXPECT_EQ(varcross.Shape()[0], Nx);
        EXPECT_EQ(varcross.Shape()[1], Ny);
        EXPECT_EQ(varcross.Shape()[2], Nz);
        EXPECT_EQ(varcross.Shape()[3], 3);

        bpFileReader.Get(varUx, readUx);
        bpFileReader.Get(varUy, readUy);
        bpFileReader.Get(varUz, readUz);
        bpFileReader.Get(varVx, readVx);
        bpFileReader.Get(varVy, readVy);
        bpFileReader.Get(varVz, readVz);
        bpFileReader.Get(varmag, readMag);
        bpFileReader.Get(varcross, readCross);
        bpFileReader.EndStep();

        // Magnitude
        for (size_t ind = 0; ind < Nx * Ny * Nz; ++ind)
        {
            calcDerived =
                (float)sqrt(pow(readUx[ind], 2) + pow(readUy[ind], 2) + pow(readUz[ind], 2));
            EXPECT_TRUE(fabs(calcDerived - readMag[ind]) < epsilon);
        }
        // Cross Product
        for (size_t ind = 0; ind < Nx * Ny * Nz; ++ind)
        {
            calcDerived = (readUy[ind] * readVz[ind]) - (readUz[ind] * readVy[ind]);
            EXPECT_TRUE(fabs(calcDerived - readCross[3 * ind]) < epsilon);
            calcDerived = (readUx[ind] * readVz[ind]) - (readUz[ind] * readVx[ind]);
            EXPECT_TRUE(fabs(calcDerived - readCross[3 * ind + 1]) < epsilon);
            calcDerived = (readUx[ind] * readVy[ind]) - (readUy[ind] * readVx[ind]);
            EXPECT_TRUE(fabs(calcDerived - readCross[3 * ind + 2]) < epsilon);
        }
    }
    bpFileReader.Close();
}

TEST_P(DerivedCorrectnessP, CurlCorrectnessTest)
{
    const size_t Nx = 25, Ny = 70, Nz = 13;
    float error_limit = 0.0000001f;

    adios2::DerivedVarType mode = GetParam();
    std::cout << "Mode is " << mode << std::endl;

    // Application variable
    std::vector<float> simArray1(Nx * Ny * Nz);
    std::vector<float> simArray2(Nx * Ny * Nz);
    std::vector<float> simArray3(Nx * Ny * Nz);
    for (size_t i = 0; i < Nx; ++i)
    {
        for (size_t j = 0; j < Ny; ++j)
        {
            for (size_t k = 0; k < Nz; ++k)
            {
                size_t idx = (i * Ny * Nz) + (j * Nz) + k;
                float x = static_cast<float>(i);
                float y = static_cast<float>(j);
                float z = static_cast<float>(k);
                // Linear curl example
                simArray1[idx] = (6 * x * y) + (7 * z);
                simArray2[idx] = (4 * x * z) + powf(y, 2);
                simArray3[idx] = sqrtf(z) + (2 * x * y);
                /* Less linear example
                simArray1[idx] = sinf(z);
                simArray2[idx] = 4 * x;
                simArray3[idx] = powf(y, 2) * cosf(x);
                */
                /* Nonlinear example
                simArray1[idx] = expf(2 * y) * sinf(x);
                simArray2[idx] = sqrtf(z + 1) * cosf(x);
                simArray3[idx] = powf(x, 2) * sinf(y) + (6 * z);
                */
            }
        }
    }

    adios2::ADIOS adios;
    adios2::IO bpOut = adios.DeclareIO("BPWriteExpression");
    std::vector<std::string> varname = {"sim3/VX", "sim3/VY", "sim3/VZ"};
    std::string derivedname = "derived/curlV";

    auto VX = bpOut.DefineVariable<float>(varname[0], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    auto VY = bpOut.DefineVariable<float>(varname[1], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    auto VZ = bpOut.DefineVariable<float>(varname[2], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    // clang-format off
    bpOut.DefineDerivedVariable(derivedname,
                                "Vx =" + varname[0] + " \n"
                                "Vy =" + varname[1] + " \n"
                                "Vz =" + varname[2] + " \n"
                                "curl(Vx,Vy,Vz)",
                                mode);
    // clang-format on
    std::string filename = "ADIOS2BPWriteDerivedCurl.bp";
    adios2::Engine bpFileWriter = bpOut.Open(filename, adios2::Mode::Write);

    bpFileWriter.BeginStep();
    bpFileWriter.Put(VX, simArray1.data());
    bpFileWriter.Put(VY, simArray2.data());
    bpFileWriter.Put(VZ, simArray3.data());
    bpFileWriter.EndStep();
    bpFileWriter.Close();

    adios2::IO bpIn = adios.DeclareIO("BPReadCurlExpression");
    adios2::Engine bpFileReader = bpIn.Open(filename, adios2::Mode::Read);

    std::vector<float> readVX;
    std::vector<float> readVY;
    std::vector<float> readVZ;
    std::vector<float> readCurl;

    std::vector<std::vector<float>> calcCurl;
    double sum_x = 0;
    double sum_y = 0;
    double sum_z = 0;
    bpFileReader.BeginStep();
    auto varVX = bpIn.InquireVariable<float>(varname[0]);
    auto varVY = bpIn.InquireVariable<float>(varname[1]);
    auto varVZ = bpIn.InquireVariable<float>(varname[2]);
    auto varCurl = bpIn.InquireVariable<float>(derivedname);

    EXPECT_EQ(varCurl.Shape().size(), 4);
    EXPECT_EQ(varCurl.Shape()[0], Nx);
    EXPECT_EQ(varCurl.Shape()[1], Ny);
    EXPECT_EQ(varCurl.Shape()[2], Nz);
    EXPECT_EQ(varCurl.Shape()[3], 3);

    bpFileReader.Get(varVX, readVX);
    bpFileReader.Get(varVY, readVY);
    bpFileReader.Get(varVZ, readVZ);
    bpFileReader.Get(varCurl, readCurl);
    bpFileReader.EndStep();

    float curl_x, curl_y, curl_z;
    float err_x, err_y, err_z;
    for (size_t i = 0; i < Nx; ++i)
    {
        for (size_t j = 0; j < Ny; ++j)
        {
            for (size_t k = 0; k < Nz; ++k)
            {
                size_t idx = (i * Ny * Nz) + (j * Nz) + k;
                float x = static_cast<float>(i);
                float y = static_cast<float>(j);
                float z = static_cast<float>(k);
                // Linear example
                curl_x = -(2 * x);
                curl_y = 7 - (2 * y);
                curl_z = (4 * z) - (6 * x);
                /* Less linear
                curl_x = 2 * y * cosf(x);
                curl_y = cosf(z) + (powf(y, 2) * sinf(x));
                curl_z = 4;
                */
                /* Nonlinear example
                curl_x = powf(x, 2) * cosf(y) - (cosf(x) / (2 * sqrtf(z + 1)));
                curl_y = -2 * x * sinf(y);
                curl_z = -sqrtf(z + 1) * sinf(x) - (2 * expf(2 * y) * sinf(x));
                */
                if (fabs(curl_x) < 1)
                {
                    err_x = fabs(curl_x - readCurl[3 * idx]) / (1 + fabs(curl_x));
                }
                else
                {
                    err_x = fabs(curl_x - readCurl[3 * idx]) / fabs(curl_x);
                }
                if (fabs(curl_y) < 1)
                {
                    err_y = fabs(curl_y - readCurl[3 * idx + 1]) / (1 + fabs(curl_y));
                }
                else
                {
                    err_y = fabs(curl_y - readCurl[3 * idx + 1]) / fabs(curl_y);
                }
                if (fabs(curl_z) < 1)
                {
                    err_z = fabs(curl_z - readCurl[3 * idx + 2]) / (1 + fabs(curl_z));
                }
                else
                {
                    err_z = fabs(curl_z - readCurl[3 * idx + 2]) / fabs(curl_z);
                }
                sum_x += err_x;
                sum_y += err_y;
                sum_z += err_z;
            }
        }
    }
    bpFileReader.Close();
    EXPECT_LT((sum_x + sum_y + sum_z) / (3 * Nx * Ny * Nz), error_limit);
    EXPECT_LT(sum_x / (Nx * Ny * Nz), error_limit);
    EXPECT_LT(sum_y / (Nx * Ny * Nz), error_limit);
    EXPECT_LT(sum_z / (Nx * Ny * Nz), error_limit);
}

TEST_P(DerivedCorrectnessP, CurlAggCorrectnessTest)
{
    const size_t Nx = 25, Ny = 70, Nz = 13;
    const size_t dataSize = Nx * Ny * Nz;
    float error_limit = 0.0000001f;

    adios2::DerivedVarType mode = GetParam();
    std::cout << "Mode is " << mode << std::endl;

    // Application variable
    std::vector<float> simArray(dataSize * 3);
    for (size_t i = 0; i < Nx; ++i)
    {
        for (size_t j = 0; j < Ny; ++j)
        {
            for (size_t k = 0; k < Nz; ++k)
            {
                size_t idx = (i * Ny * Nz) + (j * Nz) + k;
                float x = static_cast<float>(i);
                float y = static_cast<float>(j);
                float z = static_cast<float>(k);
                // Linear curl example
                simArray[idx] = (6 * x * y) + (7 * z);
                simArray[idx + dataSize] = (4 * x * z) + powf(y, 2);
                simArray[idx + 2 * dataSize] = sqrtf(z) + (2 * x * y);
            }
        }
    }

    adios2::ADIOS adios;
    adios2::IO bpOut = adios.DeclareIO("BPWriteExpression");
    std::string derivedname = "derived/curlV";

    auto VX = bpOut.DefineVariable<float>("sim/V", {Nx, Ny, Nz, 3}, {0, 0, 0, 0}, {Nx, Ny, Nz, 3});
    // clang-format off
    bpOut.DefineDerivedVariable(derivedname,
                                "V=sim/V \n"
                                "curl(V)",
                                mode);
    // clang-format on
    std::string filename = "ADIOS2BPWriteDerivedCurlAgg.bp";
    adios2::Engine bpFileWriter = bpOut.Open(filename, adios2::Mode::Write);

    bpFileWriter.BeginStep();
    bpFileWriter.Put(VX, simArray.data());
    bpFileWriter.EndStep();
    bpFileWriter.Close();

    adios2::IO bpIn = adios.DeclareIO("BPReadCurlExpression");
    adios2::Engine bpFileReader = bpIn.Open(filename, adios2::Mode::Read);

    std::vector<float> readV;
    std::vector<float> readCurl;

    std::vector<std::vector<float>> calcCurl;
    double sum_x = 0;
    double sum_y = 0;
    double sum_z = 0;
    bpFileReader.BeginStep();
    auto varV = bpIn.InquireVariable<float>("sim/V");
    auto varCurl = bpIn.InquireVariable<float>(derivedname);

    EXPECT_EQ(varCurl.Shape().size(), 4);
    EXPECT_EQ(varCurl.Shape()[0], Nx);
    EXPECT_EQ(varCurl.Shape()[1], Ny);
    EXPECT_EQ(varCurl.Shape()[2], Nz);
    EXPECT_EQ(varCurl.Shape()[3], 3);

    bpFileReader.Get(varV, readV);
    bpFileReader.Get(varCurl, readCurl);
    bpFileReader.EndStep();

    float curl_x, curl_y, curl_z;
    float err_x, err_y, err_z;
    for (size_t i = 0; i < Nx; ++i)
    {
        for (size_t j = 0; j < Ny; ++j)
        {
            for (size_t k = 0; k < Nz; ++k)
            {
                size_t idx = (i * Ny * Nz) + (j * Nz) + k;
                float x = static_cast<float>(i);
                float y = static_cast<float>(j);
                float z = static_cast<float>(k);
                // Linear example
                curl_x = -(2 * x);
                curl_y = 7 - (2 * y);
                curl_z = (4 * z) - (6 * x);
                if (fabs(curl_x) < 1)
                {
                    err_x = fabs(curl_x - readCurl[3 * idx]) / (1 + fabs(curl_x));
                }
                else
                {
                    err_x = fabs(curl_x - readCurl[3 * idx]) / fabs(curl_x);
                }
                if (fabs(curl_y) < 1)
                {
                    err_y = fabs(curl_y - readCurl[3 * idx + 1]) / (1 + fabs(curl_y));
                }
                else
                {
                    err_y = fabs(curl_y - readCurl[3 * idx + 1]) / fabs(curl_y);
                }
                if (fabs(curl_z) < 1)
                {
                    err_z = fabs(curl_z - readCurl[3 * idx + 2]) / (1 + fabs(curl_z));
                }
                else
                {
                    err_z = fabs(curl_z - readCurl[3 * idx + 2]) / fabs(curl_z);
                }
                sum_x += err_x;
                sum_y += err_y;
                sum_z += err_z;
            }
        }
    }
    bpFileReader.Close();
    EXPECT_LT((sum_x + sum_y + sum_z) / (3 * Nx * Ny * Nz), error_limit);
    EXPECT_LT(sum_x / (Nx * Ny * Nz), error_limit);
    EXPECT_LT(sum_y / (Nx * Ny * Nz), error_limit);
    EXPECT_LT(sum_z / (Nx * Ny * Nz), error_limit);
}

TEST_P(DerivedCorrectnessP, MagCurlCorrectnessTest)
{
    const size_t Nx = 2, Ny = 3, Nz = 10;
    float error_limit = 0.0000001f;
    adios2::DerivedVarType mode = GetParam();
    std::cout << "Mode is " << mode << std::endl;

    std::vector<float> simArray1(Nx * Ny * Nz);
    std::vector<float> simArray2(Nx * Ny * Nz);
    std::vector<float> simArray3(Nx * Ny * Nz);
    for (size_t i = 0; i < Nx; ++i)
    {
        for (size_t j = 0; j < Ny; ++j)
        {
            for (size_t k = 0; k < Nz; ++k)
            {
                size_t idx = (i * Ny * Nz) + (j * Nz) + k;
                float x = static_cast<float>(i);
                float y = static_cast<float>(j);
                float z = static_cast<float>(k);
                simArray1[idx] = (6 * x * y) + (7 * z);
                simArray2[idx] = (4 * x * z) + powf(y, 2);
                simArray3[idx] = sqrtf(z) + (2 * x * y);
            }
        }
    }

    adios2::ADIOS adios;
    adios2::IO bpOut = adios.DeclareIO("BPWriteExpr");
    std::vector<std::string> varname = {"sim2/Ux", "sim2/Uy", "sim2/Uz"};
    std::string derivedname = "derived/magCurlU";

    auto Ux = bpOut.DefineVariable<float>(varname[0], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    auto Uy = bpOut.DefineVariable<float>(varname[1], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    auto Uz = bpOut.DefineVariable<float>(varname[2], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    // clang-format off
    bpOut.DefineDerivedVariable(derivedname,
                                "x =" + varname[0] + " \n"
                                "y =" + varname[1] + " \n"
                                "z =" + varname[2] + " \n"
                                "magnitude(curl(x,y,z))",
                                mode);
    // clang-format on
    std::string filename = "ADIOS2BPWriteDerivedMagCurl.bp";
    adios2::Engine bpFileWriter = bpOut.Open(filename, adios2::Mode::Write);

    bpFileWriter.BeginStep();
    bpFileWriter.Put(Ux, simArray1.data());
    bpFileWriter.Put(Uy, simArray2.data());
    bpFileWriter.Put(Uz, simArray3.data());
    bpFileWriter.EndStep();
    bpFileWriter.Close();

    adios2::IO bpIn = adios.DeclareIO("BPReadMagCurlExpr");
    adios2::Engine bpFileReader = bpIn.Open(filename, adios2::Mode::Read);

    std::vector<float> readMag;
    bpFileReader.BeginStep();
    auto varmag = bpIn.InquireVariable<float>(derivedname);
    bpFileReader.Get(varmag, readMag);
    bpFileReader.EndStep();

    float curl_x, curl_y, curl_z;
    double err;
    for (size_t i = 0; i < Nx; ++i)
    {
        for (size_t j = 0; j < Ny; ++j)
        {
            for (size_t k = 0; k < Nz; ++k)
            {
                size_t idx = (i * Ny * Nz) + (j * Nz) + k;
                float x = static_cast<float>(i);
                float y = static_cast<float>(j);
                float z = static_cast<float>(k);
                curl_x = -(2 * x);
                curl_y = 7 - (2 * y);
                curl_z = (4 * z) - (6 * x);
                auto mag_curl = sqrt(pow(curl_x, 2) + pow(curl_y, 2) + pow(curl_z, 2));
                if (fabs(mag_curl) < 1)
                {
                    err = fabs(mag_curl - readMag[idx]) / (1 + fabs(mag_curl));
                }
                else
                {
                    err = fabs(mag_curl - readMag[idx]) / fabs(mag_curl);
                }
            }
        }
    }
    bpFileReader.Close();
    EXPECT_LT(err / (Nx * Ny * Nz), error_limit);
}

INSTANTIATE_TEST_SUITE_P(DerivedCorrectness, DerivedCorrectnessP,
                         ::testing::Values(adios2::DerivedVarType::StatsOnly,
                                           adios2::DerivedVarType::ExpressionString,
                                           adios2::DerivedVarType::StoreData));
int main(int argc, char **argv)
{
    int result;
    ::testing::InitGoogleTest(&argc, argv);

    result = RUN_ALL_TESTS();

    return result;
}
