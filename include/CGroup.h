/*
 * CGroup.h
 *
 *  Created on: Oct 12, 2016
 *      Author: wfg
 */

#ifndef CGROUP_H_
#define CGROUP_H_

#include <map>
#include <string>
#include <memory> //for shared_pointer
#include <vector>

#include "CVariable.h"
#include "SAttribute.h"
#include "CTransport.h"


namespace adios
{

class CGroup
{

public:

    /**
     * @brief Constructor for XML config file
     * @param xmlGroup contains <adios-group....</adios-group> single group definition from XML config file
     * @param groupName returns the groupName from <adios-group name=" "
     */
    CGroup( const std::string& xmlGroup, std::string& groupName );

    CGroup( ); ///Non-XML empty constructor

    ~CGroup( ); ///< Using STL containers

    /**
     * @brief Sets a variable in current Group, name must be unique
     * @param name
     * @param isGlobal
     * @param type supported type
     * @param dimensionsCSV comma separated dimensions, default 1D = {1}
     * @param transform method, format = lib or lib:level, where lib = zlib, bzip2, szip, and level=1:9 . If no level is defined then library default is taken
     */
    void SetVariable( const std::string name, const bool isGlobal, const std::string type,
                      const std::string dimensionsCSV = {1}, const std::string transform = "" );

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
     * Set a transport method for this group
     * @param transport supported values in SSupport.h TransportMethods
     * @param priority numeric priority for the I/O to schedule this write with others that might be pending
     * @param iteration iterations between writes of a group to gauge how quickly this data should be evacuated from the compute node
     * @param verbose level
     */
    void SetTransportMethod( const std::string transport, unsigned int priority = 1, unsigned int iteration=1, unsigned int verbose = 0 );

    void MonitorGroup( ); ///< Dumps information about Group variables


private:

    /**
     * @brief Contains all group variables (from XML Config file).
     * <pre>
     *     Key: std::string unique variable name
     *     Value: Children of SVariable struct defined in SVariable.h, using unique_ptr for polymorphism
     * </pre>
     */
    std::map< std::string, std::shared_ptr<CVariable> > m_Variables;
    std::vector< SAttribute > m_Attributes; ///< Contains all group attributes from SAttribute.h

    std::vector< std::string > m_GlobalDimensions; ///< from global-bounds in XML File, data in global space
    std::vector< std::string > m_GlobalOffsets; ///< from global-bounds in XML File, data in global space

    std::shared_ptr< CTransport > m_TransportMethod; ///< transport method defined in XML File, using shared pointer as SGroup can be uninitialized
    std::string m_ActiveMethod;


    /**
     * Called from XML constructor
     * @param xmlGroup contains <adios-group....</adios-group> single group definition from XML config file
     * @param groupName returns the groupName from <adios-group name=" "
     */
    void ParseXMLGroup( const std::string& xmlGroup, std::string& groupName );
};


} //end namespace




#endif /* CGROUP_H_ */
