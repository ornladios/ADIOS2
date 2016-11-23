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
#include <set>
/// \endcond

#ifdef HAVE_MPI
  #include <mpi.h>
#else
  #include "public/mpidummy.h"
#endif


#include "core/SVariable.h"
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
     * @param hostLanguage reference from ADIOS class
     * @param xmlGroup contains <adios-group (tag excluded)....</adios-group> single group definition from XML config file
     * @param debugMode from ADIOS
     */
    CGroup( const std::string& hostLanguage, const std::string& xmlGroup, const bool debugMode );


    /**
     * Non-XML empty constructor
     * @param hostLanguage reference from ADIOS class
     * @param debugMode
     */
    CGroup( const std::string& hostLanguage, const bool debugMode );

    /**
     * Create a Group object associated with a transport, can be modified later
     * @param hostLanguage reference from ADIOS class
     * @param transport transport method
     * @param priority  transport priority
     * @param iterations iterations for transport
     * @param debugMode reference from ADIOS class
     */
    CGroup( const std::string& hostLanguage, const std::string transport, const unsigned int priority, const unsigned int iterations,
            const bool debugMode );



    ~CGroup( ); ///< Using STL containers, no deallocation

    /**
     * Creates a new variable in the group object
     * @param name variable name, must be unique. If name exists it removes the current variable. In debug mode program will exit.
     * @param type variable type, must be in SSupport::Datatypes[hostLanguage] in public/SSupport.h
     * @param dimensionsCSV comma separated variable local dimensions (e.g. "Nx,Ny,Nz")
     * @param transform method, format = lib or lib:level, where lib = zlib, bzip2, szip, and level=1:9
     * @param globalDimensionsCSV comma separated variable global dimensions (e.g. "gNx,gNy,gNz"), if globalOffsetsCSV is also empty variable is local
     * @param globalOffsetsCSV comma separated variable global dimensions (e.g. "gNx,gNy,gNz"), if globalOffsetsCSV is also empty variable is local
     */
    void CreateVariable( const std::string name, const std::string type,
                         const std::string dimensionsCSV, const short transformIndex,
                         const std::string globalDimensionsCSV, const std::string globalOffsetsCSV );

    /**
     *
     * @param name attribute name, must be unique. If name exists it removes the current variable. In debug mode program will exit.
     * @param type attribute type string or numeric type
     * @param value information about the attribute
     * @param globalDimensionsCSV comma separated variable global dimensions (e.g. "gNx,gNy,gNz"), if globalOffsetsCSV is also empty variable is local
     * @param globalOffsetsCSV comma separated variable global dimensions (e.g. "gNx,gNy,gNz"), if globalOffsetsCSV is also empty variable is local
     */
    void CreateAttribute( const std::string name, const std::string type, const std::string value,
                          const std::string globalDimensionsCSV, const std::string globalOffsetsCSV );

    /**
     * @brief Dumps groups information to a file stream or standard output.
     * Note that either the user closes this fileStream or it's closed at the end.
     * @param logStream either std::cout standard output, or a std::ofstream file
     */
    void Monitor( std::ostream& logStream ) const;


    std::vector<unsigned long long int> CGroup::GetDimensions( const std::string dimensionsCSV ) const;


private:

    const std::string& m_HostLanguage; ///< reference to class ADIOS m_HostLanguage, this erases the copy constructor. Might be moved later to non-reference const
    const bool m_DebugMode = false; ///< if true will do more checks, exceptions, warnings, expect slower code, known at compile time

    std::map< std::string, std::pair< std::string, unsigned int > > m_Variables; ///< Makes variable name unique, key: variable name, value: pair.first = type, pair.second = index in corresponding vector of CVariable

    std::vector< SVariable<char> > m_Char; ///< Key: variable name, Value: variable of type char
    std::vector< SVariable<unsigned char> > m_UChar; ///< Key: variable name, Value: variable of type unsigned char
    std::vector< SVariable<short> > m_Short; ///< Key: variable name, Value: variable of type short
    std::vector< SVariable<unsigned short> > m_UShort; ///< Key: variable name, Value: variable of type unsigned short
    std::vector< SVariable<int> > m_Int; ///< Key: variable name, Value: variable of type int
    std::vector< SVariable<unsigned int> > m_UInt; ///< Key: variable name, Value: variable of type unsigned int
    std::vector< SVariable<long int> > m_LInt; ///< Key: variable name, Value: variable of type long int
    std::vector< SVariable<unsigned long int> > m_ULInt; ///< Key: variable name, Value: variable of type unsigned long int
    std::vector< SVariable<long long int> > m_LLInt; ///< Key: variable name, Value: variable of type long long int
    std::vector< SVariable<unsigned long long int> > m_ULLInt; ///< Key: variable name, Value: variable of type unsigned long long int
    std::vector< SVariable<float> > m_Float; ///< Key: variable name, Value: variable of type float
    std::vector< SVariable<double> > m_Double; ///< Key: variable name, Value: variable of type double
    std::vector< SVariable<long double> > m_LDouble; ///< Key: variable name, Value: variable of type double

    std::set<std::string> m_SetVariables; ///< set of variables whose T* values have been set (no nullptr)

    std::vector< std::string > m_Transforms; ///< if a variable has a transform it fills this container, the variable holds an index

    /**
     * @brief Contains all group attributes from SAttribute.h
     * <pre>
     *     Key: std::string unique attribute name
     *     Value: SAttribute, plain-old-data struct
     * </pre>
     */
    std::map< std::string, SAttribute > m_Attributes;

    std::vector< std::pair< std::string, std::string > > m_GlobalBounds; ///<  if a variable or an attribute is global it fills this container, from global-bounds in XML File, data in global space, pair.first = global dimensions, pair.second = global bounds

    unsigned long int m_SerialSize = 0; ///< size used for potential serialization of metadata into a std::vector<char>. Counts sizes from m_Variables, m_Attributes, m_GlobalBounds

    /**
     * Called from XML constructor
     * @param xmlGroup contains <adios-group....</adios-group> single group definition from XML config file passing by reference as it could be big
     */
    void ParseXMLGroup( const std::string& xmlGroup );


    const unsigned int CurrentTypeIndex( const std::string type ) const noexcept;

    /**
     * Used by SetVariable and SetAttribute to check if global bounds exist in m_GlobalBounds
     * @param globalDimensionsCSV comma separated variables defining global dimensions (e.g. "Nx,NY,Nz")
     * @param globalOffsetsCSV comma separated variables defining global offsets (e.g. "oNx,oNY,oNz")
     * @return -1 if not global --> both inputs are empty, otherwise index in m_GlobalBounds if exist or create a new element in m_GlobalBounds;
     */
    const int SetGlobalBounds( const std::string globalDimensionsCSV, const std::string globalOffsetsCSV ) noexcept;

    /**
     * Used by SetVariable to check if transform exists in m_Transform
     * @param transform variable transformation method (e.g. bzip2, szip, zlib )
     * @return -1 variable is not associated with a transform, otherwise index in m_Transforms
     */
    const int SetTransforms( const std::string transform ) noexcept;


    const unsigned long long int GetIntVariableValue( const std::string variableName ) const;

};


} //end namespace


#endif /* CGROUP_H_ */
