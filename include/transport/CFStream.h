/*
 * CFStream.h
 *
 *  Created on: Oct 18, 2016
 *      Author: wfg
 */

#ifndef CFSTREAM_H_
#define CFSTREAM_H_


#include "core/CTransport.h"


namespace adios
{

/**
 * Class that defines a transport method using C++ file streams
 */
class CFStream : public CTransport
{

public:

    CFStream( const unsigned int priority, const unsigned int iteration, MPI_Comm mpiComm );

    ~CFStream( );

    void Write( const CVariable& variable );
};


} //end namespace



#endif /* CFSTREAM_H_ */
