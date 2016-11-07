/*
 * CICEEMPI.h
 *
 *  Created on: Oct 17, 2016
 *      Author: wfg
 */

#ifndef CICEE_H_
#define CICEE_H_

#include "core/CTransport.h"


namespace adios
{


class CICEE : public CTransport
{

public:

    CICEE( const unsigned int priority, const unsigned int iteration, MPI_Comm mpiComm );

    ~CICEE( );

};


} //end namespace




#endif /* CICEE_H_ */
