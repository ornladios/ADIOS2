/*
 * Group.h
 *
 *  Created on: Oct 12, 2016
 *      Author: wfg
 */

#ifndef GROUP_H_
#define GROUP_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <map>
#include <string>
#include <memory> //shared_pointer
#include <vector>
#include <ostream>
#include <set>
/// \endcond

#ifdef HAVE_MPI
  #include <mpi.h>
#else
  #include "mpidummy.h"
#endif


#include "core/Variable.h"
#include "core/Attribute.h"
#include "core/Transport.h"
#include "core/Transform.h"
#include "functions/adiosTemplates.h"


namespace adios
{

using Var = std::string; ///< used for returning variables from DefineVariable
/**
 * Class that defines each ADIOS Group composed of Variables, Attributes and GlobalBounds (if global variables exist)
 */
class Group
{
    friend class Engine;

public:

    const std::string m_Name;
    std::vector< std::pair< std::string, std::string > > m_GlobalBounds; ///<  if a variable or an attribute is global it fills this container, from global-bounds in XML File, data in global space, pair.first = global dimensions, pair.second = global bounds

    std::vector< Variable<char> > m_Char; ///< Key: variable name, Value: variable of type char
    std::vector< Variable<unsigned char> > m_UChar; ///< Key: variable name, Value: variable of type unsigned char
    std::vector< Variable<short> > m_Short; ///< Key: variable name, Value: variable of type short
    std::vector< Variable<unsigned short> > m_UShort; ///< Key: variable name, Value: variable of type unsigned short
    std::vector< Variable<int> > m_Int; ///< Key: variable name, Value: variable of type int
    std::vector< Variable<unsigned int> > m_UInt; ///< Key: variable name, Value: variable of type unsigned int
    std::vector< Variable<long int> > m_LInt; ///< Key: variable name, Value: variable of type long int
    std::vector< Variable<unsigned long int> > m_ULInt; ///< Key: variable name, Value: variable of type unsigned long int
    std::vector< Variable<long long int> > m_LLInt; ///< Key: variable name, Value: variable of type long long int
    std::vector< Variable<unsigned long long int> > m_ULLInt; ///< Key: variable name, Value: variable of type unsigned long long int
    std::vector< Variable<float> > m_Float; ///< Key: variable name, Value: variable of type float
    std::vector< Variable<double> > m_Double; ///< Key: variable name, Value: variable of type double
    std::vector< Variable<long double> > m_LDouble; ///< Key: variable name, Value: variable of type double

    /**
     * Empty constructor
     */
    Group( );

    /**
     * Empty constructor
     * @param debugMode true: additional checks throwing exceptions, false: skip checks
     */
    Group( const std::string name, const bool debugMode = false );

    /**
     * @brief Constructor for XML config file
     * @param hostLanguage reference from ADIOS class
     * @param xmlGroup contains <adios-group (tag excluded)....</adios-group> single group definition from XML config file
     * @param transforms passed from ADIOS.m_Transforms, single look up table for all transforms
     * @param debugMode
     */
    Group( const std::string name, const std::string& xmlGroup, std::vector< std::shared_ptr<Transform> >& transforms, const bool debugMode );


    ~Group( ); ///< Using STL containers, no deallocation

    /**
     * Define a new variable in the group object
     * @param name variable name, must be unique in the group. If name exists it removes the current variable. In debug mode program will exit.
     * @param type variable type, must be in SSupport::Datatypes[hostLanguage] in public/SSupport.h
     * @param dimensionsCSV comma separated variable local dimensions (e.g. "Nx,Ny,Nz")
     * @param globalDimensionsCSV comma separated variable global dimensions (e.g. "gNx,gNy,gNz"), if globalOffsetsCSV is also empty variable is local
     * @param globalOffsetsCSV comma separated variable global dimensions (e.g. "gNx,gNy,gNz"), if globalOffsetsCSV is also empty variable is local
     * @param transforms collection of Transform objects applied to this variable, sequence matters, default is empty
     * @param parameters corresponding parameter used by a Transform object in transforms (index should match), default is empty
     */
    Var DefineVariable( const std::string variableName, const std::string type,
                        const std::string dimensionsCSV = "",
                        const std::string globalDimensionsCSV = "", const std::string globalOffsetsCSV = "",
                        const std::vector<Transform*> transforms = std::vector<Transform*>(),
                        const std::vector<int> parameters = std::vector<int>() );


    template< class T >
    Var DefineVariable( const std::string variableName,
                        const std::string dimensionsCSV = "",
                        const std::string globalDimensionsCSV = "", const std::string globalOffsetsCSV = "",
                        const std::vector<Transform*> transforms = std::vector<Transform*>(),
                        const std::vector<int> parameters = std::vector<int>() )
    {
        return DefineVariable( variableName, GetType<T>(), dimensionsCSV, globalDimensionsCSV, globalOffsetsCSV, transforms, parameters );
    }

    /**
     * Sets a variable transform contained in ADIOS Transforms (single container for all groups and variables)
     * @param variableName variable to be assigned a transformation
     * @param transform corresponding transform object, non-const as a pointer is created and pushed to a vector
     * @param parameter optional parameter interpreted by the corresponding Transform, default = -1
     */
    void AddTransform( const std::string variableName, Transform& transform, const int parameter = -1 );

    /**
     * Define a new attribute
     * @param attributeName attribute name, must be unique. If name exists it removes the current variable. In debug mode program will exit.
     * @param type attribute type string or numeric type
     * @param value information about the attribute
     */
    void DefineAttribute( const std::string attributeName, const std::string type, const std::string value );

    /**
     * @brief Dumps groups information to a file stream or standard output.
     * Note that either the user closes this fileStream or it's closed at the end.
     * @param logStream either std::cout standard output, or a std::ofstream file
     */
    void Monitor( std::ostream& logStream ) const;

    /**
     * Looks for variables defining a variable dimensions and return their actual values in a vector
     * @param dimensionsCSV comma separated dimensions "Nx,Ny,Nz"
     * @return actual vector values = { Nx, Ny, Nz }
     */
    std::vector<unsigned long long int> GetDimensions( const std::string dimensionsCSV ) const;

private:

    std::set<std::string> m_WrittenVariables;

    bool m_DebugMode = false; ///< if true will do more checks, exceptions, warnings, expect slower code, known at compile time

    std::map< std::string, std::pair< std::string, unsigned int > > m_Variables; ///< Makes variable name unique, key: variable name, value: pair.first = type, pair.second = index in corresponding vector of Variable

    /**
     * @brief Contains all group attributes from SAttribute.h
     * <pre>
     *     Key: std::string unique attribute name
     *     Value: SAttribute, plain-old-data struct
     * </pre>
     */
    std::map< std::string, Attribute > m_Attributes;

    /**
     * Called from XML constructor
     * @param xmlGroup contains <adios-group....</adios-group> single group definition from XML config file passing by reference as it could be big
     */
    void ParseXMLGroup( const std::string& xmlGroup, std::vector< std::shared_ptr<Transform> >& transforms );

    /**
     * Used by SetVariable and SetAttribute to check if global bounds exist in m_GlobalBounds
     * @param globalDimensionsCSV comma separated variables defining global dimensions (e.g. "Nx,NY,Nz")
     * @param globalOffsetsCSV comma separated variables defining global offsets (e.g. "oNx,oNY,oNz")
     * @return -1 if not global --> both inputs are empty, otherwise index in m_GlobalBounds if exist or create a new element in m_GlobalBounds;
     */
    int SetGlobalBounds( const std::string globalDimensionsCSV, const std::string globalOffsetsCSV ) noexcept;

    /**
     * Retrieves the value of a variable representing another's variable dimensions. Set with Write
     * Must of integer type (from short to unsigned long long int) and positive.
     * used by function GetDimensions
     * @param variableName  variable to be searched in m_SetVariables
     * @return variable value
     */
    unsigned long long int GetIntVariableValue( const std::string variableName ) const;

    /**
     * Looks for variables assigned for dimensions in csv entry (local dimensions, global dimensions, or global offsets), and sets the bool flag IsDimension to true
     * If m_DebugMode is true throws an exception if the variable is not found
     * @param csv comma separated values string containing the dimension variables to look for
     * @param hint message used if exceptions are thrown in debug mode to provide more context
     */
    void SetDimensionVariablesFlag( const std::string csv, const std::string hint );

};


} //end namespace


#endif /* GROUP_H_ */
