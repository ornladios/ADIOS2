/*
 * HDF5Writer.cpp
 *
 *  Created on: March 20, 2017
 *      Author: Junmin
 */

#include <iostream>
#include <vector>

#include <mpi.h>

#define ADIOS_HAVE_PHDF5 // so hdf5 related items are loaded in ADIOS_CPP.h
//#include "ADIOS_CPP.h"
#include "adios2.h"
#include "adios2/engine/hdf5/HDF5ReaderP.h"

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const bool adiosDebug = true;
    adios::ADIOS adios(MPI_COMM_WORLD, adios::Verbose::INFO, adiosDebug);

    // Application variable
    const std::size_t intDim1 = 4;
    const std::size_t intDim2 = 3;
    std::vector<int> myInts = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21};

    std::vector<double> myDoubles = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    const std::size_t Nx = myDoubles.size();

    std::vector<std::complex<float>> myCFloats;
    const std::size_t ComplexDataSize = 3;
    myCFloats.reserve(ComplexDataSize);
    myCFloats.emplace_back(1, 3);
    myCFloats.emplace_back(2, 2);
    myCFloats.emplace_back(3, 1);

    std::vector<std::complex<double>> myCDoubles;
    myCDoubles.reserve(ComplexDataSize);
    myCDoubles.emplace_back(1.1, -3.3);
    myCDoubles.emplace_back(2.1, -2.2);
    myCDoubles.emplace_back(3.1, -1.1);

    std::vector<std::complex<long double>> myCLongDoubles;
    myCLongDoubles.reserve(ComplexDataSize);
    myCLongDoubles.emplace_back(1.11, -3.33);
    myCLongDoubles.emplace_back(2.11, -2.22);
    myCLongDoubles.emplace_back(3.11, -1.11);

    std::size_t doubleVCount = Nx / size;
    std::size_t complexCount = ComplexDataSize / size;
    std::size_t intCountDim1 = intDim1 / size;

    std::size_t doubleVOffset = rank * doubleVCount;
    std::size_t complexOffset = rank * complexCount;
    std::size_t intOffsetDim1 = rank * intCountDim1;
    std::size_t intOffsetDim2 = 0;

    if ((size > 1) && (rank == size - 1))
    {
        doubleVCount = Nx - rank * (Nx / size);
        complexCount = ComplexDataSize - rank * (ComplexDataSize / size);
        intCountDim1 = intDim1 - rank * (intDim1 / size);
    }

    std::cout<<" rank="<<rank<<" of "<<size<<",  dim1 count: "<<intCountDim1<<", offset: "<<intOffsetDim1<<std::endl;
    std::cout<<" intOffsetDim2="<<intOffsetDim2<<" "<<intDim2<<std::endl;
    try
    {
        // Define variable and local size
        auto &ioMyInts =
            adios.DefineVariable<int>("myInts", {intCountDim1, intDim2}, {4, 3},
                                      {intOffsetDim1, intOffsetDim2});
        auto &ioMyDoubles = adios.DefineVariable<double>(
            "myDoubles", {doubleVCount}, {Nx}, {doubleVOffset});
        auto &ioMyCFloats = adios.DefineVariable<std::complex<float>>(
            "myCFloats", {complexCount}, {3}, {complexOffset});
        auto &ioMyCDoubles = adios.DefineVariable<std::complex<double>>(
            "myCDoubles", {complexCount}, {3}, {complexOffset});
        auto &ioMyCLongDoubles =
            adios.DefineVariable<std::complex<long double>>(
                "myCLongDoubles", {complexCount}, {3}, {complexOffset});

        // Define method for engine creation, it is basically straight-forward
        // parameters
        adios::Method &HDF5Settings = adios.DeclareMethod("hdf5");
        HDF5Settings.SetEngine("HDF5Writer");
        HDF5Settings.SetParameters("chunck=yes", "collectiveIO=yes");
        // HDF5Settings.AddTransport( "Mdtm", "localIP=128.0.0.0.1",
        // "remoteIP=128.0.0.0.2", "tolerances=1,2,3" );

        // Create engine smart pointer to HDF5 Engine due to polymorphism,
        // Open returns a smart pointer to Engine containing the Derived class
        // HDF5
        auto HDF5Writer = adios.Open("test.h5", "w", HDF5Settings);

        if (HDF5Writer == nullptr)
            throw std::ios_base::failure(
                "ERROR: failed to create HDF5 I/O engine at Open\n");

	int ts = 0;
	int totalts = 3;
	while (true) {
	  if (rank == 0) {
	    std::cout<<" total timesteps: "<<totalts<<" curr: "<<ts<<" th"<<std::endl;
	  }
	  HDF5Writer->Write(ioMyDoubles, myDoubles.data() +
			    doubleVOffset); // Base class Engine
	  // own the Write<T>
	  // that will call
	  // overloaded Write
	  // from Derived
	  HDF5Writer->Write(ioMyInts,
			    myInts.data() + (intOffsetDim1 * intDim2 ));
	  
	  HDF5Writer->Write(ioMyCFloats, myCFloats.data() + complexOffset);
	  HDF5Writer->Write(ioMyCDoubles, myCDoubles.data() + complexOffset);
	  HDF5Writer->Write(ioMyCLongDoubles,
			    myCLongDoubles.data() + complexOffset);
	  ts++;
	  if (ts >= totalts) {
	    break;
	  }
	  HDF5Writer->Advance();
	}
	HDF5Writer->Close();

	// now read out:
	/*
        HDF5Settings.SetEngine("HDF5Reader");
	std::cout<<"... Testing a copy of test.h5, [test1.h5] , b/c engine does not decrease name count !! "<<std::endl;
	auto HDF5Reader = adios.Open("test1.h5", "r", HDF5Settings);

	//int findts = HDF5Reader->getNumTimeSteps();
	//HDF5Reader->InquireVariableDouble("wrongMyDoubles", true);
	HDF5Reader->InquireVariableDouble(ioMyDoubles.m_Name, true);
	*/
	adios::HDF5Reader myReader(adios, "test.h5", "r", MPI_COMM_WORLD, HDF5Settings);
	double values[15];

#ifndef NEVER	
	ts = 0;
	while (ts < totalts) {    
	  myReader.ReadMe(ioMyDoubles, values, H5T_NATIVE_DOUBLE);    
	  myReader.Advance();
	  ts++;
	}

#else
	myReader.ReadMe(ioMyDoubles, values, H5T_NATIVE_DOUBLE);
	myReader.Close();
#endif

    }
    catch (std::invalid_argument &e)
    {
        if (rank == 0)
        {
            std::cout << "Invalid argument exception, STOPPING PROGRAM\n";
            std::cout << e.what() << "\n";
        }
    }
    catch (std::ios_base::failure &e)
    {
        if (rank == 0)
        {
            std::cout << "System exception, STOPPING PROGRAM\n";
            std::cout << e.what() << "\n";
        }
    }
    catch (std::exception &e)
    {
        if (rank == 0)
        {
            std::cout << "Exception, STOPPING PROGRAM\n";
            std::cout << e.what() << "\n";
        }
    }

    MPI_Finalize();

    return 0;
}
