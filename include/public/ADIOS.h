/*
 * ADIOS.h
 *
 *  Created on: Oct 3, 2016
 *      Author: wfg
 */

#ifndef ADIOS_H_
#define ADIOS_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
#include <memory>
#include <ostream>
/// \endcond

#ifdef HAVE_MPI
  #include <mpi.h>
#else
  #include "public/mpidummy.h"
#endif

#include "core/CGroup.h"
#include "core/CTransport.h"
#include "public/SSupport.h"


namespace adios
{
/**
 * @brief Unique class interface between user application and ADIOS library
 */
class ADIOS
{

public: // PUBLIC Constructors and Functions define the User Interface with ADIOS

    /**
     * @brief ADIOS empty constructor. Used for non XML config file API calls.
     */
    ADIOS( const bool debugMode = false );

    /**
     * @brief Serial constructor for XML config file
     * @param xmlConfigFile passed to m_XMLConfigFile
     */
    ADIOS( const std::string xmlConfigFile, const bool debugMode = false );

    /**
     * @brief Parallel constructor for XML config file and MPI
     * @param xmlConfigFile passed to m_XMLConfigFile
     * @param mpiComm MPI communicator ...const to be discussed
     */
    ADIOS( const std::string xmlConfigFile, const MPI_Comm mpiComm, const bool debugMode = false );

    /**
     * @brief Parallel MPI communicator without XML config file
     * @param mpiComm MPI communicator passed to m_MPIComm
     */
    ADIOS( const MPI_Comm mpiComm, const bool debugMode = false );

    ~ADIOS( ); ///< virtual destructor overriden by children's own destructors

    /**
     * @brief Open or Append to an output file
     * @param groupName should match an existing group from XML file or created through CreateGroup
     * @param fileName associated file or stream
     * @param accessMode "w": write, "a": append, need more info on this
     */
    void Open( const std::string groupName, const std::string fileName, const std::string accessMode = "w" );

    /**
     * @brief Get the total sum of payload and overhead, which includes name, data type, dimensions and other metadata
     * @param groupName
     * @return group size in MB
     */
    unsigned long int GetGroupSize( const std::string groupName ) const;

    /**
     * Submits a data element values for writing and associates it with the given variableName
     * @param groupName name of group that owns the variable
     * @param variableName name of existing scalar or vector variable in the XML file or created with CreateVariable
     * @param values pointer to the variable values passed from the user application, use dynamic_cast to check that pointer is of the same value type
     */
    template<class T>
    void Write( const std::string groupName, const std::string variableName, const T* values );

    /**
     * @brief Dumps groups information to a file stream or standard output.
     * Note that either the user closes this fileStream or it's closed at the end.
     * @param logStream either std::cout standard output, or a std::ofstream file
     */
    void MonitorGroups( std::ostream& logStream ) const;

    /**
     * Close a particular group
     * @param groupName
     */
    void Close( const std::string groupName );



private:

    std::string m_XMLConfigFile; ///< XML File to be read containing configuration information
    MPI_Comm m_MPIComm = nullptr; ///< only used as reference to MPI communicator passed from parallel constructor, MPI_Comm is a pointer itself
    std::string m_HostLanguage; ///< Supported languages: C, C++, Fortran
    const bool m_DebugMode = false; ///< if true will do more checks, exceptions, warnings, expect slower code

    /**
     * @brief List of groups defined from either ADIOS XML configuration file or the CreateGroup function.
     * <pre>
     *     Key: std::string unique group name
     *     Value: SGroup struct in SGroup.h
     * </pre>
     */
    std::map< std::string, CGroup > m_Groups;

    std::map< std::string, CTransform > m_Transforms;

    /**
     * @brief Maximum buffer size in ADIOS write() operation. From buffer max - size - MB in XML file
     * Note, that if there are two ADIOS outputs going on at the same time,
     * ADIOS will allocate two separate buffers each with the specified maximum limit.
     * Default = 0 means there is no limit
     */
    unsigned int MaxBufferSizeInMB = 0;

    /**
     * Checks for group existence in m_Groups
     * @param groupName to be checked
     */
    void CheckGroup( const std::string groupName );
};



} //end namespace




#endif /* ADIOS_H_ */
