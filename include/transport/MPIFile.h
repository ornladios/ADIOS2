/*
 * MPIFile.h
 *
 *  Created on: Jan 5, 2017
 *      Author: wfg
 */

#ifndef MPIFILE_H_
#define MPIFILE_H_

#include <mpi.h>


namespace adios
{

/**
 * Class that defines a transport method using C++ file streams
 */
class MPIFile : public Transport
{

public:

    MPIFile( MPI_Comm mpiComm, const bool debugMode, const std::vector<std::string>& arguments );

    ~MPIFile( );

    void Open( const std::string streamName, const std::string accessMode );

    void SetBuffer( char* buffer, std::size_t size );

    void Write( const char* buffer, std::size_t size );

    void Flush( );

    void Close( );


private:

    MPI_File m_MPIFile; ///< MPI File

};




} //end namespace





#endif /* MPIFILE_H_ */
