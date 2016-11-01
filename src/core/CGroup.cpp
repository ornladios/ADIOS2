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
#include "functions/ADIOSFunctions.h" //for XML Parsing functions (e.g. GetTag)
#include "core/CVariable.h" //for cast implementation of CVariableBase::Set that calls CVariable::Set


namespace adios
{


CGroup::CGroup( const std::string& hostLanguage, const bool debugMode ):
    m_HostLanguage{ hostLanguage },
    m_DebugMode{ debugMode }
{ }


CGroup::CGroup( const std::string& hostLanguage, const std::string& xmlGroup, const bool debugMode ):
    m_HostLanguage{ hostLanguage },
    m_DebugMode{ debugMode }
{
    ParseXMLGroup( xmlGroup );
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
        if( SSupport::Datatypes.at( m_HostLanguage ).count( type ) == 0 )
            throw std::invalid_argument( "ERROR: type " + type + " for variable " + name + " is not supported.\n" );

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

    if( m_ActiveTransport.empty() == false ) //there is an existing transport method, so reset
        m_Transport.reset();

    CreateTransport( method, priority, iteration, mpiComm, m_DebugMode, m_Transport );
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

    SetVariableValues( *itVariable->second, values ); //will check type and cast to appropriate template<type>
    //here must do something with Capsule or transport
    m_Transport->Write( *itVariable->second ); //transport will write?
}


void CGroup::Close( )
{
    //here must think what to do with Capsule and close Transport
    m_Transport->Close( );
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


void CGroup::ParseXMLGroup( const std::string& xmlGroup )
{
    std::string::size_type currentPosition( 0 );
    bool isGlobal = false;

    while( currentPosition != std::string::npos )
    {
        //Get tag
        std::string tag;
        GetSubString( "<", ">", xmlGroup, tag, currentPosition );
        if( tag == "</adios-group>" ) break;
        if( tag == "</global-bounds>" ) isGlobal = false;

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


} //end namespace
