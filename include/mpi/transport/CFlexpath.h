/*
 * CFlexpath.h
 *
 *  Created on: Oct 17, 2016
 *      Author: wfg
 */

#ifndef CFLEXPATH_H_
#define CFLEXPATH_H_

#include "mpi/transport/CTransportMPI.h"


namespace adios
{


class CFlexpath : public CTransportMPI
{

public:

    CFlexpath( const unsigned int priority, const unsigned int iteration, MPI_Comm mpiComm );

    ~CFlexpath( );

    void Write( const CVariable& variable );
};


} //end namespace



#endif /* CFLEXPATH_H_ */
