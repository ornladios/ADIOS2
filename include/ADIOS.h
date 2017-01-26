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
#include <vector>
#include <memory> //shared_ptr
#include <ostream>
#include <set>
#include <map>
/// \endcond

#ifdef HAVE_MPI
  #include <mpi.h>
#else
  #include "mpidummy.h"
#endif

#include "core/Engine.h"
#include "core/Group.h"
#include "core/Method.h"
#include "core/Support.h"


namespace adios
{
/**
 * @brief Unique class interface between user application and ADIOS library
 */
class ADIOS
{

public: // PUBLIC Constructors and Functions define the User Interface with ADIOS

    #ifdef HAVE_MPI
    MPI_Comm m_MPIComm = NULL; ///< only used as reference to MPI communicator passed from parallel constructor, MPI_Comm is a pointer itself. Public as called from C
    #else
    MPI_Comm m_MPIComm = 0; ///< only used as reference to MPI communicator passed from parallel constructor, MPI_Comm is a pointer itself. Public as called from C
    #endif

    int m_RankMPI = 0; ///< current MPI rank process
    int m_SizeMPI = 1; ///< current MPI processes size

    /**
     * @brief ADIOS empty constructor. Used for non XML config file API calls.
     */
    ADIOS( );


    /**
     * @brief Serial constructor for XML config file
     * @param xmlConfigFile passed to m_XMLConfigFile
     * @param debugMode true: on throws exceptions and do additional checks, false: off (faster, but unsafe)
     */
    ADIOS( const std::string xmlConfigFile, const bool debugMode = false );


    /**
     * @brief Parallel constructor for XML config file and MPI
     * @param xmlConfigFile passed to m_XMLConfigFile
     * @param mpiComm MPI communicator ...const to be discussed
     * @param debugMode true: on, false: off (faster, but unsafe)
     */
    ADIOS( const std::string xmlConfigFile, const MPI_Comm mpiComm, const bool debugMode = false );


    /**
     * @brief Parallel MPI communicator without XML config file
     * @param mpiComm MPI communicator passed to m_MPIComm*
     * @param debugMode true: on, false: off (faster)
     */
    ADIOS( const MPI_Comm mpiComm, const bool debugMode = false );


    ~ADIOS( ); ///< empty, using STL containers for memory management


    /**
     * Creates an empty group
     * @param groupName
     */
    Group& DeclareGroup( const std::string groupName );

    /**
     * Declares a new method
     * @param name must be unique
     * @param type supported type : "Writer" (default), "DataMan"...future: "Sirius"
     */
    Method& DeclareMethod( const std::string methodName, const std::string type = "Writer" );


    /**
     * @brief Open to Write, Read. Creates a new engine from previously defined method
     * @param streamName unique stream or file name
     * @param accessMode "w" or "write", "r" or "read", "a" or "append"
     * @param mpiComm option to modify communicator from ADIOS class constructor
     * @param method looks for corresponding Method object in ADIOS to initialize the engine
     * @param cores optional parameter for threaded operations
     * @return Derived class of base Engine depending on Method parameters, shared_ptr for potential flexibility
     */
    std::shared_ptr<Engine> Open( const std::string streamName, const std::string accessMode, MPI_Comm mpiComm,
                                  const Method& method, const unsigned int cores = 1 );


    /**
     * @brief Open to Write, Read. Creates a new engine from previously defined method.
     * Reuses MPI communicator from ADIOS constructor.
     * @param streamName unique stream or file name
     * @param accessMode "w" or "write", "r" or "read", "a" or "append"
     * @param method contains engine parameters
     * @param cores optional parameter for threaded operations
     * @return Derived class of base Engine depending on Method parameters, shared_ptr for potential flexibility
     */
    std::shared_ptr<Engine> Open( const std::string streamName, const std::string accessMode, const Method& method,
                                  const unsigned int cores = 1 );


    /**
     * Version required by the XML config file implementation, searches method inside ADIOS through a unique name
     * @param streamName unique stream or file name
     * @param accessMode "w" or "write", "r" or "read", "a" or "append"
     * @param mpiComm mpi Communicator
     * @param methodName used to search method object inside ADIOS object
     * @param cores optional parameter for threaded operations
     * @return Derived class of base Engine depending on Method parameters, shared_ptr for potential flexibility
     */
    std::shared_ptr<Engine> Open( const std::string streamName, const std::string accessMode, MPI_Comm mpiComm,
                                  const std::string methodName, const unsigned int cores = 1 );

    /**
     * Version required by the XML config file implementation, searches method inside ADIOS through a unique name.
     * Reuses ADIOS MPI Communicator from constructor.
     * @param streamName unique stream or file name
     * @param accessMode "w" or "write", "r" or "read", "a" or "append"
     * @param methodName used to search method object inside ADIOS object
     * @param cores optional parameter for threaded operations
     * @return Derived class of base Engine depending on Method parameters, shared_ptr for potential flexibility
     */
    std::shared_ptr<Engine> Open( const std::string streamName, const std::string accessMode,
                                  const std::string methodName, const unsigned int cores = 1 );

    /**
     * Close a particular stream and the corresponding transport
     * @param streamName stream to be closed with all corresponding transports
     * @param transportIndex identifier to a particular transport, if == -1 Closes all transports
     */
    void Close( const unsigned int handler, const int transportIndex = -1 );


    /**
     * @brief Dumps groups information to a file stream or standard output.
     * Note that either the user closes this fileStream or it's closed at the end.
     * @param logStream either std::cout standard output, or a std::ofstream file
     */
    void MonitorGroups( std::ostream& logStream ) const;



private: //no const to allow default empty and copy constructors

    std::string m_XMLConfigFile; ///< XML File to be read containing configuration information
    std::string m_HostLanguage = "C++"; ///< Supported languages: C, C++, Fortran, Python, etc.
    bool m_DebugMode = false; ///< if true will do more checks, exceptions, warnings, expect slower code

    //GROUP
    /**
     * @brief List of groups defined from either ADIOS XML configuration file or the DeclareGroup function.
     * <pre>
     *     Key: std::string unique group name
     *     Value: Group class
     * </pre>
     */
    std::map< std::string, Group > m_Groups;
    std::vector< std::shared_ptr<Transform> > m_Transforms; ///< transforms associated with ADIOS run


    /**
     * @brief List of Methods (engine metadata) defined from either ADIOS XML configuration file or the DeclareMethod function.
     * <pre>
     *     Key: std::string unique method name
     *     Value: Method class
     * </pre>
     */
    std::map< std::string, Method > m_Methods;
    std::set< std::string > m_EngineNames; ///< set used to check Engine name uniqueness in debug mode

    /**
     * @brief Checks for group existence in m_Groups, if failed throws std::invalid_argument exception
     * @param itGroup m_Groups iterator, usually from find function
     * @param groupName unique name, passed for thrown exception only
     * @param hint adds information to thrown exception
     */
    void CheckGroup( std::map< std::string, Group >::const_iterator itGroup,
                     const std::string groupName, const std::string hint ) const;


    /**
     * @brief Checks for method existence in m_Methods, if failed throws std::invalid_argument exception
     * @param itMethod m_Methods iterator, usually from find function
     * @param methodName unique name, passed for thrown exception only
     * @param hint adds information to thrown exception
     */
    void CheckMethod( std::map< std::string, Method >::const_iterator itMethod,
                      const std::string methodName, const std::string hint ) const;

};



} //end namespace




#endif /* ADIOS_H_ */
