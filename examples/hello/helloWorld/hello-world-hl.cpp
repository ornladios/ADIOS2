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

void writer(const std::string &greeting)
{
#if ADIOS2_USE_MPI
    adios2::fstream out("hello-world-hl-cpp.bp", adios2::fstream::out, MPI_COMM_WORLD);
#else
    adios2::fstream out("hello-world-hl-cpp.bp", adios2::fstream::out);
#endif

    out.write("Greeting", greeting);
    out.close();
}

std::string reader()
{
#if ADIOS2_USE_MPI
    adios2::fstream in("hello-world-hl-cpp.bp", adios2::fstream::in, MPI_COMM_WORLD);
#else
    adios2::fstream in("hello-world-hl-cpp.bp", adios2::fstream::in);
#endif

    for (adios2::fstep iStep; adios2::getstep(in, iStep);)
    {
        const std::vector<std::string> greetings = in.read<std::string>("Greeting");
        return greetings.front();
    }
    return "";
}

int main(int argc, char *argv[])
{
#if ADIOS2_USE_MPI
    MPI_Init(&argc, &argv);
#endif

    try
    {
        const std::string greeting = "Hello World from ADIOS2";
        writer(greeting);

        const std::string message = reader();
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
