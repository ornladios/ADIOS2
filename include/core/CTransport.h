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

    const unsigned int m_Priority; ///< writing priority for this transport
    const unsigned int m_Iteration; ///< iteration number for this transport
    MPI_Comm m_MPIComm; ///< passed MPI communicator
    const bool m_DebugMode; ///< if true: additional checks and exceptions

    std::string m_StreamName; ///< file or stream name associated with a group that owns the transport
    //This part should go to a capsule ??
    int m_RankMPI = 0; ///< current MPI rank process
    int m_SizeMPI = 1; ///< current MPI processes size

    /**
     * Base constructor that all derived classes pass
     * @param method passed to m_Method
     * @param priority passed to m_Priority
     * @param iteration passed to m_Iteration
     * @param mpiComm passed to m_MPIComm
     * @param debugMode passed to m_DebugMode
     */
    CTransport( const std::string method, const unsigned int priority, const unsigned int iteration,
                MPI_Comm mpiComm, const bool debugMode ):
        m_Priority( priority ),
        m_Iteration( iteration ),
        m_MPIComm( mpiComm ),
        m_DebugMode( debugMode )
    {
        MPI_Comm_rank( m_MPIComm, &m_RankMPI );
        MPI_Comm_size( m_MPIComm, &m_SizeMPI );
    }


    virtual ~CTransport( )
    { }

    /**
     * Open Output file accesing a mode
     * @param fileName name of file
     * @param accessMode r or read, w or write, a or append
     */
    virtual void Open( const std::string outputName, const std::string accessMode ) = 0;

    virtual void Close( ) = 0; //here think what needs to be passed

    virtual void Finalize( )
    { };

};



} //end namespace



#endif /* CTRANSPORT_H_ */
