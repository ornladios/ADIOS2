/*
 * ADIOS.cpp
 *
 *  Created on: Sep 29, 2016
 *      Author: William F Godoy
 *
 */

#include <fstream>
#include <iostream>

#include "ADIOS.h"

#ifdef HAVE_MPI
#include "file_mpi/CPOSIXMPI.h"
#endif

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
    ReadXMLConfigFile( );
}

#ifdef MPI_VERSION
void ADIOS::InitMPI( )
{
    //Use POSIXMPI for now for testing purposes
    m_File = std::unique_ptr<CPOSIXMPI> ( new CPOSIXMPI( "HelloFile", m_Metadata, m_MPIComm ) );
    m_File->Open( "HelloFile", "MyGroup", "a" );
}
#endif


void ADIOS::ReadXMLConfigFile( )
{
    std::cout << "Reading XML Config File " << m_XMLConfigFile << "\n";
    std::ifstream xmlConfigStream( m_XMLConfigFile );

    if( xmlConfigStream.good() == false ) //check file
    {
        xmlConfigStream.close();
        const std::string errorMessage( "ERROR: XML Config file " + m_XMLConfigFile + " could not be opened. "
                                        "Check permissions or file existence\n");
        throw std::ios_base::failure( errorMessage );
    }
    //here fill SMetadata...

    xmlConfigStream.close();
}


} //end namespace



