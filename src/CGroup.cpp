/*
 * CGroup.cpp
 *
 *  Created on: Oct 12, 2016
 *      Author: wfg
 */

/// \cond EXCLUDED_FROM_DOXYGEN
#include <iostream>
/// \endcond
///
#include "CGroup.h"
#include "ADIOSFunctions.h"
#include "SSupport.h"

//MPI transport methods
#ifdef HAVE_MPI
#include "mpi/transport/CPOSIX.h"
#endif

#include "nompi/transport/CPOSIXNoMPI.h"


namespace adios
{

CGroup::CGroup( )
{ }

CGroup::CGroup( const std::string& xmlGroup, std::string& groupName )
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
    auto itVariable = m_Variables.find( name );
    if( itVariable == m_Variables.end() ) //name is not found
    {
        if( type == "integer") //use copy constructor as it's a small struct
            m_Variables[name] = std::make_shared< CVariableTemplate<int> >( isGlobal, type, dimensionsCSV, transform );
        else if( type == "unsigned integer")
            m_Variables[name] = std::make_shared< CVariableTemplate<unsigned int> >( isGlobal, type, dimensionsCSV, transform );
        else if( type == "real")
            m_Variables[name] = std::make_shared< CVariableTemplate<float> >( isGlobal, type, dimensionsCSV, transform );
        else if( type == "double")
            m_Variables[name] = std::make_shared< CVariableTemplate<double> >( isGlobal, type, dimensionsCSV, transform );
    }
    else //name is found
    {
        throw std::invalid_argument( "ERROR: variable " + name + " is defined twice\n" );
    }
}

void CGroup::SetAttribute( const std::string name, const bool isGlobal, const std::string type, const std::string path, const std::string value )
{
    m_Attributes.push_back( SAttribute( name, isGlobal, type, path, value ) );
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

void CGroup::SetTransport( const std::string method, const unsigned int priority, const unsigned int iteration )
{
    CheckTransport( method );
    if( m_ActiveTransport == "POSIX" ) m_Transport = std::make_shared<CPOSIXNoMPI>( method, priority, iteration );
}

#ifdef HAVE_MPI
void CGroup::SetTransport( const std::string method, const unsigned int priority, const unsigned int iteration,
                           const MPI_Comm mpiComm )
{
    CheckTransport( method );
    if( m_ActiveTransport == "POSIX" ) m_Transport = std::make_shared<CPOSIX>( method, priority, iteration, mpiComm );
}
#endif



//PRIVATE FUNCTIONS BELOW
void CGroup::Monitor( std::ostream& logStream ) const
{
    logStream << "\tVariable \t Type\n";
    for( auto& variablePair : m_Variables )
    {
        logStream << "\t" << variablePair.first << " \t " << variablePair.second->m_Type << "\n";
    }

    logStream << "\tAttribute \t Type \t Value \n";
    for( auto& attribute : m_Attributes )
    {
        logStream << "\t" << attribute.Name << " \t " << attribute.Type << " \t " << attribute.Value << "\n";
    }


    logStream << "\tTransport Method " << m_ActiveTransport << "\n";
    logStream << "\tIs Transport Method Unique?: " << std::boolalpha << m_Transport.unique() << "\n";
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
    if( c_Transports.count( method ) == 0 )
        throw std::invalid_argument( "ERROR: transport method " + method + " not supported. Check spelling or case sensitivity.\n" );

    if( m_ActiveTransport.empty() == false ) //there is an existing transport method
        m_Transport.reset();

    m_ActiveTransport = method;
}


} //end namespace
