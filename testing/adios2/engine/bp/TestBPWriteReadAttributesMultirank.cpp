/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TestBPWriteReadAttributesMultirank.cpp :
 * Test if attributes from different ranks will get into the global metadata
 *  Created on: Jul 11, 2018
 *      Author: Norbert Podhorski pnb@ornl.gov
 */
#include <cstdint>
#include <string>

#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

std::string engineName; // comes from command line

class BPWriteReadAttributeTestMultirank : public ::testing::Test
{
public:
    BPWriteReadAttributeTestMultirank() = default;
};

// ADIOS2  declare attributes on multiple ranks
TEST_F(BPWriteReadAttributeTestMultirank, ADIOS2BPWriteReadArrayTypes)
{
    const std::string fName =
        "foo" + std::string(&adios2::PathSeparator, 1) +
        "ADIOS2BPWriteAttributeMultirankReadArrayTypes.bp";

    int mpiRank = 0;
#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
#endif

    // a different variable and associated attribute on each rank
    std::string varpath = "rank" + std::to_string(mpiRank) +
                          std::string(&adios2::PathSeparator, 1) + "value";
    std::string attrpath =
        varpath + std::string(&adios2::PathSeparator, 1) + "description";
    std::string desc =
        "This variable and associated attribute was created on rank " +
        std::to_string(mpiRank);

// Write test data using BP
#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(true);
#endif
    {
        adios2::IO io = adios.DeclareIO("TestIO");

        auto var = io.DefineVariable<int>(varpath);
        auto attr = io.DefineAttribute<std::string>(attrpath, desc);

        std::cout << "Rank " << mpiRank << " create variable " << varpath
                  << " = " << mpiRank << " and attribute " << attrpath
                  << " = \"" << desc << "\"" << std::endl;

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }
        io.AddTransport("file");

        adios2::Engine engine = io.Open(fName, adios2::Mode::Write);
        engine.BeginStep();
        engine.Put(varpath, mpiRank);
        engine.EndStep();
        engine.Close();
    }
    // reader
    {
        adios2::IO ioRead = adios.DeclareIO("ioRead");

        if (!engineName.empty())
        {
            ioRead.SetEngine(engineName);
        }

        adios2::Engine bpRead = ioRead.Open(fName, adios2::Mode::Read);

        auto var = ioRead.InquireVariable<int>(varpath);
        EXPECT_TRUE(var);
        ASSERT_EQ(var.Name(), varpath);
        ASSERT_EQ(var.Type(), "int");

        int value;
        bpRead.Get(varpath, &value);
        bpRead.PerformGets();
        EXPECT_EQ(value, mpiRank);

        auto attr = ioRead.InquireAttribute<std::string>(attrpath);
        EXPECT_TRUE(attr);
        ASSERT_EQ(attr.Name(), attrpath);
        ASSERT_EQ(attr.Data().size() == 1, true);
        std::cout << "Rank " << mpiRank << " attribute is " << attrpath
                  << " = \"" << attr.Data()[0] << "\"" << std::endl;
        ASSERT_EQ(attr.Type(), "string");

        bpRead.Close();
    }
}

//******************************************************************************
// main
//******************************************************************************

int main(int argc, char **argv)
{
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(nullptr, nullptr);
#endif

    int result;
    ::testing::InitGoogleTest(&argc, argv);

    if (argc > 1)
    {
        engineName = std::string(argv[1]);
    }
    result = RUN_ALL_TESTS();

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
