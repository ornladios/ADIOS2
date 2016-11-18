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


    ~ADIOS( ); ///< virtual destructor overriden by children's own destructors


    /**
     * @brief Open to Write, Read or Append to a stream
     * @param groupName should match an existing group from XML file or created through CreateGroup
     * @param streamName associated file or stream
     * @param accessMode "w": write, "a": append, need more info on this
     * @param maxBufferSize used for transport
     */
    void Open( const std::string groupName, const std::string streamName, const std::string accessMode = "w",
               unsigned long long int maxBufferSize );


    /**
     * Submits a data element values for writing and associates it with the given variableName
     * @param groupName name of group that owns the variable
     * @param variableName name of existing scalar or vector variable in the XML file or created with CreateVariable
     * @param values pointer to the variable values passed from the user application, use dynamic_cast to check that pointer is of the same value type
     */
    template<class T>
    void Write( const std::string groupName, const std::string variableName, const T* values, const unsigned int cores = 1 )
    {
        auto itGroup = m_Groups.find( groupName );
        if( m_DebugMode == true )
        {
            CheckGroup( itGroup, groupName, " from call to Write with variable " + variableName );

            if( itGroup->second.m_IsOpen == false )
                throw std::invalid_argument( "ERROR: group " + groupName + " is not open in Write function.\n" );
        }
        WriteVariableValues( itGroup->second, variableName, values, m_Capsule );
    }

    /**
     * Close a particular group, group will be out of scope and destroyed
     * @param groupName group to be closed
     */
    void Close( const std::string groupName );


    /**
     * @brief Dumps groups information to a file stream or standard output.
     * Note that either the user closes this fileStream or it's closed at the end.
     * @param logStream either std::cout standard output, or a std::ofstream file
     */
    void MonitorGroups( std::ostream& logStream ) const;

    /**
     * @brief Create a new group or replace an existing one
     * @param groupName unique name
     */
    void CreateGroup( const std::string groupName );

    /**
     * Creates a new Variable, if debugMode = true, program will throw an exception, else it will overwrite current variable with the same name
     * @param groupName corresponding variable group
     * @param variableName corresponding variable name
     * @param type variable type
     * @param dimensionsCSV comma separated (no space) dimensions "Nx,Ny,Nz" defined by other variables
     * @param transform transformation method applied to current variable
     * @param globalDimensionsCSV comma separated (no space) global dimensions "gNx,gNy,gNz" defined by other variables
     * @param globalOffsetsCSV comma separated (no space) global offsets "oNx,oNy,oNz" defined by other variables
     */
    void CreateVariable( const std::string groupName, const std::string variableName, const std::string type,
                         const std::string dimensionsCSV = "", const std::string transform = "",
                         const std::string globalDimensionsCSV = "", const std::string globalOffsetsCSV = ""  );

    /**
     * Creates a new Variable, if debugMode = true, program will throw an exception, else it will overwirte current variable with the same name
     * @param groupName corresponding variable group
     * @param attributeName corresponding attribute name
     * @param type string or number
     * @param value string contents of the attribute (e.g. "Communication value" )
     * @param globalDimensionsCSV comma separated (no space) global dimensions "gNx,gNy,gNz" defined by other variables
     * @param globalOffsetsCSV comma separated (no space) global offsets "oNx,oNy,oNz" defined by other variables
     */
    void CreateAttribute( const std::string groupName, const std::string attributeName,
                          const std::string type, const std::string value,
                          const std::string globalDimensionsCSV = "", const std::string globalOffsetsCSV = "" );

    /**
     * Sets a transport method to be associated with a group. Need to think variadic function for other methods
     * @param groupName unique name
     * @param transport transport method
     */
    void SetTransport( const std::string groupName, const std::string transport );


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

    CCapsule m_Capsule; ///< manager of data transports, transforms and movement operations

    /**
     * @brief Maximum buffer size in ADIOS write() operation. From buffer max - size - MB in XML file
     * Note, that if there are two ADIOS outputs going on at the same time,
     * ADIOS will allocate two separate buffers each with the specified maximum limit.
     * Default = 0 means there is no limit
     */
    unsigned long int MaxBufferSizeInMB = 0;

    /**
     * @brief Checks for group existence in m_Groups, if failed throws std::invalid_argument exception
     * @param itGroup group iterator, usually from find function
     * @param groupName passed for thrown exception only
     * @param hint adds information to thrown exception
     */
    void CheckGroup( std::map< std::string, CGroup >::const_iterator itGroup, const std::string groupName, const std::string hint );
};



} //end namespace




#endif /* ADIOS_H_ */
