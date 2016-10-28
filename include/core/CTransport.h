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

#ifdef HAVE_MPI
    #include <mpi.h>
#else
    #include "public/mpidummy.h"
#endif


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

    CTransport( const std::string method, const unsigned int priority, const unsigned int iteration,
                MPI_Comm mpiComm ):
        m_Method( method ),
        m_Priority( priority ),
        m_Iteration( iteration ),
        m_MPIComm( mpiComm )
    { }

    virtual ~CTransport( )
    { }


    /**
     * Open Output file accesing a mode
     * @param fileName name of file
     * @param accessMode r or read, w or write, a or append
     */
    virtual void Open( const std::string fileName, const std::string accessMode ) = 0;

    virtual void Close( ) = 0; //here think what needs to be passed
};



} //end namespace



#endif /* CTRANSPORT_H_ */
