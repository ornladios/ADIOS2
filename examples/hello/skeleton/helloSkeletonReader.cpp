#include <algorithm>
#include <ios>       //std::ios_base::failure
#include <iostream>  //std::cout
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>

#include <adios2.h>

#include "HelloSkeletonArgs.h"
#include "HelloSkeletonPrint.h"

int main(int argc, char *argv[])
{
    int rank = 0, nproc = 1;

#ifdef ADIOS2_HAVE_MPI
    int wrank = 0, wnproc = 1;
    MPI_Comm mpiReaderComm;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wnproc);

    const unsigned int color = 2;
    MPI_Comm_split(MPI_COMM_WORLD, color, wrank, &mpiReaderComm);

    MPI_Comm_rank(mpiReaderComm, &rank);
    MPI_Comm_size(mpiReaderComm, &nproc);
#endif

    try
    {
        HelloSkeletonArgs settings(false, argc, argv, rank, nproc);

/** ADIOS class factory of IO class objects, Debug is ON by default */
#ifdef ADIOS2_HAVE_MPI
        adios2::ADIOS adios(settings.configfile, mpiReaderComm);
#else
        adios2::ADIOS adios(settings.configfile);
#endif
        adios2::IO io = adios.DeclareIO("reader");
        adios2::Engine reader =
            io.Open(settings.streamname, adios2::Mode::Read);

        int step = 0;
        adios2::Variable<float> vMyArray;
        adios2::Dims count, start;
        std::vector<float> myArray;

        while (true)
        {
            adios2::StepStatus status =
                reader.BeginStep(adios2::StepMode::Read, 60.0f);
            if (status != adios2::StepStatus::OK)
            {
                break;
            }

            if (step == 0)
            {
                // this just discovers in the metadata file that the variable
                // exists
                vMyArray = io.InquireVariable<float>("myArray");
                if (!vMyArray)
                {
                    std::cout
                        << "Missing 'myArray' variable. The Skeleton reader "
                           "engine must retrieve variables from the writer and "
                           "create Variable objects before they can be "
                           "inquired\n";
                    count.push_back(0);
                    count.push_back(0);
                    start.push_back(0);
                    start.push_back(0);
                }
                else
                {
                    // now read the variable
                    // Get the read decomposition
                    settings.DecomposeArray(vMyArray.Shape()[0],
                                            vMyArray.Shape()[1]);
                    count.push_back(settings.ndx);
                    count.push_back(settings.ndy);
                    start.push_back(settings.offsx);
                    start.push_back(settings.offsy);

                    vMyArray.SetSelection({start, count});
                    size_t elementsSize = count[0] * count[1];
                    myArray.resize(elementsSize);
                }
            }

            if (vMyArray)
            {
                reader.Get(vMyArray, myArray.data());
            }

            reader.EndStep();

            printDataStep(myArray.data(), count, start, rank, step);
            ++step;
        }
        reader.Close();
    }
    catch (std::invalid_argument &e)
    {
        std::cout << "Invalid argument exception, STOPPING PROGRAM from rank "
                  << rank << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::ios_base::failure &e)
    {
        std::cout << "IO System base failure exception, STOPPING PROGRAM "
                     "from rank "
                  << rank << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::exception &e)
    {
        std::cout << "Exception, STOPPING PROGRAM from rank " << rank << "\n";
        std::cout << e.what() << "\n";
    }

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return 0;
}
