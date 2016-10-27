/*
 * CGroup.cpp
 *
 *  Created on: Oct 12, 2016
 *      Author: wfg
 */


#include <iostream>


#include "core/CGroup.h"
#include "functions/GroupFunctions.h" //for CreateVariableLanguage
#include "public/SSupport.h"
#include "functions/ADIOSFunctions.h" //for XML Parsing GetTag


namespace adios
{


CGroup::CGroup( const std::string& hostLanguage, const bool debugMode ):
    m_HostLanguage{ hostLanguage },
    m_DebugMode{ debugMode }
{ }


CGroup::CGroup( const std::string& hostLanguage, const std::string& xmlGroup, std::string& groupName, const bool debugMode ):
    m_HostLanguage{ hostLanguage },
    m_DebugMode{ debugMode }
{
    ParseXMLGroup( xmlGroup, groupName );
}


CGroup::~CGroup( )
{ }


void CGroup::Open( const std::string fileName, const std::string accessMode )
{
    m_IsOpen = true;
    m_Transport->Open( fileName, accessMode );
}


void CGroup::SetVariable( const std::string name, const bool isGlobal, const std::string type, const std::string dimensionsCSV,
                          const std::string transform )
{
    if( m_DebugMode == true )
    {
        if( m_Variables.count( name ) == 0 ) //variable doesn't exists
            CreateVariableLanguage( m_HostLanguage, name, isGlobal, type, dimensionsCSV, transform, m_Variables );
        else //name is found
            throw std::invalid_argument( "ERROR: variable " + name + " exists more than once.\n" );
    }
    else
    {
        CreateVariableLanguage( m_HostLanguage, name, isGlobal, type, dimensionsCSV, transform, m_Variables );
    }
}


void CGroup::SetAttribute( const std::string name, const bool isGlobal, const std::string type, const std::string path,
                           const std::string value )
{
    if( m_DebugMode == true )
    {
        if( m_Attributes.count( name ) == 0 ) //variable doesn't exists
            m_Attributes.emplace( name, SAttribute{ isGlobal, type, value } );
        else //name is found
            throw std::invalid_argument( "ERROR: attribute " + name + " exists, NOT setting a new variable\n" );
    }
    else
    {
        m_Attributes.emplace( name, SAttribute{ isGlobal, type, value } );
    }
}


void CGroup::SetGlobalBounds( const std::string dimensionsCSV, const std::string offsetsCSV )
{
    auto lf_SetGlobalMember = []( const std::string inputCSV, std::vector<std::string>& output )
    {
        if( inputCSV.empty() ) return;
        std::istringstream inputSS( inputCSV );
        std::string element;

        while( std::getline( inputSS, element, ',' ) )  //might have to check for "comma" existence
            output.push_back( element );
    };

    if( m_DebugMode == true )
    {
        if( m_GlobalDimensions.empty() == false ) throw std::invalid_argument("ERROR: global dimensions already set\n" );
        if( m_GlobalOffsets.empty() == false ) throw std::invalid_argument("ERROR: global offsets already set\n" );
    }

    lf_SetGlobalMember( dimensionsCSV, m_GlobalDimensions );
    lf_SetGlobalMember( offsetsCSV, m_GlobalOffsets );
}


void CGroup::SetTransport( const std::string method, const unsigned int priority, const unsigned int iteration,
                           const MPI_Comm mpiComm )
{
    if( m_DebugMode == true )
    {
        if( SSupport::Transports.count( method ) == 0 )
            throw std::invalid_argument( "ERROR: transport method " + method + " not supported. Check spelling or case sensitivity.\n" );
    }

    if( m_ActiveTransport.empty() == false ) //there is an existing transport method
        m_Transport.reset();

    CreateTransport( method, priority, iteration, mpiComm, m_Transport );
    m_ActiveTransport = method;
}


void CGroup::Write( const std::string variableName, const void* values )
{
    auto itVariable = m_Variables.find( variableName );

    if( m_DebugMode == true )
    {
        if( itVariable == m_Variables.end() )
            throw std::invalid_argument( "ERROR: variable " + variableName + " is undefined.\n" );
    }
    // must move this to GroupFunctions.h

    const std::string type = itVariable->second->m_Type;
    auto& variable = itVariable->second;

    //Set variable values
    if( type == "double" ) variable->Set<double>( values );
    else if( type == "integer" ) variable->Set<int>( values );
    else if( type == "std::vector<int>" || type == "vector<int>"  ) variable->Set<std::vector<int>>( values );

//    else if( type == "unsigned integer" ) variable->Set<unsigned int>( values );
//    else if( type == "float" ) variable->Set<float>( values );

    std::cout << "Hello from " << type << " variable " << variableName << "\n";
    m_Transport->Write( *variable ); //Using shared_ptr for Variable, must dereference
}


void CGroup::Close( )
{
    //here must think what to do with Capsule and close Transport
}

//PRIVATE FUNCTIONS BELOW
void CGroup::Monitor( std::ostream& logStream ) const
{
    logStream << "\tVariable \t Type\n";
    for( auto& variablePair : m_Variables )
    {
        logStream << "\t" << variablePair.first << " \t " << variablePair.second->m_Type << "\n";
    }
    std::cout << "\n";

    logStream << "\tAttribute \t Type \t Value \n";
    for( auto& attributePair : m_Attributes )
    {
        logStream << "\t" << attributePair.first << " \t " << attributePair.second.Type << " \t " << attributePair.second.Value << "\n";
    }
    std::cout << "\n";

    logStream << "\tTransport Method " << m_ActiveTransport << "\n";
    std::cout << "\n";
}


void CGroup::ParseXMLGroup( const std::string& xmlGroup, std::string& groupName )
{
    //get name
    std::string tag;
    std::string::size_type currentPosition( 0 );
    GetSubString( "<adios-group ", ">", xmlGroup, tag, currentPosition );
    tag = tag.substr( 1, tag.size() - 2 ); //eliminate < >

    std::vector< std::pair<const std::string, const std::string> > pairs;
    GetPairsFromTag( xmlGroup, tag, pairs );

    for( auto& pair : pairs )
    {
        if( pair.first == "name") groupName = pair.second;
    }

    bool isGlobal = false;

    while( currentPosition != std::string::npos )
    {
        GetSubString( "<", ">", xmlGroup, tag, currentPosition );
        if( tag == "</adios-group>" ) break;
        if( tag == "</global-bounds>" ) isGlobal = false;

        tag = tag.substr( 1, tag.size() - 2 ); //eliminate < > needs an exception?
        GetPairsFromTag( xmlGroup, tag, pairs );

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
            SetVariable( name, isGlobal, type, dimensionsCSV, transform );
        }
        else if( tagName == "attribute" )
        {
            std::string name, path, value, type;
            for( auto& pair : pairs ) //loop through all pairs
            {
                if( pair.first == "name"       ) name = pair.second;
                else if( pair.first == "path"  )  path = pair.second;
                else if( pair.first == "value" ) value = pair.second;
                else if( pair.first == "type"  ) type = pair.second;
            }
            SetAttribute( name, isGlobal, type, path, value );
        }
        else if( tagName == "global-bounds" )
        {
            isGlobal = true;
            std::string dimensionsCSV, offsetsCSV;
            for( auto& pair : pairs ) //loop through all pairs
            {
                if( pair.first == "dimensions" ) dimensionsCSV = pair.second;
                else if( pair.first == "offsets" ) offsetsCSV = pair.second;
            }
            SetGlobalBounds( dimensionsCSV, offsetsCSV );
        }
    } //end while loop
}


void CGroup::CheckTransport( const std::string method )
{

}


} //end namespace
