/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <cstdint>
#include <cstring>

#include <iostream>
#include <stdexcept>
#include <vector>

#include <adios2.h>

#include <gtest/gtest.h>

#ifdef ADIOS2_HAVE_MPI
#include "mpi.h"
#endif

class InSituMPIMPMDExceptions : public ::testing::Test
{
public:
    InSituMPIMPMDExceptions() = default;
};

TEST_F(InSituMPIMPMDExceptions, Writer)
{
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
    adios2::IO dataManIO = adios.DeclareIO("Test");
    dataManIO.SetEngine("insitumpi");
    EXPECT_THROW(dataManIO.Open("filename", adios2::Mode::Write),
                 std::runtime_error);
}

TEST_F(InSituMPIMPMDExceptions, Reader)
{
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
    adios2::IO dataManIO = adios.DeclareIO("Test");
    dataManIO.SetEngine("insitumpi");
    EXPECT_THROW(dataManIO.Open("filename", adios2::Mode::Read),
                 std::runtime_error);
}

//******************************************************************************
// main
//******************************************************************************

int main(int argc, char **argv)
{
    MPI_Init(nullptr, nullptr);

    int result;
    ::testing::InitGoogleTest(&argc, argv);
    result = RUN_ALL_TESTS();

    MPI_Finalize();
    return result;
}
