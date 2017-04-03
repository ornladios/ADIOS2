/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * timeBPWriter.cpp  example for time aggregation
 *
 *  Created on: Feb 16, 2017
 *      Author: wfg
 */

#include <iostream>
#include <vector>

#include "ADIOS_CPP.h"

int main(int /*argc*/, char ** /*argv*/)
{
  const bool adiosDebug = true;
  adios::ADIOS adios(adios::Verbose::ERROR, adiosDebug);

  // Application variable
  std::vector<double> myDoubles = {10, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  const std::size_t Nx = myDoubles.size();

  const std::size_t rows = 3;
  const std::size_t columns = 3;

  std::vector<float> myMatrix;
  myMatrix.reserve(rows * columns);
  myMatrix.push_back(1);
  myMatrix.push_back(2), myMatrix.push_back(3);
  myMatrix.push_back(4);
  myMatrix.push_back(5), myMatrix.push_back(6);
  myMatrix.push_back(7);
  myMatrix.push_back(8), myMatrix.push_back(8);

  std::vector<float> myMatrix2 = {-1, -2, -3, -4, -5, -6, -7, -8, -9};

  try
  {
    // Define variable and local size
    adios::Variable<double> &ioMyDoubles =
        adios.DefineVariable<double>("myDoubles", {Nx});
    adios::Variable<float> &ioMyMatrix =
        adios.DefineVariable<float>("myMatrix", {rows, columns});
    adios::Variable<float> &ioMyMatrix2 =
        adios.DefineVariable<float>("myMatrix2", {rows, columns});

    // Define method for engine creation, it is basically straight-forward
    // parameters
    adios::Method &bpWriterSettings =
        adios.DeclareMethod("SingleFile"); // default method type is BPWriter
    bpWriterSettings.SetParameters("profile_units=mus");
    bpWriterSettings.AddTransport(
        "File", "profile_units=mus",
        "have_metadata_file=no"); // uses default POSIX library

    // Create object directly rather than using polymorphism with ADIOS.Open
    adios::BPFileWriter bpWriter(adios, "time_nompi.bp", "w", adios.m_MPIComm,
                                 bpWriterSettings);

    for (unsigned int t = 0; t < 3; ++t)
    {
      myDoubles[0] = t; // t * -1;
      myMatrix[0] = t;
      myMatrix2[0] = t;

      bpWriter.Write(ioMyDoubles, myDoubles.data()); // Base class Engine own
                                                     // the Write<T> that will
                                                     // call overloaded Write
                                                     // from Derived
      bpWriter.Write(ioMyMatrix, myMatrix.data());
      bpWriter.Write(ioMyMatrix2, myMatrix2.data());
      bpWriter.Advance();
    }

    bpWriter.Close();
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
