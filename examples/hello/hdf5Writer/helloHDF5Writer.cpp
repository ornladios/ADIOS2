/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloHDF5Writer.cpp: Simple self-descriptive example of how to write a
 * variable to a parallel HDF5 File using MPI processes.
 *
 *  Created on: March 20, 2017
 *      Author: Junmin
 */

#include <ios>      //std::ios_base::failure
#include <iostream> //std::cout
#include <mpi.h>
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>

#include <adios2.h>

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /** Application variable */
    std::vector<float> myFloats = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<int> myInts = {0, -1, -2, -3, -4, -5, -6, -7, -8, -9};
    double myScalar = 1.234;
    const std::size_t Nx = myFloats.size();

    try
    {
        /** ADIOS class factory of IO class objects, DebugON is recommended */
        adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);

        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines */
        adios2::IO hdf5IO = adios.DeclareIO("HDFFileIO");
        hdf5IO.SetEngine("HDF5");

        /** global array : name, { shape (total) }, { start (local) }, { count
         * (local) }, all are constant dimensions */
        adios2::Variable<float> h5Floats = hdf5IO.DefineVariable<float>(
            "h5Floats", {size * Nx}, {rank * Nx}, {Nx}, adios2::ConstantDims);

        adios2::Variable<int> h5Ints = hdf5IO.DefineVariable<int>(
            "h5Ints", {size * Nx}, {rank * Nx}, {Nx}, adios2::ConstantDims);

        adios2::Variable<double> h5ScalarDouble =
            hdf5IO.DefineVariable<double>("h5ScalarDouble");
        /** Engine derived class, spawned to start IO operations */
        adios2::Engine hdf5Writer =
            hdf5IO.Open("myVector.h5", adios2::Mode::Write);

        /** Write variable for buffering */
        hdf5Writer.Put<float>(h5Floats, myFloats.data());
        hdf5Writer.Put(h5Ints, myInts.data());
        hdf5Writer.Put(h5ScalarDouble, &myScalar);

        std::vector<int64_t> m_globalDims = {10, 20, 30, 40};
        hdf5IO.DefineAttribute<std::string>(
            "adios2_schema/version_major",
            std::to_string(ADIOS2_VERSION_MAJOR));
        hdf5IO.DefineAttribute<std::string>(
            "adios2_schema/version_minor",
            std::to_string(ADIOS2_VERSION_MINOR));
        hdf5IO.DefineAttribute<std::string>("/adios2_schema/mesh/type",
                                            "explicit");
        hdf5IO.DefineAttribute<std::int64_t>("adios2_schema/mesh/dimension0",
                                             m_globalDims[0]);
        hdf5IO.DefineAttribute<std::int64_t>("adios2_schema/mesh/dimension1",
                                             m_globalDims[1]);
        hdf5IO.DefineAttribute<std::int64_t>("adios2_schema/mesh/dimension2",
                                             m_globalDims[2]);
        hdf5IO.DefineAttribute<std::int64_t>("adios2_schema/mesh/dimension3",
                                             m_globalDims[3]);
        hdf5IO.DefineAttribute<std::int64_t>("adios2_schema/mesh/dimension-num",
                                             m_globalDims.size());

#ifdef NEVER
        /** Create h5 file, engine becomes unreachable after this*/
        hdf5Writer.Close();
#else
        hdf5Writer.Flush();
        hdf5Writer.Flush();
#endif
    }
    catch (std::invalid_argument &e)
    {
        std::cout << "Invalid argument exception, STOPPING PROGRAM from rank "
                  << rank << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::ios_base::failure &e)
    {
        std::cout
            << "IO System base failure exception, STOPPING PROGRAM from rank "
            << rank << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::exception &e)
    {
        std::cout << "Exception, STOPPING PROGRAM from rank " << rank << "\n";
        std::cout << e.what() << "\n";
    }

    MPI_Finalize();

    return 0;
}
