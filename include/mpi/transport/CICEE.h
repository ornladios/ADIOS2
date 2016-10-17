/*
 * CICEEMPI.h
 *
 *  Created on: Oct 17, 2016
 *      Author: wfg
 */

#ifndef CICEE_H_
#define CICEE_H_

#include "mpi/transport/CTransportMPI.h"


namespace adios
{


class CICEE : public CTransportMPI
{

public:

    CICEE( const unsigned int priority, const unsigned int iteration, MPI_Comm mpiComm );

    ~CICEE( );

    void Write( const CVariable& variable );
};


} //end namespace




#endif /* CICEE_H_ */
