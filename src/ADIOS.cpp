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


namespace adios
{

//here assign default values of non-primitives
ADIOS::ADIOS( )
{ }


ADIOS::ADIOS( const std::string xmlConfigFile ):
    m_XMLConfigFile{ xmlConfigFile }
{ }


//#ifdef USE_MPI
ADIOS::ADIOS( const std::string xmlConfigFile, const MPI_Comm& mpiComm  ):
    m_XMLConfigFile{ xmlConfigFile },
    m_IsUsingMPI{ true },
	m_MPIComm{ mpiComm }
{ }
//#endif


void ADIOS::Init( )
{
    if( m_IsUsingMPI == false && m_XMLConfigFile.empty() == false )
    {
        InitNoMPI( );
    }
    else
    {
        //#ifdef USE_MPI
        InitMPI( );
        //#endif
    }
}


void ADIOS::InitNoMPI( )
{
    ReadXMLConfigFile( );
}

//#ifdef USE_MPI
void ADIOS::InitMPI( )
{
    //here just say hello from MPI processes

    int size;
    MPI_Comm_size( *m_MPIComm, &size );

    int rank;
    MPI_Comm_rank( *m_MPIComm, &rank );

    std::cout << " Hello World from processor " << rank << "/" << size << "\n";
}
//#endif


void ADIOS::ReadXMLConfigFile( )
{
    std::ifstream xmlConfigStream( m_XMLConfigFile );

    if( xmlConfigStream.good() == false ) //check file
    {
        const std::string errorMessage( "XML Config file " + m_XMLConfigFile + " could not be opened. "
                                        "Check permissions or file existence\n");
        throw std::ios_base::failure( errorMessage );
    }
    //here fill SMetadata...




    xmlConfigStream.close();
}


} //end namespace



