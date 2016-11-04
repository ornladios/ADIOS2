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

    bool m_IsOpen = false; ///< checks if group was opened for operations;

    /**
     * @brief Constructor for XML config file
     * @param hostLanguage reference from ADIOS class
     * @param xmlGroup contains <adios-group (tag excluded)....</adios-group> single group definition from XML config file
     * @param groupName returns the groupName from <adios-group name=" "
     * @param debugMode from ADIOS
     */
    CGroup( const std::string& hostLanguage, const std::string& xmlGroup, const bool debugMode = false );


    /**
     * Non-XML empty constructor
     * @param hostLanguage reference from ADIOS class
     * @param debugMode
     */
    CGroup( const std::string& hostLanguage, const bool debugMode = false );


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
    void Write( const std::string variableName, const void* values );

    /**
     * Must think what to do with Capsule and Transport
     */
    void Close( );


    /**
     * Sets a new variable in the group object
     * @param name variable name, must be unique. If name exists it removes the current variable. In debug mode program will exit.
     * @param type variable type, must be in SSupport::Datatypes[hostLanguage] in public/SSupport.h
     * @param dimensionsCSV comma separated variable local dimensions (e.g. "Nx,Ny,Nz")
     * @param transform method, format = lib or lib:level, where lib = zlib, bzip2, szip, and level=1:9
     * @param globalDimensionsCSV comma separated variable global dimensions (e.g. "gNx,gNy,gNz"), if globalOffsetsCSV is also empty variable is local
     * @param globalOffsetsCSV comma separated variable global dimensions (e.g. "gNx,gNy,gNz"), if globalOffsetsCSV is also empty variable is local
     */
    void SetVariable( const std::string name, const std::string type,
                      const std::string dimensionsCSV, const std::string transform,
                      const std::string globalDimensionsCSV, const std::string globalOffsetsCSV );

    /**
     * @brief Sets a new attribute in current Group
     * @param name  attribute name, must be unique. If name exists it removes the current variable. In debug mode program will exit.
     * @param isGlobal
     * @param type
     * @param value
     */
    void SetAttribute( const std::string name, const bool isGlobal, const std::string type, const std::string value );


    /**
     * @brief Sets m_Transport with available supported method
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


private:

    const std::string& m_HostLanguage; ///< reference to class ADIOS m_HostLanguage, this erases the copy constructor. Might be moved later to non-reference const
    const bool m_DebugMode = false; ///< if true will do more checks, exceptions, warnings, expect slower code

    /**
     * @brief Contains all group variables (from XML Config file).
     * <pre>
     *     Key: std::string unique variable name
     *     Value: Polymorphic value is always unique child defined in SVariableTemplate.h, allow different variable types
     * </pre>
     */
    std::map< std::string, CVariable > m_Variables;
    std::vector< std::string > m_VariableTransforms; ///< if a variable has a transform it fills this container, the variable hold an index


    /**
     * @brief Contains all group attributes from SAttribute.h
     * <pre>
     *     Key: std::string unique attribute name
     *     Value: SAttribute, plain-old-data struct
     * </pre>
     */
    std::map< std::string, SAttribute > m_Attributes;

    std::vector< std::pair< std::string, std::string > > m_GlobalBounds; ///<  if a variable or an attribute is global it fills this container, from global-bounds in XML File, data in global space, pair.first = global dimensions, pair.second = global bounds

    std::string m_CurrentTransport; ///< current transport method associated with this group
    std::string m_OutputName; ///< associated output (file, stream, buffer, etc.) if the Group is opened.

    unsigned long int m_SerialSize; ///< size used for potential serialization of metadata into a std::vector<char>. Counts sizes from m_Variables, m_Attributes, m_GlobalBounds

    /**
     * Called from XML constructor
     * @param xmlGroup contains <adios-group....</adios-group> single group definition from XML config file passing by reference as it could be big
     */
    void ParseXMLGroup( const std::string& xmlGroup );

    /**
     * Function that checks if transport method is valid, called from overloaded SetTransform functions
     * @param method transport method to be checked from SSupport
     */
    void CheckTransport( const std::string method );

};


} //end namespace


#endif /* CGROUP_H_ */
