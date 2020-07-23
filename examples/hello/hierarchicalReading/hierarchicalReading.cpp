
//Write data first
#include <ios>      //std::ios_base::failure
#include <iostream> //std::cout
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>

#include <adios2.h>
#if ADIOS2_USE_MPI
#include <mpi.h>
#endif

int main(int argc, char *argv[])
{
    int rank, size;
#if ADIOS2_USE_MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
#else
    rank = 0;
    size = 1;
#endif
    const int number_of_steps = 3;
    /** Application variable */
    std::string filename = "myVector_cpp.bp";
    std::vector<float> myFloats = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<int> myInts = {0, -1, -2, -3, -4, -5, -6, -7, -8, -9};
    const std::size_t Nx = myFloats.size();

    const std::string myString("Hello Variable String from rank " +
                               std::to_string(rank));

    try
    {
        /** ADIOS class factory of IO class objects */
#if ADIOS2_USE_MPI
        adios2::ADIOS adios(MPI_COMM_WORLD);
#else
        adios2::ADIOS adios;
#endif

        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines */
        adios2::IO bpIO = adios.DeclareIO("BPFile_N2N");

        /** global array : name, { shape (total) }, { start (local) }, {
         * count
         * (local) }, all are constant dimensions */
        adios2::Variable<float> bpFloats = bpIO.DefineVariable<float>(
            "/group1/subgroup1/bpFloats", {size * Nx}, {rank * Nx}, {Nx}, adios2::ConstantDims);

        adios2::Variable<int> bpInts = bpIO.DefineVariable<int>(
            "/group1/subgroup1/bpInts", {size * Nx}, {rank * Nx}, {Nx}, adios2::ConstantDims);

        adios2::Variable<std::string> bpString =
            bpIO.DefineVariable<std::string>("bpString");

        std::vector<std::string> myStrings = {"one", "two", "three"};
        bpIO.DefineAttribute<std::string>("Array_of_Strings", myStrings.data(),
                                          myStrings.size());


        /** Engine derived class, spawned to start IO operations */
        adios2::Engine bpFileWriter = bpIO.Open(filename, adios2::Mode::Append);

        for (int i = 0; i < number_of_steps; i++){

             bpFileWriter.BeginStep();
            /** Put variables for buffering, template type is optional */
             if (i%2 == 0)
                bpFileWriter.Put<float>(bpFloats, myFloats.data());
             else
                 bpFileWriter.Put(bpInts, myInts.data());
            // bpFileWriter.Put(bpString, myString);
             bpFileWriter.EndStep();
        }
        /** Create bp file, engine becomes unreachable after this*/
        bpFileWriter.Close();
        if (rank == 0)
        {
            std::cout << "Wrote file " << filename
                      << " to disk. It can now be read by running "
                         "./bin/hello_bpReader.\n";
        }
    }
    catch (std::invalid_argument &e)
    {
        std::cerr << "Invalid argument exception: " << e.what() << "\n";
#if ADIOS2_USE_MPI
        std::cerr << "STOPPING PROGRAM from rank " << rank << "\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
#endif
    }
    catch (std::ios_base::failure &e)
    {
        std::cerr << "IO System base failure exception: " << e.what() << "\n";
#if ADIOS2_USE_MPI
        std::cerr << "STOPPING PROGRAM from rank " << rank << "\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
#endif
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
#if ADIOS2_USE_MPI
        std::cerr << "STOPPING PROGRAM from rank " << rank << "\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
#endif
    }

    try
    {
        /** ADIOS class factory of IO class objects */
       // adios2::ADIOS adios(MPI_COMM_WORLD);
#if ADIOS2_USE_MPI
        adios2::ADIOS adios(MPI_COMM_WORLD);
#else
        adios2::ADIOS adios;
#endif

        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines */
        adios2::IO bpIO = adios.DeclareIO("ReadBP");

        const auto attributesInfo = bpIO.AvailableAttributes();

        for (const auto &attributeInfoPair : attributesInfo)
        {
            std::cout << "Attribute: " << attributeInfoPair.first;
            for (const auto &attributePair : attributeInfoPair.second)
            {
                std::cout << "\tKey: " << attributePair.first
                          << "\tValue: " << attributePair.second << "\n";
            }
            std::cout << "\n";
        }

        /** Engine derived class, spawned to start IO operations */
        adios2::Engine bpReader = bpIO.Open(filename, adios2::Mode::Read);

        for (int step = 0; step < number_of_steps; step++)
        {
            bpReader.BeginStep();
            const std::map<std::string, adios2::Params> variables =
                bpIO.AvailableVariables();

            for (const auto variablePair : variables)
            {
                std::cout << "Name: " << variablePair.first;

                for (const auto &parameter : variablePair.second)
                {
                    std::cout << "\t" << parameter.first << ": "
                              << parameter.second << "\n";
                }
            }

        /** Write variable for buffering */
        adios2::Variable<float> bpFloats =
            bpIO.InquireVariable<float>("/group1/subgroup1/bpFloats");
        adios2::Variable<int> bpInts = bpIO.InquireVariable<int>("/group1/subgroup1/bpInts");

        const std::size_t Nx = 10;
        if (bpFloats) // means found
        {
            std::vector<float> myFloats;

            // read only the chunk corresponding to our rank
            bpFloats.SetSelection({{Nx * rank}, {Nx}});
            // myFloats.data is pre-allocated
            bpReader.Get<float>(bpFloats, myFloats, adios2::Mode::Sync);

            if (rank == 0)
            {
                std::cout << "MyFloats: \n";
                for (const auto number : myFloats)
                {
                    std::cout << number << " ";
                }
                std::cout << "\n";
            }
        }

        if (bpInts) // means not found
        {
            std::vector<int> myInts;
            // read only the chunk corresponding to our rank
            bpInts.SetSelection({{Nx * rank}, {Nx}});

            bpReader.Get<int>(bpInts, myInts, adios2::Mode::Sync);

            if (rank == 0)
            {
                std::cout << "myInts: \n";
                for (const auto number : myInts)
                {
                    std::cout << number << " ";
                }
                std::cout << "\n";
            }
        }
            bpReader.EndStep();
        }
        /** Close bp file, engine becomes unreachable after this*/
        bpReader.Close();
    }
    catch (std::invalid_argument &e)
    {
        if (rank == 0)
        {
            std::cerr
                << "Invalid argument exception, STOPPING PROGRAM from rank "
                << rank << "\n";
            std::cerr << e.what() << "\n";
        }
#if ADIOS2_USE_MPI
        MPI_Abort(MPI_COMM_WORLD, 1);
#endif
    }
    catch (std::ios_base::failure &e)
    {
        if (rank == 0)
        {
            std::cerr << "IO System base failure exception, STOPPING PROGRAM "
                         "from rank "
                      << rank << "\n";
            std::cerr << e.what() << "\n";
            std::cerr << "The file " << filename << " does not exist."
                      << " Presumably this is because hello_bpWriter has not "
                         "been run."
                      << " Run ./hello_bpWriter before running this program.\n";
        }
#if ADIOS2_USE_MPI
        MPI_Abort(MPI_COMM_WORLD, 1);
#endif
    }
    catch (std::exception &e)
    {
        if (rank == 0)
        {
            std::cerr << "Exception, STOPPING PROGRAM from rank " << rank
                      << "\n";
            std::cerr << e.what() << "\n";
        }
#if ADIOS2_USE_MPI
        MPI_Abort(MPI_COMM_WORLD, 1);
#endif

    }

#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif
//now read part



    return 0;
}
