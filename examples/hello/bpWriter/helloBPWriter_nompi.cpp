/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloADIOSNoXML_OOP.cpp
 *
 *  Created on: Jan 9, 2017
 *      Author: wfg
 */

#include <ios>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "ADIOS_CPP.h"

int main(int /*argc*/, char ** /*argv*/)
{
  const bool adiosDebug = true;
  adios::ADIOS adios(adios::Verbose::WARN, adiosDebug);

  // Application variable
  std::vector<double> myDoubles = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  const std::size_t Nx = myDoubles.size();

  const std::size_t rows = 3;
  const std::size_t columns = 3;
  std::vector<float> myMatrix = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  std::vector<float> myMatrix2 = {-1, -2, -3, -4, -5, -6, -7, -8, -9};

  try
  {
    // Define variable and local size
    adios::Variable<double> &ioMyDoubles =
        adios.DefineVariable<double>("myDoubles", adios::Dims{Nx});
    adios::Variable<float> &ioMyMatrix =
        adios.DefineVariable<float>("myMatrix", adios::Dims{rows, columns});
    adios::Variable<float> &ioMyMatrix2 =
        adios.DefineVariable<float>("myMatrix2", adios::Dims{rows, columns});
    adios::Variable<float> &ioMyMatrix3 =
        adios.DefineVariable<float>("myMatrix3", adios::Dims{rows, columns});

    // Define method for engine creation, it is basically straight-forward
    // parameters
    adios::Method &bpWriterSettings =
        adios.DeclareMethod("SinglePOSIXFile"); // default method type is Writer
    bpWriterSettings.SetParameters("profile_units=mus");
    bpWriterSettings.AddTransport("File", "have_metadata_file=yes",
                                  "profile_units=mus");

    // Create engine smart pointer due to polymorphism,
    // Open returns a smart pointer to Engine containing the Derived class
    // Writer
    auto bpFileWriter = adios.Open("myDoubles_nompi.bp", "w", bpWriterSettings);

    if (bpFileWriter == nullptr)
    {
      throw std::ios_base::failure("ERROR: couldn't create bpWriter at Open\n");
    }

    bpFileWriter->Write<double>(
        ioMyDoubles, myDoubles.data()); // Base class Engine own the Write<T>
                                        // that will call overloaded Write from
                                        // Derived
    bpFileWriter->Write<float>(ioMyMatrix, myMatrix.data());   // 2d Example
    bpFileWriter->Write<float>(ioMyMatrix2, myMatrix2.data()); // 2d Example
    bpFileWriter->Write<float>(ioMyMatrix3, myMatrix2.data()); // 2d Example
    bpFileWriter->Close();
    //
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
