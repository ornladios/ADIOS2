/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * cxx98ADIOS.h : C++98 bindings based on C API, eliminates all C++11
 * features in public headers
 *
 *  Created on: Apr 4, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_CXX98_CXX98_CXX98ADIOS_H_
#define ADIOS2_BINDINGS_CXX98_CXX98_CXX98ADIOS_H_

#include <string>

#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#endif

#include "cxx98IO.h"

#include "adios2/ADIOSConfig.h"

struct adios2_adios;

namespace adios2
{
namespace cxx98
{

class ADIOS
{

public:
#ifdef ADIOS2_HAVE_MPI

    /**
     * adios2 library starting point. Creates an ADIOS object allowing a
     * runtime
     * config file.
     * @param configFile runtime config file
     * @param mpiComm defines domain scope from application
     * @param debugMode true: extra user-input debugging information, false:
     * run
     * without checking user-input (stable workflows)
     * @exception std::invalid_argument in debugMode = true if user input is
     * incorrect
     */
    ADIOS(const std::string &configFile, MPI_Comm mpiComm,
          const bool debugMode = true);

    /**
     * adios2 library starting point. Creates an ADIOS object.
     * @param mpiComm defines domain scope from application
     * @param debugMode true: extra user-input debugging information, false: run
     * without checking user-input (stable workflows)
     * @exception std::invalid_argument in debugMode = true if user input is
     * incorrect
     */
    ADIOS(MPI_Comm mpiComm, const bool debugMode = true);

    /**
     * adios2 library starting point. Creates an ADIOS object allowing a
     * runtime config file and using MPI_COMM_SELF
     * @param configFile runtime config file
     * @param debugMode true: extra user-input debugging information, false: run
     * without checking user-input (stable workflows)
     * @exception std::invalid_argument in debugMode = true if user input is
     * incorrect
     */
    ADIOS(const std::string &configFile, const bool debugMode = true);

    /**
     * adios2 library starting point. Creates an ADIOS object using
     * MPI_COMM_SELF
     * @param debugMode true: extra user-input debugging information, false: run
     * without checking user-input (stable workflows)
     * @exception std::invalid_argument in debugMode = true if user input is
     * incorrect
     */
    ADIOS(const bool debugMode = true);
#else
    /**
     * adios2 NON-MPI library starting point. Creates an ADIOS object allowing a
     * runtime config file.
     * @param configFile runtime config file
     * @param debugMode true: extra user-input debugging information, false: run
     * without checking user-input (stable workflows)
     * @exception std::invalid_argument in debugMode = true if user input is
     * incorrect
     */
    ADIOS(const std::string &configFile, const bool debugMode = true);

    /**
     * adios2 NON-MPI library starting point. Creates an ADIOS object
     * @param debugMode true: extra user-input debugging information, false: run
     * without checking user-input (stable workflows)
     * @exception std::invalid_argument in debugMode = true if user input is
     * incorrect
     */
    ADIOS(const bool debugMode = true);
#endif

    ~ADIOS();

    /** true: valid ADIOS object, false: invalid */
    operator bool() const;

    /**
     * Declares a new IO class object and returns a reference to that
     * object.
     * @param ioName must be unique
     * @return reference to newly created IO object inside current ADIOS
     * object
     * @exception std::invalid_argument if IO with unique name is already
     * declared, in debug mode only
     */
    IO DeclareIO(const std::string &name);

    /**
     * Retrieve a reference to an existing IO object created with DeclareIO.
     * @param name of IO to look for
     * @return if IO exists returns a reference to existing IO object inside
     * ADIOS
     * @exception std::invalid_argument if IO was not created with
     * DeclareIO, in debug mode only
     */
    IO AtIO(const std::string &name);

    /**
     * Flushes all engines in all IOs created with the current ADIOS object
     * using DeclareIO and IO.Open.
     * If no IO or engine is created it does nothing.
     * @exception std::runtime_error if any engine Flush fails
     */
    void FlushAll();

private:
    /** pointer to C adios2_adios implementation */
    adios2_adios *m_ADIOS;

    /**
     * Disables copy constructor
     * @param adios
     */
    ADIOS(const ADIOS &adios);
};

} // end namespace cxx03
} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX98_CXX98_CXX98ADIOS_H_ */
