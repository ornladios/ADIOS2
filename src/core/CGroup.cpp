/*
 * CGroup.cpp
 *
 *  Created on: Oct 12, 2016
 *      Author: wfg
 */


#include <iostream>


#include "core/CGroup.h"
#include "public/SSupport.h"
#include "functions/ADIOSFunctions.h"
#include "core/CVariableTemplate.h"

//transports
#include "transport/CPOSIX.h"
#include "transport/CFStream.h"


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
    auto lf_SetVariable = [&] ( const std::string name, const bool isGlobal, const std::string type,
                                const std::string dimensionsCSV,
                                const std::string transform )
    {
        if( type == "int") //using copy constructor as it's a small class, only metadata
            m_Variables[name] = std::make_shared< CVariableTemplate<int> >( isGlobal, type, dimensionsCSV, transform );
        else if( type == "unsigned int" )
            m_Variables[name] = std::make_shared< CVariableTemplate<unsigned int> >( isGlobal, type, dimensionsCSV, transform );
        else if( type == "real" )
            m_Variables[name] = std::make_shared< CVariableTemplate<float> >( isGlobal, type, dimensionsCSV, transform );
        else if( type == "double")
            m_Variables[name] = std::make_shared< CVariableTemplate<double> >( isGlobal, type, dimensionsCSV, transform );
        else if( type == "std::vector<int>" )
            m_Variables[name] = std::make_shared< CVariableTemplate< std::vector<int> > >( isGlobal, type, dimensionsCSV, transform );
        else
            throw std::invalid_argument( "ERROR: type " + type + " for variable " + name + "not supported\n" );
    };

    //Function body start  here
    if( m_DebugMode == true )
    {
        if( m_Variables.count( name ) == 0 ) //variable doesn't exists
            lf_SetVariable( name, isGlobal, type, dimensionsCSV, transform );
        else //name is found
            throw std::invalid_argument( "ERROR: variable " + name + " exists more than once.\n" );
    }
    else
    {
        lf_SetVariable( name, isGlobal, type, dimensionsCSV, transform );
    }
}


void CGroup::SetAttribute( const std::string name, const bool isGlobal, const std::string type, const std::string path, const std::string value )
{
    if( m_DebugMode == true )
    {
        if( m_Attributes.count( name ) == 0 ) //variable doesn't exists
            m_Attributes[name] = SAttribute{ isGlobal, type, value }; //copy, it's small
        else //name is found
            throw std::invalid_argument( "ERROR: attribute " + name + " exists, NOT setting a new variable\n" );
    }
    else
    {
        m_Attributes[name] = SAttribute{ isGlobal, type, value }; //copy, it's small
    }
}


void CGroup::SetGlobalBounds( const std::string dimensionsCSV, const std::string offsetsCSV )
{
    if( dimensionsCSV.empty() ) return;

    std::istringstream dimensionsCSVSS( dimensionsCSV );
    std::string dimension;
    while( std::getline( dimensionsCSVSS, dimension, ',' ) )     //might have to check for "comma" existence
    {
        m_GlobalDimensions.push_back( dimension );
    }

    if( offsetsCSV.empty() ) return;

    std::istringstream offsetsCSVSS( offsetsCSV );
    std::string offset;
    while( std::getline( offsetsCSVSS, offset, ',' ) )     //might have to check for "comma" existence
    {
        m_GlobalOffsets.push_back( offset );
    }
}


void CGroup::SetTransport( const std::string method, const unsigned int priority, const unsigned int iteration,
                           const MPI_Comm mpiComm )
{
    CheckTransport( method );
    if( m_ActiveTransport == "POSIX" ) m_Transport = std::make_shared<CPOSIX>( priority, iteration, mpiComm );
    else if( m_ActiveTransport == "FStream" ) m_Transport = std::make_shared<CFStream>( priority, iteration, mpiComm );
}


void CGroup::Write( const std::string variableName, const void* values )
{
    auto itVariable = m_Variables.find( variableName );

    if( m_DebugMode == true )
    {
        if( itVariable == m_Variables.end() )
            throw std::invalid_argument( "ERROR: variable " + variableName + " is undefined.\n" );
        //here implemented dynamic_cast checks
    }

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
    logStream << "\tIs Transport Method Unique?: " << std::boolalpha << m_Transport.unique() << "\n";
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


void CGroup::Close( )
{
    //Need to implement
}


void CGroup::CheckTransport( const std::string method )
{
    if( SSupport::Transports.count( method ) == 0 )
        throw std::invalid_argument( "ERROR: transport method " + method + " not supported. Check spelling or case sensitivity.\n" );

    if( m_ActiveTransport.empty() == false ) //there is an existing transport method
        m_Transport.reset();

    m_ActiveTransport = method;
}


} //end namespace
