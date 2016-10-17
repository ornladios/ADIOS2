/*
 * CTransport.h
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */

#ifndef CTRANSPORT_H_
#define CTRANSPORT_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
/// \endcond

#include "CVariable.h"


namespace adios
{

class CTransport
{

public:

    const std::string m_Method;  ///< method name, must be defined in SSupport.h TransportMethods
    const unsigned int m_Priority;
    const unsigned int m_Iteration;
    const bool m_IsUsingMPI;
    std::string m_FileName;
    std::string m_AccessMode;

    CTransport( const std::string method, const unsigned int priority, const unsigned int iteration,
                bool isUsingMPI ):
        m_Method( method ),
        m_Priority( priority ),
        m_Iteration( iteration ),
        m_IsUsingMPI( isUsingMPI )
    { }

    virtual ~CTransport( )
    { }


    void Open( const std::string fileName, const std::string accessMode = "w" )
    {
        m_FileName = fileName;
        m_AccessMode = accessMode;
    }

    virtual void Write( const CVariable& variable ) = 0;
};



} //end namespace



#endif /* CTRANSPORT_H_ */
