/*
 * CMPILustre.h
 *
 *  Created on: Oct 17, 2016
 *      Author: wfg
 */

#ifndef CMPILUSTRE_H_
#define CMPILUSTRE_H_


#include "core/CTransport.h"


namespace adios
{


class CMPILustre : public CTransport
{

public:

    CMPILustre( const unsigned int priority, const unsigned int iteration, MPI_Comm mpiComm );

    ~CMPILustre( );

    void Write( const CVariable& variable );
};


} //end namespace


#endif /* CMPILUSTRE_H_ */
