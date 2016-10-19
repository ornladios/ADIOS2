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

#ifdef HAVE_MPI
    #include <mpi.h>
#else
    #include "public/mpidummy.h"
#endif
/// \endcond

#include "core/CVariable.h"


namespace adios
{

class CTransport
{

public:

    const std::string m_Method;  ///< method name, must be defined in SSupport.h TransportMethods
    const unsigned int m_Priority;
    const unsigned int m_Iteration;
    MPI_Comm m_MPIComm;

    std::string m_FileName; ///< file name associated with a group that owns the transport
    std::string m_AccessMode; ///< read, write, append

    CTransport( const std::string method, const unsigned int priority, const unsigned int iteration,
                MPI_Comm mpiComm ):
        m_Method( method ),
        m_Priority( priority ),
        m_Iteration( iteration ),
        m_MPIComm( mpiComm )
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
