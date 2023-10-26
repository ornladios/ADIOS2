#include <cstdint>
#include <cstring>

#include <iostream>
#include <stdexcept>
#include <numeric>
#include <random>
#include <vector>

#include <adios2.h>
#if ADIOS2_USE_MPI
#include <mpi.h>
#endif
//#include <gtest/gtest.h>

//TEST(DerivedGrammarTest, AddTwoDefinedVariables)
void AddTwoDefinedVariables()
{
    int rank, size = 0;
#if ADIOS2_USE_MPI
    int provided;

    // MPI_THREAD_MULTIPLE is only required if you enable the SST MPI_DP
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
#else
    rank = 0;
    size = 1;
#endif

    const size_t Nx = 2, Ny = 2, Nz = 10;
    const size_t steps = 2;
    /** Application variable */
    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution(0.0, 10.0);

    std::vector<float> simArray1(Nx * Ny * Nz);
    std::vector<float> simArray2(Nx * Ny * Nz);
    for (size_t i = 0; i < Nx * Ny * Nz; ++i)
    {
        simArray1[i] = distribution(generator);
        simArray2[i] = distribution(generator);
    }

#if ADIOS2_USE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    adios2::ADIOS adios;
#endif

    adios2::IO bpIO = adios.DeclareIO("BPExpression");
    bpIO.SetEngine("bp5");
    bpIO.DefineAttribute<int>("nsteps", steps);
    bpIO.DefineAttribute<int>("arraySize", Nx * Ny * Nz);
    bpIO.SetParameters("statslevel=1");
    bpIO.SetParameters("statsblocksize=5000");

    auto Ux =
        bpIO.DefineVariable<float>("sim/Ux", {Nz, Ny, size * Nx}, {0, 0, rank * Nx}, {Nz, Ny, Nx});
    auto Uy =
        bpIO.DefineVariable<float>("sim/Uy", {Nz, Ny, size * Nx}, {0, 0, rank * Nx}, {Nz, Ny, Nx});
    auto addU = bpIO.DefineDerivedVariable("derive/addU",
                                           "x:sim/Ux \n"
                                           "y:sim/Uy \n"
                                           "x+y",
					   adios2::DerivedVarType::StoreData);

    // TODO add Operation to magU

    std::string filename = "expMagnitude.bp";
    adios2::Engine bpFileWriter = bpIO.Open(filename, adios2::Mode::Write);

    for (int i = 0; i < steps; i++)
    {
        std::cout << "Start step " << i << std::endl;
        bpFileWriter.BeginStep();
        bpFileWriter.Put(Ux, simArray1.data());
        bpFileWriter.Put(Uy, simArray2.data());
        bpFileWriter.EndStep();
    }

    /** Create bp file, engine becomes unreachable after this*/
    bpFileWriter.Close();
    if (rank == 0)
    {
        std::cout << "Wrote file " << filename << " to disk. \n";
    }
#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif

}

//TEST(DerivedGrammarTest, AddExpressionNonBinary)
void AddExpressionNonBinary()
{
    int rank, size;
#if ADIOS2_USE_MPI
    int provided;

    // MPI_THREAD_MULTIPLE is only required if you enable the SST MPI_DP
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
#else
    rank = 0;
    size = 1;
#endif

    const size_t Nx = 2, Ny = 2, Nz = 10;
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

#if ADIOS2_USE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    adios2::ADIOS adios;
#endif

    adios2::IO bpIO = adios.DeclareIO("BPExpression");
    bpIO.SetEngine("bp5");
    bpIO.DefineAttribute<int>("nsteps", steps);
    bpIO.DefineAttribute<int>("arraySize", Nx * Ny * Nz);
    bpIO.SetParameters("statslevel=1");
    bpIO.SetParameters("statsblocksize=5000");

    std::vector<std::string> varpath = {"sim/Ux", "sim/Uy", "sim/Uz"};
    
    auto Ux =
        bpIO.DefineVariable<float>(varpath[0], {Nz, Ny, size * Nx}, {0, 0, rank * Nx}, {Nz, Ny, Nx});
    auto Uy =
        bpIO.DefineVariable<float>(varpath[1], {Nz, Ny, size * Nx}, {0, 0, rank * Nx}, {Nz, Ny, Nx});
    auto Uz =
        bpIO.DefineVariable<float>(varpath[2], {Nz, Ny, size * Nx}, {0, 0, rank * Nx}, {Nz, Ny, Nx});
    auto addU = bpIO.DefineDerivedVariable("derive/addU",
                                           "x:" + varpath[0] + " \n"
                                           "y:" + varpath[1] + " \n"
                                           "z:" + varpath[2] + " \n"
                                           "x+y+z",
					   adios2::DerivedVarType::StoreData);
    
    // TODO add Operation to magU

    std::string filename = "expMagnitude.bp";
    adios2::Engine bpFileWriter = bpIO.Open(filename, adios2::Mode::Write);

    for (int i = 0; i < steps; i++)
    {
        std::cout << "Start step " << i << std::endl;
        bpFileWriter.BeginStep();
        bpFileWriter.Put(Ux, simArray1.data());
        bpFileWriter.Put(Uy, simArray2.data());
        bpFileWriter.EndStep();
    }
    adios2::Expression addexpr = addU.m_expr;
    if (addexpr.operation != adios2::detail::ExpressionOperator::OP_ADD)
      {
	std::cout << "TEST: ADD EXPRESSION OPERATION INCORRECT" << std::endl;
      }
    if (addexpr.subexpr.size != 3)
      {
	std::cout << "TEST: ADD EXPRESSION SUBEXPR COUNT INCORRECT" << std::endl;
      }
    if (std::get<1>(addexpr.m_Expr.subexpr[0]) == varpath[0])
      {
	std::cout << "TEST: ADD EXPRESSION subexpr 0 should be " << varpath[0];
	std::cout << " not " << addexpr.m_Expr.subexpr[0] << std::endl;
      }
    if (std::get<1>(addexpr.m_Expr.subexpr[1]) == varpath[1])
      {
	std::cout << "TEST: ADD EXPRESSION subexpr 1 should be " << varpath[1];
	std::cout << " not " << addexpr.m_Expr.subexpr[1] << std::endl;
      }
    if (std::get<1>(addexpr.m_Expr.subexpr[2]) == varpath[2])
      {
	std::cout << "TEST: ADD EXPRESSION subexpr 2 should be " << varpath[2];
	std::cout << " not " << addexpr.m_Expr.subexpr[2] << std::endl;
      }
    std::cout << "TEST: ADD EXPRESSION complete" << std::endl;

    /** Create bp file, engine becomes unreachable after this*/
    bpFileWriter.Close();
    if (rank == 0)
    {
        std::cout << "Wrote file " << filename << " to disk. \n";
    }
#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif
}

int main(int argc, char **argv)
{
#if ADIOS2_USE_MPI
    int provided;

    // MPI_THREAD_MULTIPLE is only required if you enable the SST MPI_DP
    MPI_Init_thread(nullptr, nullptr, MPI_THREAD_MULTIPLE, &provided);
#endif

    AddTwoDefinedVariables();
    AddExpressionNonBinary();
    /*
    int result;
    ::testing::InitGoogleTest(&argc, argv);

    if (argc > 1)
    {
        engineName = std::string(argv[1]);
    }
    result = RUN_ALL_TESTS();

#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif

    return result;
    */
    return 0;
}
