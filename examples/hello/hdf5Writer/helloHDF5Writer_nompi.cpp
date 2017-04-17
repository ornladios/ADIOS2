/*
 * HDF5Writer.cpp
 *
 *  Created on: March 20, 2017
 *      Author: Junmin
 */

#include <iostream>
#include <vector>

#include "ADIOS_CPP.h"

int main(int argc, char *argv[])
{
    const bool adiosDebug = true;
    adios::ADIOS adios(adios::Verbose::INFO, adiosDebug);

    // Application variable
    const std::size_t intDim1 = 4;
    const std::size_t intDim2 = 3;
    std::vector<int> myInts = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21};

    std::vector<double> myDoubles = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    const std::size_t Nx = myDoubles.size();

    std::vector<std::complex<float>> myCFloats;
    const std::size_t CFloatSize = 3;
    myCFloats.reserve(CFloatSize);
    myCFloats.emplace_back(1, 3);
    myCFloats.emplace_back(2, 2);
    myCFloats.emplace_back(3, 1);

    std::vector<std::complex<double>> myCDoubles;
    const std::size_t CDoubleSize = 3;
    myCDoubles.reserve(CDoubleSize);
    myCDoubles.emplace_back(1, 3);
    myCDoubles.emplace_back(2, 2);
    myCDoubles.emplace_back(3, 1);

    std::size_t doubleVCount = Nx / size;
    std::size_t floatCount = CFloatSize / size;
    std::size_t intCountDim1 = intDim1 / size;

    std::size_t doubleVOffset = rank * doubleVCount;
    std::size_t floatOffset = rank * floatCount;
    std::size_t intOffsetDim1 = rank * intCountDim1;
    std::size_t intOffsetDim2 = 0;

    if ((size > 1) && (rank == size - 1))
    {
        doubleVCount = Nx - rank * (Nx / size);
        floatCount = CFloatSize - rank * (CFloatSize / size);
        intCountDim1 = intDim1 - rank * (intDim1 / size);
    }

    try
    {
        // Define variable and local size
        // auto& ioMyDoubles = adios.DefineVariable<double>( "myDoubles", {Nx},
        // {Nx} );
        // auto& ioMyCFloats = adios.DefineVariable<std::complex<float>>(
        // "myCFloats", {3}, {3} );
        // auto& ioMyInts = adios.DefineVariable<int>( "myInts", {4,3}, {4,3} );

        auto &ioMyDoubles = adios.DefineVariable<double>(
            "myDoubles", {doubleVCount}, {Nx}, {doubleVOffset});
        auto &ioMyCFloats = adios.DefineVariable<std::complex<float>>(
            "myCFloats", {floatCount}, {3}, {floatOffset});
        auto &ioMyCDoubles = adios.DefineVariable<std::complex<double>>(
            "myCDoubles", {floatCount}, {3}, {floatOffset});
        auto &ioMyInts =
            adios.DefineVariable<int>("myInts", {intCountDim1, intDim2}, {4, 3},
                                      {intOffsetDim1, intOffsetDim2});

        // Define method for engine creation, it is basically straight-forward
        // parameters
        adios::Method &HDF5Settings =
            adios.DeclareMethod("HDF5Writer"); // default method type is Writer
        HDF5Settings.SetParameters("chunck=yes", "collectiveIO=yes");
        // HDF5Settings.AddTransport( "Mdtm", "localIP=128.0.0.0.1",
        // "remoteIP=128.0.0.0.2", "tolerances=1,2,3" );

        // Create engine smart pointer to HDF5 Engine due to polymorphism,
        // Open returns a smart pointer to Engine containing the Derived class
        // HDF5
        auto HDF5Writer = adios.Open("test.bp", "w", HDF5Settings);

        if (HDF5Writer == nullptr)
            throw std::ios_base::failure(
                "ERROR: failed to create HDF5 I/O engine at Open\n");

        HDF5Writer->Write(ioMyDoubles, myDoubles.data() +
                                           doubleVOffset); // Base class Engine
                                                           // own the Write<T>
                                                           // that will call
                                                           // overloaded Write
                                                           // from Derived
        HDF5Writer->Write(ioMyInts,
                          myInts.data() + (intOffsetDim1 * intDim2 * rank));
        HDF5Writer->Write(ioMyCFloats, myCFloats.data());
        HDF5Writer->Write(ioMyCDoubles, myCDoubles.data());
        HDF5Writer->Close();
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

    return 0;
}
