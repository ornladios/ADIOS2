/*
 * CPOSIXMPI.h
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */

#ifndef CPOSIX_H_
#define CPOSIX_H_


#include "core/CTransport.h"


namespace adios
{


class CPOSIX : public CTransport
{

public:

    CPOSIX( const unsigned int priority, const unsigned int iteration, MPI_Comm mpiComm );

    ~CPOSIX( );

    void Write( const CVariable& variable );

};


} //end namespace






#endif /* CPOSIX_H_ */
