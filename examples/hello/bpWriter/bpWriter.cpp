/*
Take the bpwriter C++ code and modify this to create several 1D arrays, taking an input of the size
of the array N, from the user input, and then allocate these arrays to memory X [ 0:2 * PI  done F =
cos(X) (although we will change this later) done deriv = - sin(X), determine this exactly and write
this. done DF/DX = the center derivative of F to 2nd order...  done DF/DX4 = the center derivative
of F to 4th order done DF/DX2F = the forward difference derivative to 2nd order done DF/DX2B = the
backward difference derivative to 2nd order done Write out these arrays to an adios file, along with
writing out an attribute (whatever you want) to the ADIOS file. see the readdocs to understand how
to do this. Now we want to read in the data from the ADIOS file  (call yourname.bp; ethan.bp) in
python. done Then we want to use matplotlib to graph the functions, and then to create several
variables   E1, E2, E3, E4, which are the errors from the actual derivative to the numerical
approximation E1 = deriv - DF/DX, E2= deriv-DF/DX4. make sure you also plot the errors Once this is
done, then we want to change the C++ program a little to use multiple processors, so that when the
user specifies N, we will divide this up amongst all of the processors, and we will have to use
send/receive with MPI to communicate the boundary points. Then run the MPI program, and then make
sure we get the same output from P processors (P<12) as we get with 1 processor.
*/
#include <ios>       //std::ios_base::failure
#include <iostream>  //std::cout
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>

#include <adios2.h>
#include <math.h>  // This allows me to use cosine
#include <stdio.h> // This allows me to use printf
#if ADIOS2_USE_MPI
#include <mpi.h>

#endif

#define PI 3.141592653589793
using namespace std;

int main(int argc, char *argv[])
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

    /** Application variable */

    // std::vector<float>* e_dat = new std::vector<float>;

    // std::vector<float> e_dat = new std::vector<float>;

    std::vector<float> myFloats = {0, 1, 2, 3, 4, 5, 6, 7, 8, -99, 100, 200, 300};
    // We comment this out so that we can create our own 1D array and then we will put the contents
    // into an ADIOS file
    std::vector<int> myInts = {0, -1, -2, -3, -4, -5, -6, -7, -8, -9, 10};
    const std::size_t Nx = myFloats.size();

    const std::string myString(
        "Hello I just made a change to THIS CODE!,  Variable String from rank " +
        std::to_string(rank));

    int i;

    // std::cout<<"Enter the size of the array"<<endl;
    size_t len = 1000;
    // std::cin>>len;

    // x values and y values of cos
    float *x = new float[len]();
    float *F = new float[len]();
    // centertered difference
    float *DFDX = new float[len]();
    float *DFDX4 = new float[len]();
    // foward differnce
    float *DFDX2F = new float[len]();

    // backward difference
    float *DFDX2B = new float[len]();
    // true deriveative -sin
    float *deriv = new float[len]();

    std::string filename = "ethan.bp";

    float h = (2 * PI - 0) / (len - 1);

    // the values of the Function F; the x coordinates x[i]; and the actual derivative deriv[i]
    for (i = 0; i < len; i++)
    {
        x[i] = 0.0 + i * h;
        F[i] = cos(x[i]);
        deriv[i] = -1 * sin(x[i]);
    }
    // These points cannot be calcaulated using this formula
    DFDX[0] = 0;
    DFDX[len - 1] = 0;

    for (i = 1; i < len - 1; i++)
    {
        DFDX[i] = 1 / (2 * h) * (F[i + 1] - F[i - 1]);
    }

    // cannot first two and last two values with the formula, but we can use equation above the two
    // of them
    DFDX4[0] = 0;
    DFDX4[1] = DFDX[1];
    DFDX4[len - 2] = DFDX[len - 2];
    DFDX4[len - 1] = 0;
    for (i = 2; i < len - 2; i++)
    {
        DFDX4[i] = 1 / (12 * h) * (-F[i + 2] + (8 * F[i + 1]) - (8 * F[i - 1]) + F[i - 2]);
    }

    // fowarc differncd: DF/DX2F  2nd order
    DFDX2F[len - 1] = 0;
    for (i = 0; i < len - 2; i++)
    {
        DFDX2F[i] = 1 / (2 * h) * ((-3 * F[i]) + (4 * F[i + 1]) - F[i + 2]);
    }

    // backward difference:DF/DX2B 2nd order
    DFDX2B[0] = 0;
    for (i = 2; i < len; i++)
    {
        DFDX2B[i] = 1 / (h * 2) * ((3 * F[i]) - (4 * F[i - 1]) + F[i - 2]);
    }

    // now lets get the values of that are set to 0
    DFDX[0] = DFDX2F[0];
    DFDX[len - 1] = DFDX2B[len - 1];
    DFDX4[0] = DFDX2F[0];
    DFDX4[len - 1] = DFDX2B[len - 1];

    // foward difference
    DFDX2F[len - 2] = DFDX2B[len - 2];
    DFDX2F[len - 1] = DFDX2B[len - 1];
    // backward differnce
    DFDX2B[0] = DFDX2F[0];
    DFDX2B[1] = DFDX2F[1];

    try
    {
        /** ADIOS class factory of IO class objects */
#if ADIOS2_USE_MPI
        adios2::ADIOS adios(MPI_COMM_WORLD);
#else
        adios2::ADIOS adios;
#endif
        std::cout << "\n from processor" << rank << "\n";
        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines */
        adios2::IO bpIO = adios.DeclareIO("BPFile_N2N");

        /** global array : name, { shape (total) }, { start (local) }, {
         * count
         * (local) }, all are constant dimensions */

        adios2::Variable<float> xOut = bpIO.DefineVariable<float>("x", {size * len}, {rank * len},
                                                                  {len}, adios2::ConstantDims);

        adios2::Variable<float> yOut = bpIO.DefineVariable<float>("y", {size * len}, {rank * len},
                                                                  {len}, adios2::ConstantDims);

        adios2::Variable<float> derivOut = bpIO.DefineVariable<float>(
            "DFDX", {size * len}, {rank * len}, {len}, adios2::ConstantDims);

        adios2::Variable<float> derivOut4 = bpIO.DefineVariable<float>(
            "DFDX4", {size * len}, {rank * len}, {len}, adios2::ConstantDims);

        adios2::Variable<float> derivfor = bpIO.DefineVariable<float>(
            "DFDX2F", {size * len}, {rank * len}, {len}, adios2::ConstantDims);

        adios2::Variable<float> derivbac = bpIO.DefineVariable<float>(
            "DFDX2B", {size * len}, {rank * len}, {len}, adios2::ConstantDims);

        /** Engine derived class, spawned to start IO operations */
        adios2::Engine bpWriter = bpIO.Open(filename, adios2::Mode::Write);

        bpWriter.BeginStep();
        /** Put variables for buffering, template type is optional */
        bpWriter.Put(xOut, x);
        bpWriter.Put(yOut, F);
        bpWriter.Put(derivOut, DFDX);
        bpWriter.Put(derivOut4, DFDX4);
        bpWriter.Put(derivfor, DFDX2F);
        bpWriter.Put(derivbac, DFDX2B);

        bpWriter.EndStep();

        /** Create bp file, engine becomes unreachable after this*/
        bpWriter.Close();
        if (rank == 0)
        {
            std::cout << "Wrote file " << filename
                      << " to disk. It can now be read by running "
                         "./bin/adios2_hello_bpReader.\n";
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

#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif

    return 0;
}
