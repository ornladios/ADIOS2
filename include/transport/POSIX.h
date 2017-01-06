/*
 * POSIXMPI.h
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */

#ifndef POSIX_H_
#define POSIX_H_


#include "core/Transport.h"


namespace adios
{


class POSIX : public Transport
{

public:

    POSIX( MPI_Comm mpiComm, const bool debugMode );

    ~POSIX( );

    void Open( const std::string name, const std::string accessMode );

    void Write( const char* buffer, std::size_t size );

    void Close( );


private:

    int m_FileDescriptor = -1; ///< POSIX file descriptor
};


} //end namespace


#endif /* POSIX_H_ */
