/*
 * CVarMerge.h
 *
 *  Created on: Oct 17, 2016
 *      Author: wfg
 */

#ifndef CVARMERGE_H_
#define CVARMERGE_H_


#include "core/CTransport.h"


namespace adios
{


class CVarMerge : public CTransport
{

public:

    CVarMerge( const unsigned int priority, const unsigned int iteration, MPI_Comm mpiComm );

    ~CVarMerge( );

    void Write( const CVariable& variable );

};


} //end namespace


#endif /* CVARMERGE_H_ */
