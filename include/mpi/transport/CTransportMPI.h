/*
 * CTransportMPI.h
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */

#ifndef CTRANSPORTMPI_H_
#define CTRANSPORTMPI_H_

#include <mpi.h>

#include "CTransport.h"


namespace adios
{

class CTransportMPI : public CTransport
{

public:

    MPI_Comm m_MPIComm;

    CTransportMPI( const std::string method, const unsigned int priority, const unsigned int iteration,
                   MPI_Comm mpiComm  ):
        CTransport( method, priority, iteration, true ),
        m_MPIComm( mpiComm )
    { }

    virtual ~CTransportMPI( )
    { }

    virtual void Write( CVariable& variable ) = 0;
};


} //end namespace



#endif /* CTRANSPORTMPI_H_ */
