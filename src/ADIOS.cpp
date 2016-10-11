/*
 * ADIOS.cpp
 *
 *  Created on: Sep 29, 2016
 *      Author: William F Godoy
 *
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>

#include "ADIOS.h"
#include "ADIOSFunctions.h"


namespace adios
{

//here assign default values of non-primitives
ADIOS::ADIOS( )
{ }


ADIOS::ADIOS( const std::string xmlConfigFile ):
    m_XMLConfigFile{ xmlConfigFile }
{ }


#ifdef HAVE_MPI
ADIOS::ADIOS( const std::string xmlConfigFile, const MPI_Comm mpiComm  ):
    m_XMLConfigFile{ xmlConfigFile },
    m_IsUsingMPI{ true },
	m_MPIComm{ mpiComm }
{ }
#endif

ADIOS::~ADIOS( )
{ }


void ADIOS::Init( )
{
    std::cout << "Just testing the Init Function\n";

    if( m_IsUsingMPI == false && m_XMLConfigFile.empty() == false )
    {
        InitNoMPI( );
    }
    else
    {
        #ifdef HAVE_MPI
        InitMPI( );
        #endif
    }
}


void ADIOS::InitNoMPI( )
{
    std::string xmlFileContent;
    DumpFileToStream( m_XMLConfigFile, xmlFileContent ); //in ADIOSFunctions.h dumps all XML Config File to xmlFileContent
    SetMembers( xmlFileContent, m_HostLanguage,  m_Groups );

    //SetMembersFromXMLConfigFile( xmlFileContent, m_HostLanguage,  m_Groups ); //in ADIOSFunctions.h sets m_HostLanguage and m_Groups
    //std::cout << "Finishing Initialization";
}

#ifdef MPI_VERSION
void ADIOS::InitMPI( )
{
    std::cout << "Just testing the InitMPI Function\n";
}
#endif


void ADIOS::Open( const std::string groupName, const std::string fileName, const std::string accessMode )
{
    //retrieve a group name from m_Groups
    std::cout << "Just testing the Open function\n";
}


template<class T> void ADIOS::Write( const std::string groupName, const std::string variableName, const T* values )
{
    std::cout << "Just testing the Write function\n";
}





} //end namespace



