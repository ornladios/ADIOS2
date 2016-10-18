/*
 * CDataspacesMPI.h
 *
 *  Created on: Oct 17, 2016
 *      Author: wfg
 */

#ifndef CDATASPACES_H_
#define CDATASPACES_H_


#include "core/CTransport.h"


namespace adios
{


class CDataspaces : public CTransport
{

public:

    CDataspaces( const unsigned int priority, const unsigned int iteration, MPI_Comm mpiComm );

    ~CDataspaces( );

    void Write( const CVariable& variable );
};


} //end namespace




#endif /* CDATASPACES_H_ */
