/*
 * CADIOS.cpp
 *
 *  Created on: Sep 29, 2016
 *      Author: William F Godoy
 *
 */

#include <fstream>

#include "include/CADIOS.h"


namespace adios
{

//here assign default values of non-primitives
CADIOS::CADIOS( )
{ }


CADIOS::CADIOS( const std::string xmlConfigFile ):
    m_XMLConfigFile( xmlConfigFile )
{ }


CADIOS::CADIOS( const std::string xmlConfigFile, const MPI_Comm& mpiComm  ):
    m_XMLConfigFile( xmlConfigFile ),
    m_IsUsingMPI( true ),
	m_MPIComm( mpiComm )
{ }


void CADIOS::Init( )
{
    if( m_IsUsingMPI == false )
    {
        InitSerial( );
    }
    else
    {
        InitMPI( );
    }
}


void CADIOS::InitSerial( )
{
    ReadXMLConfigFile( );
}


void CADIOS::InitMPI( )
{

}


void CADIOS::ReadXMLConfigFile( )
{
    std::ifstream xmlConfigStream( m_XMLConfigFile );

    if( xmlConfigStream.good() == false ) //check file
    {
        const std::string errorMessage( "XML Config file " + m_XMLConfigFile + " could not be opened. "
                                        "Check permissions or file existence\n");
        throw std::ios_base::failure( errorMessage );
    }




}









}



