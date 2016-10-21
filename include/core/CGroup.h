/*
 * CGroup.h
 *
 *  Created on: Oct 12, 2016
 *      Author: wfg
 */

#ifndef CGROUP_H_
#define CGROUP_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <map>
#include <string>
#include <memory> //for shared_pointer
#include <vector>
#include <ostream>
/// \endcond

#ifdef HAVE_MPI
  #include <mpi.h>
#else
  #include "public/mpidummy.h"
#endif


#include "core/CVariable.h"
#include "core/SAttribute.h"
#include "core/CTransport.h"


namespace adios
{

/**
 * Class that defines each ADIOS Group composed of Variables, Attributes and a Transport method
 */
class CGroup
{

public:

    /**
     * @brief Constructor for XML config file
     * @param xmlGroup contains <adios-group....</adios-group> single group definition from XML config file
     * @param groupName returns the groupName from <adios-group name=" "
     * @param debugMode
     */
    CGroup( const std::string& xmlGroup, std::string& groupName, const bool debugMode = false );

    CGroup( const bool debugMode = false ); ///Non-XML empty constructor

    ~CGroup( ); ///< Using STL containers, no deallocation

    /**
     * Opens group and passes fileName and accessMode to m_Transport
     * @param fileName
     * @param accessMode
     */
    void Open( const std::string fileName, const std::string accessMode = "w" );


    /**
     * Passes variableName and values to m_Transport
     * @param variableName
     * @param values
     */
    template<class T>
    void Write( const std::string variableName, const T* values );


    /**
     * @brief Sets a variable in current Group, name must be unique
     * @param name
     * @param isGlobal
     * @param type supported type
     * @param dimensionsCSV comma separated dimensions, default 1D = {1}
     * @param transform method, format = lib or lib:level, where lib = zlib, bzip2, szip, and level=1:9 . If no level is defined then library default is taken
     */
    void SetVariable( const std::string name, const bool isGlobal, const std::string type,
                      const std::string dimensionsCSV = "1", const std::string transform = "" );

    /**
     * @brief Sets a variable in current Group
     * @param name
     * @param isGlobal
     * @param type
     * @param path
     * @param value
     */
    void SetAttribute( const std::string name, const bool isGlobal, const std::string type, const std::string path, const std::string value );


    /**
     * @brief Sets global dimensions and offsets
     * @param dimensionsCSV global dimensions in comma-separated-value format "gdx,gdy"
     * @param offsetsCSV global offsets in comma-separated-value format "gdx,gdy"
     */
    void SetGlobalBounds( const std::string dimensionsCSV, const std::string offsetsCSV = "" );

    /**
     * @brief MPI version of SetTransport, includes the communicator
     * @param method supported values in SSupport.h TransportMethods
     * @param priority numeric priority for the I/O to schedule this write with others that might be pending
     * @param iteration iterations between writes of a group to gauge how quickly this data should be evacuated from the compute node
     * @param mpiComm MPI communicator from User->ADIOS->Group
     */
    void SetTransport( const std::string method, const unsigned int priority, const unsigned int iteration,
                       const MPI_Comm mpiComm );


    /**
     * @brief Dumps groups information to a file stream or standard output.
     * Note that either the user closes this fileStream or it's closed at the end.
     * @param logStream either std::cout standard output, or a std::ofstream file
     */
    void Monitor( std::ostream& logStream ) const;

    void Close( ); ///< Close Group


private:

    /**
     * @brief Contains all group variables (from XML Config file).
     * <pre>
     *     Key: std::string unique variable name
     *     Value: Children of SVariable struct defined in SVariable.h, using unique_ptr for polymorphism
     * </pre>
     */
    std::map< std::string, std::shared_ptr<CVariable> > m_Variables;
    std::vector< SAttribute > m_Attributes; ///< Contains all group attributes from SAttribute.h, should be moved to a map

    std::vector< std::string > m_GlobalDimensions; ///< from global-bounds in XML File, data in global space
    std::vector< std::string > m_GlobalOffsets; ///< from global-bounds in XML File, data in global space

    std::shared_ptr< CTransport > m_Transport; ///< transport method defined in XML File, using shared pointer as SGroup can be uninitialized
    std::string m_ActiveTransport;
    bool m_DebugMode = false; ///< if true will do more checks, exceptions, warnings, expect slower code

    bool m_IsOpen = false; ///< checks if group was opened for operations;
    std::string m_FileName; ///< associated fileName is the Group is opened.
    std::string m_AcessMode; ///< file access mode

    /**
     * Called from XML constructor
     * @param xmlGroup contains <adios-group....</adios-group> single group definition from XML config file
     * @param groupName returns the groupName from <adios-group name=" "
     */
    void ParseXMLGroup( const std::string& xmlGroup, std::string& groupName );

    /**
     * Function that checks if transport method is valid, called from overloaded SetTransform functions
     * @param method transport method to be checked from SSupport
     */
    void CheckTransport( const std::string method );
};


} //end namespace




#endif /* CGROUP_H_ */
