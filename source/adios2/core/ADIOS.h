/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS.h : ADIOS library starting point, factory class for IO and
 * (polymorphic) Engines
 *  Created on: Oct 3, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_ADIOS_H_
#define ADIOS2_CORE_ADIOS_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <map>
#include <memory> //std::shared_ptr
#include <string>
#include <vector>
/// \endcond

#include "adios2/ADIOSConfig.h"
#include "adios2/ADIOSMPICommOnly.h"
#include "adios2/core/IO.h"
#include "adios2/core/Transform.h"

namespace adios2
{

/** @brief Point of entry class for an application.
 *         Serves as factory of IO class objects and Transforms */
class ADIOS
{
public:
    /** Passed from parallel constructor, MPI_Comm is a pointer itself. */
    MPI_Comm m_MPIComm;

    /** Changed by language bindings */
    std::string m_HostLanguage = "C++";

    /**
     * @brief Constructor for MPI applications WITH a XML config file
     * @param configFile XML format (maybe support different formats in the
     * future?)
     * @param mpiComm MPI communicator from application
     * @param debugMode true: extra exception checks (recommended)
     */
    ADIOS(const std::string configFile, MPI_Comm mpiComm,
          const bool debugMode = true);

    /**
     * @brief Constructor for non-MPI applications WITH a XML config file
     * @param configFile XML format (maybe support different formats in the
     * future?)
     * @param debugMode true: extra exception checks (recommended)
     */
    ADIOS(const std::string configFile, const bool debugMode = true);

    /**
     * @brief Constructor for MPI apps WITHOUT a XML config file
     * @param mpiComm MPI communicator from application
     * @param debugMode true: extra exception checks (recommended)
     */
    ADIOS(MPI_Comm mpiComm, const bool debugMode = true);

    /**
     *  @brief ADIOS no-MPI default empty constructor
     *  @param debugMode true: extra exception checks (recommended)
     */
    ADIOS(const bool debugMode = true);

    /**
     * Delete copy constructor explicitly. Objects shouldn't be allowed to be
     * redefined. Use smart pointers if this is absolutely necessary.
     * @param adios reference to another adios object
     */
    ADIOS(const ADIOS &adios) = delete;

    ~ADIOS() = default;

    /**
     * Declares a new IO class object. If IO object is defined in the user
     * config file, by name, it will be already created during the processing
     * the config file. So this function returns a reference to that object.
     * Otherwise it will create and return a new IO object with default
     * settings.
     * Use function InConfigFile() to distinguish between the two cases.
     * @param ioName must be unique
     * @return reference to existing (or newly created) method inside ADIOS
     */
    IO &DeclareIO(const std::string ioName);

    /**
     * Retrieve an already defined IO object. Make sure IO was previously
     * created with DeclareIO. Otherwise, throws an exception in debug mode.
     * @return reference to existing IO object inside ADIOS
     */
    IO &GetIO(const std::string name);

    Transform &GetTransform(const std::string transform);

private:
    /** XML File to be read containing configuration information */
    const std::string m_ConfigFile;

    /** if true will do more checks, exceptions, warnings, expect slower code */
    const bool m_DebugMode = true;

    /** transforms associated with ADIOS run */
    std::map<std::string, std::shared_ptr<Transform>> m_Transforms;

    /**
     * @brief List of IO class objects defined from either ADIOS
     * configuration file (XML) or the DeclareIO function explicitly.
     * Using map (binary tree) to preserve references returned by DeclareIO.
     * <pre>
     *     Key: unique method name
     *     Value: IO class object
     * </pre>
     */
    std::map<std::string, IO> m_IOs;

    /** throws exception if m_MPIComm = MPI_COMM_NULL */
    void CheckMPI() const;
};

} // end namespace adios

#endif /* ADIOS2_ADIOS_H_ */
