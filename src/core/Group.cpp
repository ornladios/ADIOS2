/*
 * Group.cpp
 *
 *  Created on: Oct 12, 2016
 *      Author: wfg
 */

/// \cond EXCLUDED_FROM_DOXYGEN
#include <algorithm> // std::find
#include <sstream> // std::istringstream
/// \endcond


#include "core/Group.h"
#include "core/Support.h"
#include "functions/adiosFunctions.h"


namespace adios
{


Group::Group( )
{ }


Group::Group( const std::string& xmlGroup, std::vector< std::shared_ptr<Transform> >& transforms, const bool debugMode ):
    m_DebugMode{ debugMode }
{
    ParseXMLGroup( xmlGroup, transforms );
}


Group::~Group( )
{ }


void Group::DefineVariable( const std::string variableName, const std::string type,
                            const std::string dimensionsCSV,
                            const std::string globalDimensionsCSV, const std::string globalOffsetsCSV,
                            const Transform* transform, const unsigned short parameter )
{
    if( m_DebugMode == true )
    {
        if( m_Variables.count( variableName ) == 1 )
            throw std::invalid_argument( "ERROR: variable " + variableName + " already exists, in call to DefineVariable\n" );
    }

    short globalBoundsIndex = SetGlobalBounds( globalDimensionsCSV, globalOffsetsCSV );

    if( IsTypeAlias( type, Support::DatatypesAliases.at("char") ) == true )
    {
        m_Char.push_back( Variable<char>{ dimensionsCSV, nullptr, globalBoundsIndex, transform, parameter } );
        m_Variables[variableName] = std::make_pair( type, m_Char.size()-1 );
    }
    else if( IsTypeAlias( type, Support::DatatypesAliases.at("unsigned char") ) == true )
    {
        m_UChar.push_back( Variable<unsigned char>{ dimensionsCSV, nullptr, globalBoundsIndex, transform, parameter } );
        m_Variables[variableName] = std::make_pair( type, m_UChar.size()-1 );
    }
    else if( IsTypeAlias( type, Support::DatatypesAliases.at("short") ) == true )
    {
        m_Short.push_back( Variable<short>{ dimensionsCSV, nullptr, globalBoundsIndex, transform, parameter } );
        m_Variables[variableName] = std::make_pair( type, m_Short.size()-1 );
    }
    else if( IsTypeAlias( type, Support::DatatypesAliases.at("unsigned short") ) == true )
    {
        m_UShort.push_back( Variable<unsigned short>{ dimensionsCSV, nullptr, globalBoundsIndex, transform, parameter } );
        m_Variables[variableName] = std::make_pair( type, m_UShort.size()-1 );
    }
    else if( IsTypeAlias( type, Support::DatatypesAliases.at("int") ) == true )
    {
        m_Int.push_back( Variable<int>{ dimensionsCSV, nullptr, globalBoundsIndex, transform, parameter } );
        m_Variables[variableName] = std::make_pair( type, m_Int.size()-1 );
    }
    else if( IsTypeAlias( type, Support::DatatypesAliases.at("unsigned int") ) == true )
    {
        m_UInt.push_back( Variable<unsigned int>{ dimensionsCSV, nullptr, globalBoundsIndex, transform, parameter } );
        m_Variables[variableName] = std::make_pair( type, m_UInt.size()-1 );
    }
    else if( IsTypeAlias( type, Support::DatatypesAliases.at("long int") ) == true )
    {
        m_LInt.push_back( Variable<long int>{ dimensionsCSV, nullptr, globalBoundsIndex, transform, parameter } );
        m_Variables[variableName] = std::make_pair( type, m_LInt.size()-1 );
    }
    else if( IsTypeAlias( type, Support::DatatypesAliases.at("unsigned long int") ) == true )
    {
        m_ULInt.push_back( Variable<unsigned long int>{ dimensionsCSV, nullptr, globalBoundsIndex, transform, parameter } );
        m_Variables[variableName] = std::make_pair( type, m_ULInt.size()-1 );
    }
    else if( IsTypeAlias( type, Support::DatatypesAliases.at("long long int") ) == true )
    {
        m_LLInt.push_back( Variable<long long int>{ dimensionsCSV, nullptr, globalBoundsIndex, transform, parameter } );
        m_Variables[variableName] = std::make_pair( type, m_LLInt.size()-1 );
    }
    else if( IsTypeAlias( type, Support::DatatypesAliases.at("unsigned long long int") ) == true )
    {
        m_ULLInt.push_back( Variable<unsigned long long int>{ dimensionsCSV, nullptr, globalBoundsIndex, transform, parameter } );
        m_Variables[variableName] = std::make_pair( type, m_ULLInt.size()-1 );
    }
    else if( IsTypeAlias( type, Support::DatatypesAliases.at("float") ) == true )
    {
        m_Float.push_back( Variable<float>{ dimensionsCSV, nullptr, globalBoundsIndex, transform, parameter } );
        m_Variables[variableName] = std::make_pair( type, m_Float.size()-1 );
    }
    else if( IsTypeAlias( type, Support::DatatypesAliases.at("double") ) == true )
    {
        m_Double.push_back( Variable<double>{ dimensionsCSV, nullptr, globalBoundsIndex, transform, parameter } );
        m_Variables[variableName] = std::make_pair( type, m_Double.size()-1 );
    }
    else if( IsTypeAlias( type, Support::DatatypesAliases.at("long double") ) == true )
    {
        m_LDouble.push_back( Variable<long double>{ dimensionsCSV, nullptr, globalBoundsIndex, transform, parameter } );
        m_Variables[variableName] = std::make_pair( type, m_LDouble.size()-1 );
    }
    else
    {
        if( m_DebugMode == true )
            throw std::invalid_argument( "ERROR: type " + type + " not supported, in call to DefineVariable.\n" );
    }

    m_SerialSize += variableName.size() + type.size() + dimensionsCSV.size() + 4 * sizeof( char ); //4, adding one more for globalBoundsIndex
}


void Group::SetTransform( const std::string variableName, const Transform& transform,
                          const short parameter )
{
    auto itVariable = m_Variables.find( variableName );

    if( m_DebugMode == true )
    {
        if( itVariable == m_Variables.end() ) //variable doesn't exists
            throw std::invalid_argument( "ERROR: variable " + variableName + " doesn't exist, in call to SetTransform.\n" );
    }

    const std::string type( itVariable->second.first );
    const unsigned int index = itVariable->second.second;

    if( IsTypeAlias( type, Support::DatatypesAliases.at("char") ) == true )
    {
        m_Char[index].Transform = &transform;
        m_Char[index].Parameter = parameter;
    }
    else if( IsTypeAlias( type, Support::DatatypesAliases.at("unsigned char") ) == true )
    {
        m_UChar[index].Transform = &transform;
        m_UChar[index].Parameter = parameter;
    }
    else if( IsTypeAlias( type, Support::DatatypesAliases.at("short") ) == true )
    {
        m_Short[index].Transform = &transform;
        m_Short[index].Parameter = parameter;
    }
    else if( IsTypeAlias( type, Support::DatatypesAliases.at("unsigned short") ) == true )
    {
        m_UShort[index].Transform = &transform;
        m_UShort[index].Parameter = parameter;
    }
    else if( IsTypeAlias( type, Support::DatatypesAliases.at("int") ) == true )
    {
        m_Int[index].Transform = &transform;
        m_Int[index].Parameter = parameter;
    }
    else if( IsTypeAlias( type, Support::DatatypesAliases.at("unsigned int") ) == true )
    {
        m_UInt[index].Transform = &transform;
        m_UInt[index].Parameter = parameter;
    }
    else if( IsTypeAlias( type, Support::DatatypesAliases.at("long int") ) == true )
    {
        m_LInt[index].Transform = &transform;
        m_LInt[index].Parameter = parameter;
    }
    else if( IsTypeAlias( type, Support::DatatypesAliases.at("unsigned long int") ) == true )
    {
        m_ULInt[index].Transform = &transform;
        m_ULInt[index].Parameter = parameter;
    }
    else if( IsTypeAlias( type, Support::DatatypesAliases.at("unsigned long long int") ) == true )
    {
        m_ULLInt[index].Transform = &transform;
        m_ULLInt[index].Parameter = parameter;
    }
    else if( IsTypeAlias( type, Support::DatatypesAliases.at("float") ) == true )
    {
        m_Float[index].Transform = &transform;
        m_Float[index].Parameter = parameter;
    }
    else if( IsTypeAlias( type, Support::DatatypesAliases.at("double") ) == true )
    {
        m_Double[index].Transform = &transform;
        m_Double[index].Parameter = parameter;
    }
    else if( IsTypeAlias( type, Support::DatatypesAliases.at("long double") ) == true )
    {
        m_LDouble[index].Transform = &transform;
        m_LDouble[index].Parameter = parameter;
    }
}


void Group::DefineAttribute( const std::string attributeName, const std::string type, const std::string value )
{
    auto lf_GetTypeID = []( const std::string type, const bool debugMode ) -> const char
    {
        char typeID;
        if( type == "string" )
            typeID = '0';
        else if( type == "numeric" )
            typeID = '1';
        else
        {
            if( debugMode == true )
                throw std::invalid_argument( "ERROR: type " + type + " must be string or numeric, in call to DefineAttribute\n" );
        }

        return typeID;
    };


    if( m_DebugMode == true )
    {
        if( m_Attributes.count( attributeName ) == 0 ) //attribute doesn't exists
            m_Attributes.emplace( attributeName, Attribute{ lf_GetTypeID( type, m_DebugMode ), value } );
        else //name is found
            throw std::invalid_argument( "ERROR: attribute " + attributeName + " exists, NOT setting a new variable\n" );
    }
    else
    {
        m_Attributes.emplace( attributeName, Attribute{ lf_GetTypeID( type, m_DebugMode ), value } );
    }

    m_SerialSize += attributeName.size() + 1 + value.size() + 3 * sizeof( char ); //3 is one byte storing the size as a char
}


const unsigned long long int Group::GetIntVariableValue( const std::string variableName ) const
{
    if( m_DebugMode == true )
    {
        if( m_SetVariables.count( variableName ) == 0 )
            throw std::invalid_argument( "ERROR: variable value for " + variableName + " was not set with Write function\n" );
    }

    const std::string type( m_Variables.at( variableName ).first );
    const unsigned int index = m_Variables.at( variableName ).second;
    long long int value = -1;

    if( IsTypeAlias( type, Support::DatatypesAliases.at("short") ) == true )
        value = *( m_Short[index].Values );

    else if( IsTypeAlias( type, Support::DatatypesAliases.at("unsigned short") ) == true )
        value = *( m_UShort[index].Values );

    else if( IsTypeAlias( type, Support::DatatypesAliases.at("int") ) == true )
        value = *( m_Int[index].Values );

    else if( IsTypeAlias( type, Support::DatatypesAliases.at("unsigned int") ) == true )
        value = *( m_UInt[index].Values );

    else if( IsTypeAlias( type, Support::DatatypesAliases.at("long int") ) == true )
        value = *( m_LInt[index].Values );

    else if( IsTypeAlias( type, Support::DatatypesAliases.at("unsigned long int") ) == true )
        value = *( m_ULInt[index].Values );

    else if( IsTypeAlias( type, Support::DatatypesAliases.at("long long int") ) == true )
        value = *( m_LLInt[index].Values );

    else if( IsTypeAlias( type, Support::DatatypesAliases.at("unsigned long long int") ) == true )
        value = *( m_ULLInt[index].Values );

    else
    {
        if( m_DebugMode == true )
            throw std::invalid_argument( "ERROR: variable " + variableName + " must be of integer type : short, int or associated type (long int, unsigned long int, etc.)\n" );
    }

    if( m_DebugMode == true )
    {
        if( value <= 0 )
            throw std::invalid_argument( "ERROR: variable " + variableName + " must be >= 0 to represent a dimension\n" );
    }

    return value;
}


std::vector<unsigned long long int> Group::GetDimensions( const std::string dimensionsCSV ) const
{
    std::vector<unsigned long long int> dimensions;

    if( dimensionsCSV.find(',') == dimensionsCSV.npos ) //check if 1D
    {
        const std::string dimension( dimensionsCSV );
        dimensions.push_back( GetIntVariableValue( dimension ) );
        return dimensions;
    }

    std::istringstream dimensionsSS( dimensionsCSV );
    std::string dimension;
    while( std::getline( dimensionsSS, dimension, ',' ) ) //need to test
    {
        dimensions.push_back( GetIntVariableValue( dimension ) );
    }

    return dimensions;
}


//PRIVATE FUNCTIONS BELOW
void Group::Monitor( std::ostream& logStream ) const
{
    logStream << "\tVariable \t Type\n";
    for( auto& variablePair : m_Variables )
    {
        logStream << "\t" << variablePair.first << " \t " << variablePair.second.first << "\n";
    }
    logStream << "\n";

    logStream << "\tAttribute \t Type \t Value \n";
    for( auto& attributePair : m_Attributes )
    {
        logStream << "\t" << attributePair.first << " \t " << attributePair.second.TypeID << " \t " << attributePair.second.Value << "\n";
    }
    logStream << "\n";
}


void Group::ParseXMLGroup( const std::string& xmlGroup, std::vector< std::shared_ptr<Transform> >& transforms )
{
    std::string::size_type currentPosition( 0 );
    std::string globalDimensionsCSV; //used to set variables
    std::string globalOffsetsCSV; //used to set variables

    while( currentPosition != std::string::npos )
    {
        //Get tag
        std::string tag;
        GetSubString( "<", ">", xmlGroup, tag, currentPosition );
        if( tag == "</adios-group>" ) break; //end of current group

        if( tag == "</global-bounds>" )
        {
            globalDimensionsCSV.clear(); //used for variables
            globalOffsetsCSV.clear(); //used for variables
        }

        if( m_DebugMode == true )
        {
            if( tag.size() < 2 )
                throw std::invalid_argument( "ERROR: wrong tag " + tag + " when reading group \n" ); //check < or <=)
        }
        tag = tag.substr( 1, tag.size() - 2 ); //eliminate < >

        //Get pairs from tag
        std::vector< std::pair<const std::string, const std::string> > pairs;
        GetPairsFromTag( xmlGroup, tag, pairs );

        //Check based on tagName
        const std::string tagName( tag.substr( 0, tag.find_first_of(" \t\n\r") ) );

        if( tagName == "var" ) //assign a Group variable
        {
            std::string name, type, transform, dimensionsCSV("1");

            for( auto& pair : pairs ) //loop through all pairs
            {
                if( pair.first == "name"       ) name = pair.second;
                else if( pair.first == "type"       ) type = pair.second;
                else if( pair.first == "dimensions" ) dimensionsCSV = pair.second;
                else if( pair.first == "transform"  ) transform = pair.second;
            }

            int transformIndex = -1;
            int compressionLevel = 0;

            if( transform.empty() == false ) //if transform is present
                SetTransformHelper( transform, transforms, m_DebugMode, transformIndex, compressionLevel );

            DefineVariable( name, type, dimensionsCSV, globalDimensionsCSV, globalOffsetsCSV, transformIndex, compressionLevel );
        }
        else if( tagName == "attribute" )
        {
            std::string name, value, type;
            for( auto& pair : pairs ) //loop through all pairs
            {
                if( pair.first == "name"       ) name = pair.second;
                else if( pair.first == "value" ) value = pair.second;
                else if( pair.first == "type"  ) type = pair.second;
            }
            DefineAttribute( name, type, value );
        }
        else if( tagName == "global-bounds" )
        {
            for( auto& pair : pairs ) //loop through all pairs
            {
                if( pair.first == "dimensions" )
                    globalDimensionsCSV = pair.second;
                else if( pair.first == "offsets" )
                    globalOffsetsCSV = pair.second;
            }

            if( m_DebugMode == true )
            {
                if( globalDimensionsCSV.empty() )
                    throw std::invalid_argument( "ERROR: dimensions missing in global-bounds tag\n");

                if( globalOffsetsCSV.empty() )
                    throw std::invalid_argument( "ERROR: offsets missing in global-bounds tag\n");
            }
        }
    } //end while loop
}


const short Group::SetGlobalBounds( const std::string globalDimensionsCSV, const std::string globalOffsetsCSV ) noexcept
{
    if( globalDimensionsCSV.empty() || globalOffsetsCSV.empty() )
        return -1;

    int globalBoundsIndex = -1;
    const auto globalBounds = std::make_pair( globalDimensionsCSV, globalOffsetsCSV );
    auto itGlobalBounds = std::find( m_GlobalBounds.begin(), m_GlobalBounds.end(), globalBounds );

    if( itGlobalBounds != m_GlobalBounds.end() )
    {
        globalBoundsIndex = std::distance( m_GlobalBounds.begin(), itGlobalBounds );
    }
    else
    {
        m_GlobalBounds.push_back( globalBounds );
        globalBoundsIndex = m_GlobalBounds.size();
    }

    return globalBoundsIndex;
}



} //end namespace
