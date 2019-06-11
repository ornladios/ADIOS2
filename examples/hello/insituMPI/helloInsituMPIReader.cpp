#include <algorithm>
#include <ios>      //std::ios_base::failure
#include <iostream> //std::cout
#include <mpi.h>
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>

#include <adios2.h>

#include "HelloInsituArgs.h"
#include "HelloInsituPrint.h"

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int wrank, wnproc;
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wnproc);

    const unsigned int color = 2;
    MPI_Comm mpiReaderComm;
    MPI_Comm_split(MPI_COMM_WORLD, color, wrank, &mpiReaderComm);

    int rank, nproc;
    MPI_Comm_rank(mpiReaderComm, &rank);
    MPI_Comm_size(mpiReaderComm, &nproc);

    try
    {
        HelloInsituArgs settings(false, argc, argv, rank, nproc);

        /** ADIOS class factory of IO class objects, Debug is ON by default */
        adios2::ADIOS adios(settings.configfile, mpiReaderComm);

        adios2::IO io = adios.DeclareIO("reader");
        adios2::Engine reader =
            io.Open(settings.streamname, adios2::Mode::Read, mpiReaderComm);

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
                // now read the variable
                if (vMyArray)
                {
                    throw std::runtime_error(
                        "Missing 'myArray' variable. This reader "
                        "example only works with the "
                        "hello_InsituMPIWriter application\n");
                }

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

            if (vMyArray)
                reader.Get(vMyArray, myArray.data());

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

    MPI_Finalize();

    return 0;
}
