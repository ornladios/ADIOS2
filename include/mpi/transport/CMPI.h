/*
 * CMPI.h
 *
 *  Created on: Oct 17, 2016
 *      Author: wfg
 */

#ifndef CMPI_H_
#define CMPI_H_

#include "mpi/transport/CTransportMPI.h"


namespace adios
{

/**
 * Class that defines a transport method using MPI I/O API
 */
class CMPI : public CTransportMPI
{

public:

    CMPI( const unsigned int priority, const unsigned int iteration, MPI_Comm mpiComm );

    ~CMPI( );

    void Write( const CVariable& variable );
};


} //end namespace



#endif /* CMPI_H_ */
