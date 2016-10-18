/*
 * CMPI.h
 *
 *  Created on: Oct 17, 2016
 *      Author: wfg
 */

#ifndef CMPI_H_
#define CMPI_H_

#include "core/CTransport.h"


namespace adios
{

/**
 * Transport method using MPI I/O API
 */
class CMPI : public CTransport
{

public:

    CMPI( const unsigned int priority, const unsigned int iteration, MPI_Comm mpiComm );

    ~CMPI( );

    void Write( const CVariable& variable );
};


} //end namespace



#endif /* CMPI_H_ */
