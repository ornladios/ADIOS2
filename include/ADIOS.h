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
#include <unordered_map>
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
    void DeclareGroup( const std::string groupName );

    /**
     * Creates a new Variable, if debugMode = true, program will throw an exception, else it will overwrite current variable with the same name
     * @param groupName corresponding variable group
     * @param variableName corresponding variable name
     * @param type variable type
     * @param dimensionsCSV comma separated (no space) dimensions "Nx,Ny,Nz" defined by other variables
     * @param globalDimensionsCSV comma separated (no space) global dimensions "gNx,gNy,gNz" defined by other variables
     * @param globalOffsetsCSV comma separated (no space) global offsets "oNx,oNy,oNz" defined by other variables
     */
    void DefineVariable( const std::string groupName, const std::string variableName, const std::string type,
                         const std::string dimensionsCSV = "", const std::string globalDimensionsCSV = "",
                         const std::string globalOffsetsCSV = ""  );
    /**
     * Sets a transform method to a variable, to be applied when writing
     * @param groupName corresponding variable group
     * @param variableName corresponding variable name
     * @param transform method to be applied for this variable
     */
    void AddTransform( const std::string groupName, const std::string variableName, const std::string transform );


    /**
     * @brief Creates a new Attribute.
     * Debug Mode: program will throw an exception if attribute exists (attributeName),
     * else: overwrites current attribute using the same name
     * @param groupName corresponding variable group
     * @param attributeName corresponding attribute name
     * @param type string or number
     * @param value string contents of the attribute (e.g. "Communication value", "1" )
     */
    void DefineAttribute( const std::string groupName, const std::string attributeName,
                          const std::string type, const std::string value );


    /**
     * Declares a new method
     * @param name
     * @param type
     */
    void DeclareMethod( const std::string methodName, const std::string type = "SingleBP" );

    /**
     * Add a capsule type to method name defined from DeclareMethod
     * @param methodName unique method name
     * @param args variadic parameters with format parameter=value
     */
    template< class ...Args>
    void AddCapsule( const std::string methodName, const Args... args )
    {
        auto itMethod = m_Methods.find( methodName );
        if( m_DebugMode == true )
            CheckMethod( itMethod, methodName, " from call to AddBuffer\n" );

        itMethod->second.AddCapsule( args... );
    }

    /**
     * Add a transport type to method name defined from DeclareMethod
     * @param methodName unique method name
     * @param args variadic parameters with format parameter=value
     */
    template< class ...Args>
    void AddTransport( const std::string methodName, const Args... args )
    {
        auto itMethod = m_Methods.find( methodName );
        if( m_DebugMode == true )
            CheckMethod( itMethod, methodName, " from call to AddTransport\n" );

        itMethod->second.AddTransport( args... );
    }

    /**
     * @brief Open to Write, Read. Creates a new engine from previously defined method
     * @param streamName unique stream or file name
     * @param accessMode "w" or "write", "r" or "read", "a" or "append"
     * @param mpiComm option to modify communicator from ADIOS class constructor
     * @param method looks for corresponding Method object in ADIOS to initialize the engine
     * @param cores optional parameter for threaded operations
     * @return handler to created engine
     */
    unsigned int Open( const std::string streamName, const std::string accessMode, MPI_Comm mpiComm,
                             const std::string methodName, const unsigned int cores = 1 );


    /**
     * @brief Open to Write, Read. Creates a new engine from previously defined method. Reuses MPI communicator from ADIOS class constructor.
     * @param streamName unique stream or file name
     * @param accessMode "w" or "write", "r" or "read", "a" or "append"
     * @param methodName looks for corresponding Method object in ADIOS to initialize the engine
     * @param cores optional parameter for threaded operations
     * @return handler to created engine
     */
    unsigned int Open( const std::string streamName, const std::string accessMode, const std::string methodName,
                             const unsigned int cores = 1 );


    /**
     * Sets the maximum buffer size of a stream
     * @param streamName
     * @param maxBufferSize
     */
    void SetMaxBufferSize( const unsigned int engineHandler, const size_t maxBufferSize );

    /**
     * Sets a default group to be associated with a stream before writing variables with Write function.
     * @param streamName unique name
     * @param groupName default group from which variables will be used
     */
    void SetDefaultGroup( const unsigned int handler, const std::string groupName );


    template<class T>
    void Write( const unsigned int handler, const std::string groupName, const std::string variableName, const T* values )
    {
        auto itEngine = m_Engines.find( handler );
        auto itGroup = m_Groups.find( groupName );

        if( m_DebugMode == true )
        {
            CheckEngine( itEngine, handler, " from call to Write variable " + variableName );
            CheckGroup( itGroup, groupName, " from call to Write variable " + variableName );
        }

        itEngine->second->Write( itGroup->second, variableName, values );
    }


    /**
     * Write version using default group, set with Function SetGroup.
     * Submits a data element values for writing and associates it with the given variableName
     * @param streamName stream or file to be written to
     * @param variableName name of existing scalar or vector variable in the XML file or created with CreateVariable
     * @param values pointer to the variable values passed from the user application, use dynamic_cast to check that pointer is of the same value type
     * @param cores optional parameter for threaded version
     */
    template<class T>
    void Write( const unsigned int handler, const std::string variableName, const T* values )
    {
        auto itEngine = m_Engines.find( handler );

        if( m_DebugMode == true )
            CheckEngine( itEngine, handler, " from call to Write variable " + variableName );

        itEngine->second->Write( variableName, values );
    }


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



private:

    std::string m_XMLConfigFile; ///< XML File to be read containing configuration information
    std::string m_HostLanguage = "C++"; ///< Supported languages: C, C++, Fortran, Python, etc.
    bool m_DebugMode = false; ///< if true will do more checks, exceptions, warnings, expect slower code

    //GROUP
    /**
     * @brief List of groups defined from either ADIOS XML configuration file or the CreateGroup function.
     * <pre>
     *     Key: std::string unique group name
     *     Value: Group class
     * </pre>
     */
    std::map< std::string, Group > m_Groups;

    //METHODS -> Engine Metadata
    /**
     * @brief List of Methods defined from either ADIOS XML configuration file or the CreateMethod function.
     * <pre>
     *     Key: std::string unique method name
     *     Value: Method class
     * </pre>
     */
    std::map< std::string, Method > m_Methods;


    //ENGINE
    /**
     * @brief List of Engines, ADIOS Open adds to this container.
     * <pre>
     *     Key: Engine handle give by a counter
     *     Value: Engine derived class
     * </pre>
     */
    std::unordered_map< unsigned int, std::shared_ptr<Engine> > m_Engines;
    std::set< std::string > m_EngineNames; ///< set used to check Engine name uniqueness in debug mode
    int m_EngineCounter = -1; ///< used to set the unsigned int key in m_Capsules, helpful is a capsule is removed


    //TRANSFORMS
    std::vector< std::shared_ptr<Transform> > m_Transforms; ///< transforms associated with ADIOS run


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


    /**
     * @brief Checks for engine existence in m_Engines, if failed throws std::invalid_argument exception
     * @param itEngine from Open
     * @param hint adds information to thrown exception
     */
    void CheckEngine( std::unordered_map< unsigned int, std::shared_ptr<Engine> >::const_iterator itEngine,
                      const unsigned int handle, const std::string hint ) const;

};



} //end namespace




#endif /* ADIOS_H_ */
