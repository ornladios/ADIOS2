/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <stdexcept>

#include <adios2.h>
#if ADIOS2_USE_MPI
#include <mpi.h>
#endif

void writer(adios2::ADIOS &adios, const std::string &greeting)
{
    adios2::IO io = adios.DeclareIO("hello-world-writer");
    adios2::Variable<std::string> varGreeting = io.DefineVariable<std::string>("Greeting");

    adios2::Engine writer = io.Open("hello-world-cpp.bp", adios2::Mode::Write);
    writer.BeginStep();
    writer.Put(varGreeting, greeting);
    writer.EndStep();
    writer.Close();
}

std::string reader(adios2::ADIOS &adios)
{
    adios2::IO io = adios.DeclareIO("hello-world-reader");
    adios2::Engine reader = io.Open("hello-world-cpp.bp", adios2::Mode::Read);
    reader.BeginStep();
    adios2::Variable<std::string> varGreeting = io.InquireVariable<std::string>("Greeting");
    std::string greeting;
    reader.Get(varGreeting, greeting);
    reader.EndStep();
    reader.Close();
    return greeting;
}

int main(int argc, char *argv[])
{
#if ADIOS2_USE_MPI
    MPI_Init(&argc, &argv);
#endif

    try
    {
#if ADIOS2_USE_MPI
        adios2::ADIOS adios(MPI_COMM_WORLD);
#else
        adios2::ADIOS adios;
#endif

        const std::string greeting = "Hello World from ADIOS2";
        writer(adios, greeting);

        const std::string message = reader(adios);
        std::cout << message << "\n";
    }
    catch (std::exception &e)
    {
        std::cout << "ERROR: ADIOS2 exception: " << e.what() << "\n";
#if ADIOS2_USE_MPI
        MPI_Abort(MPI_COMM_WORLD, -1);
#endif
    }

#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif

    return 0;
}
