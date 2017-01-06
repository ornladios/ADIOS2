/*
 * File.h
 *
 *  Created on: Jan 6, 2017
 *      Author: wfg
 */

#ifndef FILE_H_
#define FILE_H_


#include <stdio.h> // FILE*

#include "core/Transport.h"


namespace adios
{

/**
 * Class that defines a transport method using C file pointer to streams FILE*
 */
class File : public Transport
{

public:

    File( MPI_Comm mpiComm, const bool debugMode );

    ~File( );

    void Open( const std::string name, const std::string accessMode );

    void SetBuffer( char* buffer, std::size_t size );

    void Write( const char* buffer, std::size_t size );

    void Flush( );

    void Close( );


private:

    FILE* m_File = NULL; ///< C file pointer

};


} //end namespace




#endif /* FILE_H_ */
