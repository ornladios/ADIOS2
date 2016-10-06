/*
 * CTransportNoMPI.h
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */

#ifndef CTRANSPORTNOMPI_H_
#define CTRANSPORTNOMPI_H_

#include "CTransport.h"


namespace adios
{

class CTransportNoMPI : public CTransport
{

public:

    CTransportNoMPI( const std::string method, const unsigned int priority, const unsigned int iteration ):
        CTransport( method, priority, iteration, false )
    { }

    virtual ~CTransportNoMPI( )
    { }

    virtual void Write( CVariable& variable ) = 0;
};


} //end namespace




#endif /* CTRANSPORTNOMPI_H_ */
