/*
 * CPHDF5.h
 *
 *  Created on: Oct 17, 2016
 *      Author: wfg
 */

#ifndef CPHDF5_H_
#define CPHDF5_H_


#include <hdf5.h>

#include "mpi/transport/CTransportMPI.h"


namespace adios
{


class CPHDF5 : public CTransportMPI
{

public:

    CPHDF5( const unsigned int priority, const unsigned int iteration, MPI_Comm mpiComm );

    ~CPHDF5( );

    void Write( const CVariable& variable );
};


} //end namespace



#endif /* CPHDF5_H_ */
