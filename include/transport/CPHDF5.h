/*
 * CPHDF5.h
 *
 *  Created on: Oct 17, 2016
 *      Author: wfg
 */

#ifndef CPHDF5_H_
#define CPHDF5_H_

#include <hdf5.h>

#include "core/CTransport.h"


namespace adios
{


class CPHDF5 : public CTransport
{

public:

    CPHDF5( const unsigned int priority, const unsigned int iteration, MPI_Comm mpiComm );

    ~CPHDF5( );

};


} //end namespace



#endif /* CPHDF5_H_ */
