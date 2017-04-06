/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloCompound.cpp
 *
 *  Created on: Feb 20, 2017
 *      Author: wfg
 */

#include <cstddef> // offsetof
#include <cstdio>
#include <iostream>
#include <vector>

#include "ADIOS_CPP.h"

struct Particle
{
    char Type[10];      ///< alpha, beta, gamma, etc.
    double Position[3]; ///< x, y, z
    double Velocity[3]; ///< Vx, Vy, Vz
};

int main(int argc, char *argv[])
{
    const bool adiosDebug = true;
    adios::ADIOS adios(adiosDebug);

    // Application variable
    std::vector<double> myDoubles = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    const std::size_t Nx = myDoubles.size();

    Particle myParticle;
    sprintf(myParticle.Type, "%s", "photon");
    myParticle.Position[0] = 0;
    myParticle.Position[1] = 1;
    myParticle.Position[2] = 2;

    myParticle.Velocity[0] = 10;
    myParticle.Velocity[1] = 11;
    myParticle.Velocity[2] = 12;

    try
    {
        // Define variable and local size
        adios::Variable<double> &ioMyDoubles =
            adios.DefineVariable<double>("myDoubles", adios::Dims{Nx});

        adios::VariableCompound &ioMyParticle =
            adios.DefineVariableCompound<Particle>("myParticle");
        ioMyParticle.InsertMember<char>("Type", offsetof(Particle, Type));
        ioMyParticle.InsertMember<double>("Position",
                                          offsetof(Particle, Position));
        ioMyParticle.InsertMember<double>("Velocity",
                                          offsetof(Particle, Velocity));

        // Define method for engine creation, it is basically straight-forward
        // parameters
        adios::Method &bpWriterSettings = adios.DeclareMethod(
            "SingleFile"); // default method type is BPWriter
        bpWriterSettings.AddTransport(
            "File", "have_metadata_file=yes"); // uses default POSIX library

        // Create engine smart pointer due to polymorphism,
        // Open returns a smart pointer to Engine containing the Derived class
        // Writer
        auto bpWriter = adios.Open("myDoubles.bp", "w", bpWriterSettings);

        if (bpWriter == nullptr)
            throw std::ios_base::failure(
                "ERROR: couldn't create bpWriter at Open\n");

        bpWriter->Write(ioMyDoubles,
                        myDoubles.data()); // Base class Engine own the
                                           // Write<T> that will call
                                           // overloaded Write from
                                           // Derived
        bpWriter->Close();
    }
    catch (std::invalid_argument &e)
    {
        std::cout << "Invalid argument exception, STOPPING PROGRAM\n";
        std::cout << e.what() << "\n";
    }
    catch (std::ios_base::failure &e)
    {
        std::cout << "System exception, STOPPING PROGRAM\n";
        std::cout << e.what() << "\n";
    }
    catch (std::exception &e)
    {
        std::cout << "Exception, STOPPING PROGRAM\n";
        std::cout << e.what() << "\n";
    }

    return 0;
}
