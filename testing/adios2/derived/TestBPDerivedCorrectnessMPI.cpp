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

class DerivedCorrectnessMPI : public ::testing::Test
{
public:
    DerivedCorrectnessMPI() = default;
};

class DerivedCorrectnessMPIP : public DerivedCorrectnessMPI,
                               public ::testing::WithParamInterface<adios2::DerivedVarType>
{
protected:
    adios2::DerivedVarType GetThreads() { return GetParam(); };
};

TEST_P(DerivedCorrectnessMPIP, ScalarFunctionsCorrectnessTest)
{
    int mpiRank = 0, mpiSize = 1;
    const size_t Nx = 2, Ny = 3, Nz = 4;
#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    const std::string filename("ADIOS2BPWriteDerivedScalar_MPI.bp");
#else
    const std::string filename("ADIOS2BPWriteDerivedScalar.bp");
#endif

    std::vector<std::string> varname = {"sim/Ux", "sim/Uy", "sim/Uz"};
    const std::string derAgrAdd = "derived/agradd";
    const std::string derAddName = "derived/add";
    const std::string derSubtrName = "derived/subtr";
    const std::string derMultName = "derived/mult";
    const std::string derDivName = "derived/div";
    const std::string derPowName = "derived/pow";
    const std::string derSqrtName = "derived/sqrt";

    { // write distributed over mpiSize processes
        std::default_random_engine generator;
        std::uniform_real_distribution<float> distribution(0.0, 10.0);
        adios2::DerivedVarType mode = GetParam();

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

        adios2::IO bpOut = adios.DeclareIO("WriteScalarExpression");

        auto Ux = bpOut.DefineVariable<float>(varname[0], {Nx * mpiSize, Ny, Nz},
                                              {Nx * mpiRank, 0, 0}, {Nx, Ny, Nz});
        auto Uy = bpOut.DefineVariable<float>(varname[1], {Nx * mpiSize, Ny, Nz},
                                              {Nx * mpiRank, 0, 0}, {Nx, Ny, Nz});
        auto Uz = bpOut.DefineVariable<float>(varname[2], {Nx * mpiSize, Ny, Nz},
                                              {Nx * mpiRank, 0, 0}, {Nx, Ny, Nz});
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
        // clang-format on
        adios2::Engine bpFileWriter = bpOut.Open(filename, adios2::Mode::Write);

        bpFileWriter.BeginStep();
        bpFileWriter.Put(Ux, simArray1.data());
        bpFileWriter.Put(Uy, simArray2.data());
        bpFileWriter.Put(Uz, simArray3.data());
        bpFileWriter.EndStep();
        bpFileWriter.Close();
    }

    if (mpiRank == 0)
    { // read on one process
        adios2::ADIOS adios;
        adios2::IO bpIn = adios.DeclareIO("ReadScalarExpression");
        adios2::Engine bpFileReader = bpIn.Open(filename, adios2::Mode::Read);

        std::vector<float> readUx(mpiSize * Nx * Ny * Nz);
        std::vector<float> readUy(mpiSize * Nx * Ny * Nz);
        std::vector<float> readUz(mpiSize * Nx * Ny * Nz);
        std::vector<float> readAdd(mpiSize * Nx * Ny * Nz);
        std::vector<float> readAgrAdd(mpiSize * Nx * Ny * Nz);
        std::vector<float> readSubtr(mpiSize * Nx * Ny * Nz);
        std::vector<float> readMult(mpiSize * Nx * Ny * Nz);
        std::vector<float> readDiv(mpiSize * Nx * Ny * Nz);
        std::vector<double> readPow(mpiSize * Nx * Ny * Nz);
        std::vector<double> readSqrt(mpiSize * Nx * Ny * Nz);

        float calcFloat;
        double calcDouble;
        float epsilon = (float)0.01;
        bpFileReader.BeginStep();
        auto varUx = bpIn.InquireVariable<float>(varname[0]);
        auto varUy = bpIn.InquireVariable<float>(varname[1]);
        auto varUz = bpIn.InquireVariable<float>(varname[2]);
        auto varAdd = bpIn.InquireVariable<float>(derAddName);
        auto varAgrAdd = bpIn.InquireVariable<float>(derAgrAdd);
        auto varSubtr = bpIn.InquireVariable<float>(derSubtrName);
        auto varMult = bpIn.InquireVariable<float>(derMultName);
        auto varDiv = bpIn.InquireVariable<float>(derDivName);
        auto varPow = bpIn.InquireVariable<double>(derPowName);
        auto varSqrt = bpIn.InquireVariable<double>(derSqrtName);

        bpFileReader.Get(varUx, readUx);
        bpFileReader.Get(varUy, readUy);
        bpFileReader.Get(varUz, readUz);
        bpFileReader.Get(varAdd, readAdd);
        bpFileReader.Get(varAgrAdd, readAgrAdd);

        bpFileReader.Get(varSubtr, readSubtr);
        bpFileReader.Get(varMult, readMult);
        bpFileReader.Get(varDiv, readDiv);
        bpFileReader.Get(varPow, readPow);
        bpFileReader.Get(varSqrt, readSqrt);
        bpFileReader.EndStep();

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
        }

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
        bpFileReader.Close();
    }
}

TEST_P(DerivedCorrectnessMPIP, TrigCorrectnessTest)
{
    int mpiRank = 0, mpiSize = 1;
    const size_t Nx = 2, Ny = 3, Nz = 4;
    adios2::DerivedVarType mode = GetParam();
#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    const std::string filename("ADIOS2BPWriteDerivedTrig_MPI.bp");
#else
    const std::string filename("ADIOS2BPWriteDerivedTrig.bp");
#endif

    std::string randDistVar = "sim/randDist";
    std::string sinDistVar = "sim/sinDist";
    std::string derSinName = "derived/sin";
    std::string derCosName = "derived/cos";
    std::string derTanName = "derived/tan";
    std::string derAsinName = "derived/asin";
    std::string derAcosName = "derived/acos";
    std::string derAtanName = "derived/atan";

    { // write distributed over mpiSize processes
        std::default_random_engine generator;
        std::uniform_real_distribution<double> randomDist(0.0, 100.0);
        std::uniform_real_distribution<double> sinDist(-1.0, 1.0);

        std::vector<double> simArray1(Nx * Ny * Nz);
        std::vector<double> simArray2(Nx * Ny * Nz);
        for (size_t i = 0; i < Nx * Ny * Nz; ++i)
        {
            simArray1[i] = randomDist(generator);
            simArray2[i] = sinDist(generator);
        }
#if ADIOS2_USE_MPI
        adios2::ADIOS adios(MPI_COMM_WORLD);
#else
        adios2::ADIOS adios;
#endif

        adios2::IO bpOut = adios.DeclareIO("BPWriteSinExpression");

        auto Ux = bpOut.DefineVariable<double>(randDistVar, {Nx * mpiSize, Ny, Nz},
                                               {Nx * mpiRank, 0, 0}, {Nx, Ny, Nz});
        auto Uy = bpOut.DefineVariable<double>(sinDistVar, {Nx * mpiSize, Ny, Nz},
                                               {Nx * mpiRank, 0, 0}, {Nx, Ny, Nz});
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
        adios2::Engine bpFileWriter = bpOut.Open(filename, adios2::Mode::Write);

        bpFileWriter.BeginStep();
        bpFileWriter.Put(Ux, simArray1.data());
        bpFileWriter.Put(Uy, simArray2.data());
        bpFileWriter.EndStep();
        bpFileWriter.Close();
    }

    if (mpiRank == 0)
    { // read on one process
        adios2::ADIOS adios;
        adios2::IO bpIn = adios.DeclareIO("BPReadExpression");
        adios2::Engine bpFileReader = bpIn.Open(filename, adios2::Mode::Read);

        std::vector<double> readRandDist(mpiSize * Nx * Ny * Nz);
        std::vector<double> readSinDist(mpiSize * Nx * Ny * Nz);
        std::vector<double> readSin(mpiSize * Nx * Ny * Nz);
        std::vector<double> readCos(mpiSize * Nx * Ny * Nz);
        std::vector<double> readTan(mpiSize * Nx * Ny * Nz);
        std::vector<double> readAsin(mpiSize * Nx * Ny * Nz);
        std::vector<double> readAcos(mpiSize * Nx * Ny * Nz);
        std::vector<double> readAtan(mpiSize * Nx * Ny * Nz);

        double calcTrig;
        double epsilon = (double)0.01;
        bpFileReader.BeginStep();

        auto varUx = bpIn.InquireVariable<double>(randDistVar);
        auto varUy = bpIn.InquireVariable<double>(sinDistVar);
        auto varSin = bpIn.InquireVariable<double>(derSinName);
        auto varCos = bpIn.InquireVariable<double>(derCosName);
        auto varTan = bpIn.InquireVariable<double>(derTanName);
        auto varAsin = bpIn.InquireVariable<double>(derAsinName);
        auto varAcos = bpIn.InquireVariable<double>(derAcosName);
        auto varAtan = bpIn.InquireVariable<double>(derAtanName);

        bpFileReader.Get(varUx, readRandDist);
        bpFileReader.Get(varUy, readSinDist);
        bpFileReader.Get(varSin, readSin);
        bpFileReader.Get(varCos, readCos);
        bpFileReader.Get(varTan, readTan);
        bpFileReader.Get(varAsin, readAsin);
        bpFileReader.Get(varAcos, readAcos);
        bpFileReader.Get(varAtan, readAtan);
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
        bpFileReader.Close();
    }
}

INSTANTIATE_TEST_SUITE_P(DerivedCorrectnessMPI, DerivedCorrectnessMPIP,
                         ::testing::Values(adios2::DerivedVarType::StatsOnly,
                                           adios2::DerivedVarType::ExpressionString,
                                           adios2::DerivedVarType::StoreData));

int main(int argc, char **argv)
{
#if ADIOS2_USE_MPI
    int provided;

    // MPI_THREAD_MULTIPLE is only required if you enable the SST MPI_DP
    MPI_Init_thread(nullptr, nullptr, MPI_THREAD_MULTIPLE, &provided);
#endif

    int result;
    ::testing::InitGoogleTest(&argc, argv);

    result = RUN_ALL_TESTS();

#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif

    return result;
}
