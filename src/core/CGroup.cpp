/*
 * CGroup.cpp
 *
 *  Created on: Oct 12, 2016
 *      Author: wfg
 */


#include <iostream>
#include <algorithm> // find
#include <sstream> //istringstream

#include "core/CGroup.h"
#include "core/SVariable.h" //for cast implementation of CVariableBase::Set that calls CVariable::Set
#include "public/SSupport.h"
#include "functions/ADIOSFunctions.h" //for XML Parsing functions (e.g. GetTag)


namespace adios
{


CGroup::CGroup( const std::string hostLanguage, const bool debugMode ):
    m_HostLanguage{ hostLanguage },
    m_DebugMode{ debugMode }
{ }


CGroup::CGroup( const std::string hostLanguage, const std::string& xmlGroup,
                std::vector< std::shared_ptr<CTransform> >& transforms, const bool debugMode ):
    m_HostLanguage{ hostLanguage },
    m_DebugMode{ debugMode }
{
    ParseXMLGroup( xmlGroup, transforms );
}


CGroup::~CGroup( )
{ }


void CGroup::DefineVariable( const std::string variableName, const std::string type,
                             const std::string dimensionsCSV,
                             const std::string globalDimensionsCSV, const std::string globalOffsetsCSV,
                             const short transformIndex, const unsigned short int compressionLevel )
{
    if( m_DebugMode == true )
    {
        if( SSupport::Datatypes.at( m_HostLanguage ).count( type ) == 0 )
            throw std::invalid_argument( "ERROR: type " + type + " for variable " + variableName +
                                         " is not supported, in call to DefineVariable\n" );

        if( m_Variables.count( variableName ) == 1 )
            throw std::invalid_argument( "ERROR: variable " + variableName + " already exists, in call to DefineVariable\n" );
    }

    short globalBoundsIndex = SetGlobalBounds( globalDimensionsCSV, globalOffsetsCSV );

    if( IsTypeAlias( type, SSupport::DatatypesAliases.at("char") ) == true )
    {
        m_Char.push_back( SVariable<char>{ dimensionsCSV, nullptr, globalBoundsIndex, transformIndex, compressionLevel } );
        m_Variables[variableName] = std::make_pair( type, m_Char.size()-1 );
    }
    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("unsigned char") ) == true )
    {
        m_UChar.push_back( SVariable<unsigned char>{ dimensionsCSV, nullptr, globalBoundsIndex, transformIndex, compressionLevel } );
        m_Variables[variableName] = std::make_pair( type, m_UChar.size()-1 );
    }
    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("short") ) == true )
    {
        m_Short.push_back( SVariable<short>{ dimensionsCSV, nullptr, globalBoundsIndex, transformIndex, compressionLevel } );
        m_Variables[variableName] = std::make_pair( type, m_Short.size()-1 );
    }
    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("unsigned short") ) == true )
    {
        m_UShort.push_back( SVariable<unsigned short>{ dimensionsCSV, nullptr, globalBoundsIndex, transformIndex, compressionLevel } );
        m_Variables[variableName] = std::make_pair( type, m_UShort.size()-1 );
    }
    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("int") ) == true )
    {
        m_Int.push_back( SVariable<int>{ dimensionsCSV, nullptr, globalBoundsIndex, transformIndex, compressionLevel } );
        m_Variables[variableName] = std::make_pair( type, m_Int.size()-1 );
    }
    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("unsigned int") ) == true )
    {
        m_UInt.push_back( SVariable<unsigned int>{ dimensionsCSV, nullptr, globalBoundsIndex, transformIndex, compressionLevel } );
        m_Variables[variableName] = std::make_pair( type, m_UInt.size()-1 );
    }
    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("long int") ) == true )
    {
        m_LInt.push_back( SVariable<long int>{ dimensionsCSV, nullptr, globalBoundsIndex, transformIndex, compressionLevel } );
        m_Variables[variableName] = std::make_pair( type, m_LInt.size()-1 );
    }
    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("unsigned long int") ) == true )
    {
        m_ULInt.push_back( SVariable<unsigned long int>{ dimensionsCSV, nullptr, globalBoundsIndex, transformIndex, compressionLevel } );
        m_Variables[variableName] = std::make_pair( type, m_ULInt.size()-1 );
    }
    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("long long int") ) == true )
    {
        m_LLInt.push_back( SVariable<long long int>{ dimensionsCSV, nullptr, globalBoundsIndex, transformIndex, compressionLevel } );
        m_Variables[variableName] = std::make_pair( type, m_LLInt.size()-1 );
    }
    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("unsigned long long int") ) == true )
    {
        m_ULLInt.push_back( SVariable<unsigned long long int>{ dimensionsCSV, nullptr, globalBoundsIndex, transformIndex, compressionLevel } );
        m_Variables[variableName] = std::make_pair( type, m_ULLInt.size()-1 );
    }
    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("float") ) == true )
    {
        m_Float.push_back( SVariable<float>{ dimensionsCSV, nullptr, globalBoundsIndex, transformIndex, compressionLevel } );
        m_Variables[variableName] = std::make_pair( type, m_Float.size()-1 );
    }
    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("double") ) == true )
    {
        m_Double.push_back( SVariable<double>{ dimensionsCSV, nullptr, globalBoundsIndex, transformIndex, compressionLevel } );
        m_Variables[variableName] = std::make_pair( type, m_Double.size()-1 );
    }
    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("long double") ) == true )
    {
        m_LDouble.push_back( SVariable<long double>{ dimensionsCSV, nullptr, globalBoundsIndex, transformIndex, compressionLevel } );
        m_Variables[variableName] = std::make_pair( type, m_LDouble.size()-1 );
    }

    m_SerialSize += variableName.size() + type.size() + dimensionsCSV.size() + 4 * sizeof( char ); //4, adding one more for globalBoundsIndex
}


void CGroup::SetTransform( const std::string variableName, const unsigned int transformIndex, const unsigned int compressionLevel )
{
    auto itVariable = m_Variables.find( variableName );

    if( m_DebugMode == true )
    {
        if( itVariable == m_Variables.end() ) //variable doesn't exists
            throw std::invalid_argument( "ERROR: variable " + variableName + " doesn't exist, in call to SetTransform.\n" );
    }

    const std::string type( itVariable->second.first );
    const unsigned int index = itVariable->second.second;

    if( IsTypeAlias( type, SSupport::DatatypesAliases.at("char") ) == true )
    {
        m_Char[index].TransformIndex = transformIndex;
        m_Char[index].CompressionLevel = compressionLevel;
    }
    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("unsigned char") ) == true )
    {
        m_UChar[index].TransformIndex = transformIndex;
        m_UChar[index].CompressionLevel = compressionLevel;
    }
    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("short") ) == true )
    {
        m_Short[index].TransformIndex = transformIndex;
        m_Short[index].CompressionLevel = compressionLevel;
    }
    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("unsigned short") ) == true )
    {
        m_UShort[index].TransformIndex = transformIndex;
        m_UShort[index].CompressionLevel = compressionLevel;
    }
    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("int") ) == true )
    {
        m_Int[index].TransformIndex = transformIndex;
        m_Int[index].CompressionLevel = compressionLevel;
    }
    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("unsigned int") ) == true )
    {
        m_UInt[index].TransformIndex = transformIndex;
        m_UInt[index].CompressionLevel = compressionLevel;
    }
    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("long int") ) == true )
    {
        m_LInt[index].TransformIndex = transformIndex;
        m_LInt[index].CompressionLevel = compressionLevel;
    }
    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("unsigned long int") ) == true )
    {
        m_ULInt[index].TransformIndex = transformIndex;
        m_ULInt[index].CompressionLevel = compressionLevel;
    }
    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("unsigned long long int") ) == true )
    {
        m_ULLInt[index].TransformIndex = transformIndex;
        m_ULLInt[index].CompressionLevel = compressionLevel;
    }
    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("float") ) == true )
    {
        m_Float[index].TransformIndex = transformIndex;
        m_Float[index].CompressionLevel = compressionLevel;
    }
    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("double") ) == true )
    {
        m_Double[index].TransformIndex = transformIndex;
        m_Double[index].CompressionLevel = compressionLevel;
    }
    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("long double") ) == true )
    {
        m_LDouble[index].TransformIndex = transformIndex;
        m_LDouble[index].CompressionLevel = compressionLevel;
    }
}


void CGroup::DefineAttribute( const std::string attributeName, const std::string type, const std::string value )
{
    if( m_DebugMode == true )
    {
        if( m_Attributes.count( attributeName ) == 0 ) //attribute doesn't exists
            m_Attributes.emplace( attributeName, SAttribute{ type, value } );
        else //name is found
            throw std::invalid_argument( "ERROR: attribute " + attributeName + " exists, NOT setting a new variable\n" );
    }
    else
    {
        m_Attributes.emplace( attributeName, SAttribute{ type, value } );
    }

    m_SerialSize += attributeName.size() + type.size() + value.size() + 3 * sizeof( char ); //3 is one byte storing the size as a char
}


const unsigned long long int CGroup::GetIntVariableValue( const std::string variableName ) const
{
    if( m_DebugMode == true )
    {
        if( m_SetVariables.count( variableName ) == 0 )
            throw std::invalid_argument( "ERROR: variable value for " + variableName + " was not set with Write function\n" );
    }

    const std::string type( m_Variables.at( variableName ).first );
    const unsigned int index = m_Variables.at( variableName ).second;
    long long int value = -1;

    if( IsTypeAlias( type, SSupport::DatatypesAliases.at("short") ) == true )
        value = *( m_Short[index].Values );

    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("unsigned short") ) == true )
        value = *( m_UShort[index].Values );

    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("int") ) == true )
        value = *( m_Int[index].Values );

    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("unsigned int") ) == true )
        value = *( m_UInt[index].Values );

    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("long int") ) == true )
        value = *( m_LInt[index].Values );

    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("unsigned long int") ) == true )
        value = *( m_ULInt[index].Values );

    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("long long int") ) == true )
        value = *( m_LLInt[index].Values );

    else if( IsTypeAlias( type, SSupport::DatatypesAliases.at("unsigned long long int") ) == true )
        value = *( m_ULLInt[index].Values );

    else
        throw std::invalid_argument( "ERROR: variable " + variableName + " must be of integer type : short, int or associated type (long int, unsigned long int, etc.)\n" );

    if( m_DebugMode == true )
    {
        if( value <= 0 )
            throw std::invalid_argument( "ERROR: variable " + variableName + " must be >= 0 to represent a dimension\n" );
    }

    return value;
}


std::vector<unsigned long long int> CGroup::GetDimensions( const std::string dimensionsCSV ) const
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
void CGroup::Monitor( std::ostream& logStream ) const
{
    logStream << "\tVariable \t Type\n";
    for( auto& variablePair : m_Variables )
    {
        logStream << "\t" << variablePair.first << " \t " << variablePair.second.first << "\n";
    }
    std::cout << "\n";

    logStream << "\tAttribute \t Type \t Value \n";
    for( auto& attributePair : m_Attributes )
    {
        logStream << "\t" << attributePair.first << " \t " << attributePair.second.Type << " \t " << attributePair.second.Value << "\n";
    }
    std::cout << "\n";
}


void CGroup::ParseXMLGroup( const std::string& xmlGroup, std::vector< std::shared_ptr<CTransform> >& transforms )
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


const short CGroup::SetGlobalBounds( const std::string globalDimensionsCSV, const std::string globalOffsetsCSV ) noexcept
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
