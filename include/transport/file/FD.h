/*
 * POSIXMPI.h
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */

#ifndef FD_H_
#define FD_H_


#include "core/Transport.h"


namespace adios
{
namespace transport
{

/**
 * File descriptor transport using the POSIX library
 */
class FD : public Transport
{

public:

    FD( MPI_Comm mpiComm, const bool debugMode );

    ~FD( );

    void Open( const std::string name, const std::string accessMode );

    void Write( const char* buffer, std::size_t size );

    void Close( );


private:

    int m_FileDescriptor = -1; ///< POSIX file descriptor
};




} //end namespace transport
} //end namespace
#endif /* FD_H_ */
