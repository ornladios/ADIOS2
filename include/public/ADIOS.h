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
#include <memory> //shared_ptr
#include <ostream>
/// \endcond

#ifdef HAVE_MPI
  #include <mpi.h>
#else
  #include "public/mpidummy.h"
#endif

#include "core/CGroup.h"
#include "core/CCapsule.h"
#include "public/SSupport.h"
#include "functions/ADIOSTemplates.h"


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
     * @brief Open to Write, Read or Append to a stream
     * @param streamName associated file or stream
     * @param accessMode "w": write, "a": append, need more info on this
     */
    template< class... Args >
    void Open( const std::string streamName, const std::string accessMode, const std::string transport, Args... args )
    {
        if( m_DebugMode == true )
        {
            if( m_Capsules.count( streamName ) == 1 ) //it exists
                throw std::invalid_argument( "ERROR: trying to open existing stream (or file) " + streamName + ".\n" );
        }

        std::vector<std::string> arguments = { args... };
        m_Capsules.emplace( streamName, CCapsule( m_MPIComm, m_DebugMode, streamName, accessMode, transport, arguments ) );
    }

    /**
     * Sets a new transport method to be associated with a current stream.
     * @param streamName must be created with Open
     * @param transport
     * @return transport index
     */
    template< class... Args >
    int AddTransport( const std::string streamName, const std::string accessMode, const std::string transport, Args... args )
    {
        if( m_DebugMode == true )
        {
            if( m_Capsules.count( streamName ) == 0 ) //it exists
                throw std::invalid_argument( "ERROR: stream (or file) " + streamName + " does not exist in call to AddTransport.\n" );
        }
        std::vector<std::string> arguments = { args... };
        return m_Capsules[streamName].AddTransport( streamName, accessMode, false, transport, arguments );
    }

    /**
     * Sets the maximum buffer size of a stream
     * @param streamName
     * @param maxBufferSize
     */
    void SetMaxBufferSize( const std::string streamName, const size_t maxBufferSize );

    /**
     * Sets a default group to be associated with a stream before writing variables with Write function.
     * @param streamName unique name
     * @param groupName default group from which variables will be used
     */
    void SetCurrentGroup( const std::string streamName, const std::string groupName );


    template<class T>
    void Write( const std::string streamName, const std::string groupName, const std::string variableName, const T* values,
                const int transportIndex = -1 )
    {
        auto itCapsule = m_Capsules.find( streamName );
        auto itGroup = m_Groups.find( groupName );

        if( m_DebugMode == true )
        {
            CheckCapsule( itCapsule, streamName, " from call to Write variable " + variableName );
            CheckGroup( itGroup, groupName, " from call to Write variable " + variableName );
            if( transportIndex < -1 )
                throw std::invalid_argument( "ERROR: transport index " + std::to_string( transportIndex ) +
                                             " must be >= -1, in call to Write\n" );
        }

        WriteHelper( itCapsule->second, itGroup->second, variableName, values, transportIndex, m_DebugMode );
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
    void Write( const std::string streamName, const std::string variableName, const T* values,
                const int transportIndex = -1 )
    {
        auto itCapsule = m_Capsules.find( streamName );
        if( m_DebugMode == true )
            CheckCapsule( itCapsule, streamName, " from call to Write variable " + variableName );

        const std::string groupName( itCapsule->second.m_CurrentGroup );
        auto itGroup = m_Groups.find( groupName );
        if( m_DebugMode == true )
            CheckGroup( itGroup, groupName, " from call to Write variable " + variableName );

        WriteHelper( itCapsule->second, itGroup->second, variableName, values, transportIndex, m_DebugMode );
    }

    /**
     * Close a particular stream and the corresponding transport
     * @param streamName stream to be closed with all corresponding transports
     * @param transportIndex identifier to a particular transport, if == -1 Closes all transports
     */
    void Close( const std::string streamName, const int transportIndex = -1 );

    /**
     * @brief Dumps groups information to a file stream or standard output.
     * Note that either the user closes this fileStream or it's closed at the end.
     * @param logStream either std::cout standard output, or a std::ofstream file
     */
    void MonitorGroups( std::ostream& logStream ) const;

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
    void SetTransform( const std::string groupName, const std::string variableName, const std::string transform );


    /**
     * Creates a new Variable, if debugMode = true, program will throw an exception, else it will overwirte current variable with the same name
     * @param groupName corresponding variable group
     * @param attributeName corresponding attribute name
     * @param type string or number
     * @param value string contents of the attribute (e.g. "Communication value" )
     */
    void DefineAttribute( const std::string groupName, const std::string attributeName,
                          const std::string type, const std::string value );


private:

    std::string m_XMLConfigFile; ///< XML File to be read containing configuration information
    std::string m_HostLanguage = "C++"; ///< Supported languages: C, C++, Fortran, Python, etc.
    bool m_DebugMode = false; ///< if true will do more checks, exceptions, warnings, expect slower code

    /**
     * @brief List of groups defined from either ADIOS XML configuration file or the CreateGroup function.
     * <pre>
     *     Key: std::string unique group name
     *     Value: SGroup struct in SGroup.h
     * </pre>
     */
    std::map< std::string, CGroup > m_Groups;

    /**
     * @brief List of Capsules, each defined from the Open function.
     * <pre>
     *     Key: std::string unique stream/file/capsule name given by ADIOS Open
     *     Value: CCapsule object
     * </pre>
     */
    std::map< std::string, CCapsule > m_Capsules;


    std::vector< std::shared_ptr<CTransform> > m_Transforms; ///< transforms associated with ADIOS run

    /**
     * @brief Checks for group existence in m_Groups, if failed throws std::invalid_argument exception
     * @param itGroup m_Group iterator, usually from find function
     * @param groupName unique name, passed for thrown exception only
     * @param hint adds information to thrown exception
     */
    void CheckGroup( std::map< std::string, CGroup >::const_iterator itGroup,
                     const std::string groupName, const std::string hint ) const;

    /**
     * @brief Checks for capsule existence in m_Groups, if failed throws std::invalid_argument exception
     * @param itCapsule m_Capsule iterator, usually from find function
     * @param streamName unique name, passed for thrown exception only
     * @param hint adds information to thrown exception
     */
    void CheckCapsule( std::map< std::string, CCapsule >::const_iterator itCapsule,
                       const std::string streamName, const std::string hint ) const;
};



} //end namespace




#endif /* ADIOS_H_ */
