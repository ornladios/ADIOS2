/*
 * CTransport.h
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */

#ifndef CTRANSPORT_H_
#define CTRANSPORT_H_

#include <string>

#include "CVariable.h"


namespace adios
{

class CTransport
{

public:

    const std::string m_Method;
    const unsigned int m_Priority;
    const unsigned int m_Iteration;
    const bool m_IsUsingMPI;

    CTransport( const std::string method, const unsigned int priority, const unsigned int iteration,
                bool isUsingMPI ):
        m_Method( method ),
        m_Priority( priority ),
        m_Iteration( iteration ),
        m_IsUsingMPI( isUsingMPI )
    { }

    virtual ~CTransport( )
    { }

    virtual void Write( CVariable& variable ) = 0;
};



} //end namespace



#endif /* CTRANSPORT_H_ */
