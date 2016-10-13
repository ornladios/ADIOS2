/*
 * CPOSIXMPI.h
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */

#ifndef CPOSIXMPI_H_
#define CPOSIXMPI_H_


#include <string>
#include <mpi.h>

#include "mpi/transport/CTransportMPI.h"


namespace adios
{


class CPOSIXMPI : public CTransportMPI
{

public:

    CPOSIXMPI( const std::string method, const unsigned int priority, const unsigned int iteration,
               MPI_Comm mpiComm );

    ~CPOSIXMPI( );

    void Write( const CVariable& variable );

};


} //end namespace






#endif /* CPOSIXMPI_H_ */
