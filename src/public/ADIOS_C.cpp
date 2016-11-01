/*
 * ADIOS_C.cpp
 *
 *  Created on: Oct 31, 2016
 *      Author: wfg
 */

#include <string>
#include <fstream>
#include <iostream>
#include <cstring>

#include "public/ADIOS_C.h"
#include "public/ADIOS.h"


#ifdef __cplusplus
extern "C"
{
#endif


ADIOS* adios_init( const char* xmlConfigFile, const MPI_Comm mpiComm )
{
    adios::ADIOS* adios;
    int rank;
    MPI_Comm_rank( mpiComm, &rank );

    try
    {
        adios = new adios::ADIOS( std::string( xmlConfigFile ), mpiComm );
    }
    catch( std::bad_alloc& e )
    {
        if( rank == 0 )
            std::cout << "Bad allocation exception, STOPPING PROGRAM\n" << e.what() << "\n";
    }
    catch( std::invalid_argument& e )
    {
        if( rank == 0 )
            std::cout << "Invalid argument exception, STOPPING PROGRAM\n" << e.what() << "\n";
    }
    catch( std::exception& e )
    {
        if( rank == 0 )
            std::cout << "Exception, STOPPING PROGRAM\n" << e.what() << "\n";
    }

    return ( ADIOS* ) ( adios );
}


ADIOS* adios_init_debug( const char* xmlConfigFile, const MPI_Comm mpiComm )
{
    adios::ADIOS* adios;
    int rank;
    MPI_Comm_rank( mpiComm, &rank );

    try
    {
        adios = new adios::ADIOS( std::string( xmlConfigFile ), mpiComm, true );
    }
    catch( std::bad_alloc& e )
    {
        if( rank == 0 )
            std::cout << "Bad allocation exception, STOPPING PROGRAM\n" << e.what() << "\n";
    }
    catch( std::invalid_argument& e )
    {
        if( rank == 0 )
            std::cout << "Invalid argument exception, STOPPING PROGRAM\n" << e.what() << "\n";
    }
    catch( std::exception& e )
    {
        if( rank == 0 )
            std::cout << "Exception, STOPPING PROGRAM\n" << e.what() << "\n";
    }

    return ( ADIOS* ) ( adios );
}


void adios_open( const ADIOS* adiosC, const char* groupName, const char* fileName, const char* accessMode )
{
    adios::ADIOS* adios = ( adios::ADIOS* ) adiosC;
    int rank;
    MPI_Comm_rank( adios->m_MPIComm, &rank );

    try
    {
        adios::ADIOS* adios = ( adios::ADIOS * ) adiosC;
        adios->Open( std::string( groupName ), std::string( fileName ), std::string( accessMode ) );
    }
    catch( std::bad_alloc& e )
    {
        if( rank == 0 )
            std::cout << "Bad allocation exception, STOPPING PROGRAM\n" << e.what() << "\n";
    }
    catch( std::invalid_argument& e )
    {
        if( rank == 0 )
            std::cout << "Invalid argument exception, STOPPING PROGRAM\n" << e.what() << "\n";
    }
    catch( std::exception& e )
    {
        if( rank == 0 )
            std::cout << "Exception, STOPPING PROGRAM\n" << e.what() << "\n";
    }
}


void adios_write( const ADIOS* adiosC, const char* groupName, const char* variableName, const void* values  )
{
    adios::ADIOS* adios = ( adios::ADIOS* ) adiosC;
    int rank;
    MPI_Comm_rank( adios->m_MPIComm, &rank );

    try
    {
        adios->Write( std::string( groupName ), std::string( variableName ), values );
    }
    catch( std::bad_alloc& e )
    {
        if( rank == 0 )
            std::cout << "Bad allocation exception, STOPPING PROGRAM\n" << e.what() << "\n";
    }
    catch( std::invalid_argument& e )
    {
        if( rank == 0 )
            std::cout << "Invalid argument exception, STOPPING PROGRAM\n" << e.what() << "\n";
    }
    catch( std::exception& e )
    {
        if( rank == 0 )
            std::cout << "Exception, STOPPING PROGRAM\n" << e.what() << "\n";
    }
}


void adios_close( const ADIOS* adiosC, const char* groupName )
{
    adios::ADIOS* adios = ( adios::ADIOS * ) adiosC;
    int rank;
    MPI_Comm_rank( adios->m_MPIComm, &rank );

    try
    {
        adios->Close( std::string( groupName ) );
    }
    catch( std::bad_alloc& e )
    {
        if( rank == 0 )
            std::cout << "Bad allocation exception, STOPPING PROGRAM\n" << e.what() << "\n";
    }
    catch( std::invalid_argument& e )
    {
        if( rank == 0 )
            std::cout << "Invalid argument exception, STOPPING PROGRAM\n" << e.what() << "\n";
    }
    catch( std::exception& e )
    {
        if( rank == 0 )
            std::cout << "Exception, STOPPING PROGRAM\n" << e.what() << "\n";
    }
}


void adios_free( const ADIOS* adiosC )
{
    adios::ADIOS* adios = ( adios::ADIOS* ) adiosC;
    delete adios;
}


void adios_monitor_groups( const ADIOS* adiosC )
{
    adios::ADIOS* adios = ( adios::ADIOS* ) adiosC;
    int rank;
    MPI_Comm_rank( adios->m_MPIComm, &rank );

    try
    {
        adios->MonitorGroups( std::cout );
    }
    catch( std::bad_alloc& e )
    {
        if( rank == 0 )
            std::cout << "Bad allocation exception, STOPPING PROGRAM\n" << e.what() << "\n";
    }
    catch( std::invalid_argument& e )
    {
        if( rank == 0 )
            std::cout << "Invalid argument exception, STOPPING PROGRAM\n" << e.what() << "\n";
    }
    catch( std::exception& e )
    {
        if( rank == 0 )
            std::cout << "Exception, STOPPING PROGRAM\n" << e.what() << "\n";
    }
}


void adios_monitor_groups_file( const ADIOS* adiosC, const char* fileName, const char* mode )
{
    adios::ADIOS* adios = ( adios::ADIOS * ) adiosC;
    int rank;
    MPI_Comm_rank( adios->m_MPIComm, &rank );

    try
    {
        std::ofstream fileStream;

        if( strcmp( mode, "a" ) == 0 || strcmp( mode, "append" ) == 0 )
            fileStream.open( fileName, std::ostream::app );
        else if( strcmp( mode, "w" ) == 0 || strcmp( mode, "write" ) == 0 )
            fileStream.open( fileName );

        adios->MonitorGroups( fileStream );

    }
    catch( std::bad_alloc& e )
    {
        if( rank == 0 )
            std::cout << "Bad allocation exception, STOPPING PROGRAM\n" << e.what() << "\n";
    }
    catch( std::invalid_argument& e )
    {
        if( rank == 0 )
            std::cout << "Invalid argument exception, STOPPING PROGRAM\n" << e.what() << "\n";
    }
    catch( std::exception& e )
    {
        if( rank == 0 )
            std::cout << "Exception, STOPPING PROGRAM\n" << e.what() << "\n";
    }
}


#ifdef __cplusplus
} //end extern C
#endif
