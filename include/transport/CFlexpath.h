/*
 * CFlexpath.h
 *
 *  Created on: Oct 17, 2016
 *      Author: wfg
 */

#ifndef CFLEXPATH_H_
#define CFLEXPATH_H_

#include "core/CTransport.h"


namespace adios
{


class CFlexpath : public CTransport
{

public:

    CFlexpath( const unsigned int priority, const unsigned int iteration, MPI_Comm mpiComm );

    ~CFlexpath( );

};


} //end namespace



#endif /* CFLEXPATH_H_ */
