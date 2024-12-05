#include <cstdint>
#include <cstring>

#include <cmath>
#include <iostream>
#include <limits>
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

TEST_P(DerivedCorrectnessP, ScalarFunctionsCorrectnessTest)
{
    int rank = 0;
    int size = 1;
#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
#endif

    const size_t Nx = 10, Ny = 3, Nz = 6;
    const size_t steps = 2;
    // Application variable
    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution(0.0, 10.0);

    adios2::DerivedVarType mode = GetParam();
    std::cout << "Mode is " << mode << std::endl;

    std::vector<float> simArray1(Nx * Ny * Nz);
    std::vector<float> simArray2(Nx * Ny * Nz);
    std::vector<float> simArray3(Nx * Ny * Nz);
    for (size_t i = 0; i < Nx * Ny * Nz; ++i)
    {
        simArray1[i] = distribution(generator);
        simArray2[i] = distribution(generator);
        simArray3[i] = distribution(generator);
    }

#if ADIOS2_USE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    adios2::ADIOS adios;
#endif

    adios2::IO bpOut = adios.DeclareIO("BPWriteAddExpression");

    std::vector<std::string> varname = {"sim1/Ux", "sim1/Uy", "sim1/Uz"};
    const std::string derAgrAdd = "derived/agradd";
    const std::string derAddName = "derived/add";
    const std::string derSubtrName = "derived/subtr";
    const std::string derMultName = "derived/mult";
    const std::string derDivName = "derived/div";
    const std::string derPowName = "derived/pow";
    const std::string derSqrtName = "derived/sqrt";
    const std::string derMinName = "derived/min";
    const std::string derMaxName = "derived/max";
    const std::string derSumName = "derived/sum";
    const std::string derMeanName = "derived/mean";
    const std::string derVarianceName = "derived/variance";
    const std::string derStdevName = "derived/stdev";

    const adios2::Dims shape{static_cast<size_t>(Nx * size), static_cast<size_t>(Ny * size),
                             static_cast<size_t>(Nz * size)};
    const adios2::Dims start{static_cast<size_t>(Nx * rank), static_cast<size_t>(Ny * rank),
                             static_cast<size_t>(Nz * rank)};
    const adios2::Dims count{Nx, Ny, Nz};

    auto Ux = bpOut.DefineVariable<float>(varname[0], shape, start, count);
    auto Uy = bpOut.DefineVariable<float>(varname[1], shape, start, count);
    auto Uz = bpOut.DefineVariable<float>(varname[2], shape, start, count);
    // clang-format off
    bpOut.DefineDerivedVariable(derAgrAdd,
                                "x=" + varname[0] + "\n"
                                "add(x)",
                                mode);
    bpOut.DefineDerivedVariable(derAddName,
                                "x =" + varname[0] + " \n"
                                "y =" + varname[1] + " \n"
                                "z =" + varname[2] + " \n"
                                "x+y+z",
                                mode);
    bpOut.DefineDerivedVariable(derSubtrName,
                                "x =" + varname[0] + " \n"
                                "y =" + varname[1] + " \n"
                                "z =" + varname[2] + " \n"
                                "x-y-z",
                                mode);
    bpOut.DefineDerivedVariable(derMultName,
                                "x =" + varname[0] + " \n"
                                "y =" + varname[1] + " \n"
                                "z =" + varname[2] + " \n"
                                "x*y*z",
                                mode);
    bpOut.DefineDerivedVariable(derDivName,
                                "x =" + varname[0] + " \n"
                                "y =" + varname[1] + " \n"
                                "z =" + varname[2] + " \n"
                                "divide(x,y,z)",
                                mode);
    bpOut.DefineDerivedVariable(derPowName,
                                "x =" + varname[0] + " \n"
                                "pow(x)",
                                mode);
    bpOut.DefineDerivedVariable(derSqrtName,
                                "x =" + varname[0] + " \n"
                                "sqrt(x)",
                                mode);
    bpOut.DefineDerivedVariable(derMinName,
                                "x =" + varname[0] + " \n"
                                "min(x)",
                                mode);
    bpOut.DefineDerivedVariable(derMaxName,
                                "x =" + varname[0] + " \n"
                                "max(x)",
                                mode);
    bpOut.DefineDerivedVariable(derSumName,
                                "x =" + varname[0] + " \n"
                                "sum(x)",
                                mode);
    bpOut.DefineDerivedVariable(derMeanName,
                                "x =" + varname[0] + " \n"
                                "mean(x)",
                                mode);
    bpOut.DefineDerivedVariable(derVarianceName,
                                "x =" + varname[0] + " \n"
                                "variance(x)",
                                mode);
    bpOut.DefineDerivedVariable(derStdevName,
                                "x =" + varname[0] + " \n"
                                "stdev(x)",
                                mode);
    // clang-format on
    std::string filename = "derivedScalar.bp";
    adios2::Engine bpFileWriter = bpOut.Open(filename, adios2::Mode::Write);

    for (size_t i = 0; i < steps; i++)
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
    std::vector<float> readAgrAdd;
    std::vector<float> readSubtr;
    std::vector<float> readMult;
    std::vector<float> readDiv;
    std::vector<double> readPow;
    std::vector<double> readSqrt;
    std::vector<float> readMin, readMax, readSum, readMean, readVariance;
    std::vector<double> readStdev;

    float calcFloat;
    double calcDouble;
    float epsilon = (float)0.01;
    for (size_t i = 0; i < steps; i++)
    {
        bpFileReader.BeginStep();
        bpFileReader.Get(varname[0], readUx);
        bpFileReader.Get(varname[1], readUy);
        bpFileReader.Get(varname[2], readUz);
        bpFileReader.Get(derAddName, readAdd);
        bpFileReader.Get(derAgrAdd, readAgrAdd);
        bpFileReader.Get(derSubtrName, readSubtr);
        bpFileReader.Get(derMultName, readMult);
        bpFileReader.Get(derDivName, readDiv);
        bpFileReader.Get(derPowName, readPow);
        bpFileReader.Get(derSqrtName, readSqrt);
        bpFileReader.Get(derMinName, readMin);
        bpFileReader.Get(derMaxName, readMax);
        bpFileReader.Get(derSumName, readSum);
        bpFileReader.Get(derMeanName, readMean);
        bpFileReader.Get(derVarianceName, readVariance);
        bpFileReader.Get(derStdevName, readStdev);
        bpFileReader.EndStep();

        float min = std::numeric_limits<float>::max();
        float max = std::numeric_limits<float>::min();
        float sum = 0;

        for (size_t ind = 0; ind < Nx * Ny * Nz; ++ind)
        {
            calcFloat = readUx[ind] + readUy[ind] + readUz[ind];
            EXPECT_TRUE(fabs(calcFloat - readAdd[ind]) < epsilon);

            calcFloat = readUx[ind] - readUy[ind] - readUz[ind];
            EXPECT_TRUE(fabs(calcFloat - readSubtr[ind]) < epsilon);

            calcFloat = readUx[ind] * readUy[ind] * readUz[ind];
            EXPECT_TRUE(fabs(calcFloat - readMult[ind]) < epsilon);

            calcFloat = readUx[ind] / readUy[ind] / readUz[ind];
            EXPECT_TRUE(fabs(calcFloat - readDiv[ind]) < epsilon);

            calcDouble = std::pow(readUx[ind], 2);
            EXPECT_TRUE(fabs(calcDouble - readPow[ind]) < epsilon);

            calcDouble = std::sqrt(readUx[ind]);
            EXPECT_TRUE(fabs(calcDouble - readSqrt[ind]) < epsilon);
            min = std::min(min, readUx[ind]);
            max = std::max(max, readUx[ind]);
            sum += readUx[ind];
        }

        float mean = sum / (Nx * Ny * Nz);
        EXPECT_TRUE(fabs(min - readMin[0]) < epsilon);
        EXPECT_TRUE(fabs(max - readMax[0]) < epsilon);
        EXPECT_TRUE(fabs(sum - readSum[0]) < epsilon);
        EXPECT_TRUE(fabs(mean - readMean[0]) < epsilon);

        float variance = 0;

        // Calculate Variance now that mean is computed
        for (size_t ind = 0; ind < Nx * Ny * Nz; ++ind)
        {
            variance += (readUx[ind] - mean) * (readUx[ind] - mean);
        }
        variance /= (Nx * Ny * Nz);
        double stdev = std::sqrt(variance);

        EXPECT_TRUE(fabs(variance - readVariance[0]) < epsilon);
        EXPECT_TRUE(fabs(stdev - readStdev[0]) < epsilon);

        for (size_t ind = 0; ind < Nx * Ny; ++ind)
        {
            size_t start = ind * Nz;
            float calcA = 0;
            for (size_t z = 0; z < Nz; ++z)
            {
                calcA += readUx[z + start];
            }
            EXPECT_TRUE(fabs(calcA - readAgrAdd[ind]) < epsilon);
        }
    }
    bpFileReader.Close();
}

TEST_P(DerivedCorrectnessP, TrigCorrectnessTest)
{
    const size_t Nx = 10, Ny = 3, Nz = 6;
    const size_t steps = 2;
    // Application variable
    std::default_random_engine generator;
    std::uniform_real_distribution<double> randomDist(0.0, 100.0);
    std::uniform_real_distribution<double> sinDist(-1.0, 1.0);

    adios2::DerivedVarType mode = GetParam();
    std::cout << "Mode is " << mode << std::endl;

    std::vector<double> simArray1(Nx * Ny * Nz);
    std::vector<double> simArray2(Nx * Ny * Nz);
    for (size_t i = 0; i < Nx * Ny * Nz; ++i)
    {
        simArray1[i] = randomDist(generator);
        simArray2[i] = sinDist(generator);
    }

    adios2::ADIOS adios;

    adios2::IO bpOut = adios.DeclareIO("BPWriteSinExpression");

    std::string randDistVar = "sim/randDist";
    std::string sinDistVar = "sim/sinDist";
    std::string derSinName = "derived/sin";
    std::string derCosName = "derived/cos";
    std::string derTanName = "derived/tan";
    std::string derAsinName = "derived/asin";
    std::string derAcosName = "derived/acos";
    std::string derAtanName = "derived/atan";

    auto Ux = bpOut.DefineVariable<double>(randDistVar, {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    auto Uy = bpOut.DefineVariable<double>(sinDistVar, {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    // clang-format off
    bpOut.DefineDerivedVariable(derSinName,
                                "x =" + randDistVar + " \n"
                                "sin(x)",
                                mode);
    bpOut.DefineDerivedVariable(derCosName,
                                "x =" + randDistVar + " \n"
                                "cos(x)",
                                mode);
    bpOut.DefineDerivedVariable(derTanName,
                                "x =" + randDistVar + " \n"
                                "tan(x)",
                                mode);
    bpOut.DefineDerivedVariable(derAsinName,
                                "x =" + sinDistVar + " \n"
                                "asin(x)",
                                mode);
    bpOut.DefineDerivedVariable(derAcosName,
                                "x =" + sinDistVar + " \n"
                                "acos(x)",
                                mode);
    bpOut.DefineDerivedVariable(derAtanName,
                                "x =" + randDistVar + " \n"
                                "atan(x)",
                                mode);
    // clang-format on
    std::string filename = "expTrig.bp";
    adios2::Engine bpFileWriter = bpOut.Open(filename, adios2::Mode::Write);

    for (size_t i = 0; i < steps; i++)
    {
        bpFileWriter.BeginStep();
        bpFileWriter.Put(Ux, simArray1.data());
        bpFileWriter.Put(Uy, simArray2.data());
        bpFileWriter.EndStep();
    }
    bpFileWriter.Close();

    adios2::IO bpIn = adios.DeclareIO("BPReadExpression");
    adios2::Engine bpFileReader = bpIn.Open(filename, adios2::Mode::Read);

    std::vector<double> readRandDist;
    std::vector<double> readSinDist;
    std::vector<double> readSin;
    std::vector<double> readCos;
    std::vector<double> readTan;
    std::vector<double> readAsin;
    std::vector<double> readAcos;
    std::vector<double> readAtan;

    double calcTrig;
    double epsilon = (double)0.01;
    for (size_t i = 0; i < steps; i++)
    {
        bpFileReader.BeginStep();
        bpFileReader.Get(randDistVar, readRandDist);
        bpFileReader.Get(sinDistVar, readSinDist);

        bpFileReader.Get(derSinName, readSin);
        bpFileReader.Get(derCosName, readCos);
        bpFileReader.Get(derTanName, readTan);
        bpFileReader.Get(derAsinName, readAsin);
        bpFileReader.Get(derAcosName, readAcos);
        bpFileReader.Get(derAtanName, readAtan);
        bpFileReader.EndStep();

        for (size_t ind = 0; ind < Nx * Ny * Nz; ++ind)
        {
            calcTrig = std::sin(readRandDist[ind]);
            EXPECT_TRUE(fabs(calcTrig - readSin[ind]) < epsilon);
        }

        for (size_t ind = 0; ind < Nx * Ny * Nz; ++ind)
        {
            calcTrig = std::cos(readRandDist[ind]);
            EXPECT_TRUE(fabs(calcTrig - readCos[ind]) < epsilon);
        }

        for (size_t ind = 0; ind < Nx * Ny * Nz; ++ind)
        {
            calcTrig = std::tan(readRandDist[ind]);
            EXPECT_TRUE(fabs(calcTrig - readTan[ind]) < epsilon);
        }

        for (size_t ind = 0; ind < Nx * Ny * Nz; ++ind)
        {
            calcTrig = std::asin(readSinDist[ind]);
            EXPECT_TRUE(fabs(calcTrig - readAsin[ind]) < epsilon);
        }

        for (size_t ind = 0; ind < Nx * Ny * Nz; ++ind)
        {
            calcTrig = std::acos(readSinDist[ind]);
            EXPECT_TRUE(fabs(calcTrig - readAcos[ind]) < epsilon);
        }

        for (size_t ind = 0; ind < Nx * Ny * Nz; ++ind)
        {
            calcTrig = std::atan(readRandDist[ind]);
            EXPECT_TRUE(fabs(calcTrig - readAtan[ind]) < epsilon);
        }
    }
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
    std::cout << "Mode is " << mode << std::endl;

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
    std::string filename = "expVector.bp";
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
    std::string filename = "expCurl.bp";
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
    std::string filename = "expMagCurl.bp";
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
