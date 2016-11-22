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

    const std::string m_Method; ///< transport method name
    #ifdef HAVE_MPI
    MPI_Comm m_MPIComm = NULL; ///< only used as reference to MPI communicator passed from parallel constructor, MPI_Comm is a pointer itself. Public as called from C
    #else
    MPI_Comm m_MPIComm = 0; ///< only used as reference to MPI communicator passed from parallel constructor, MPI_Comm is a pointer itself. Public as called from C
    #endif

    //const unsigned int m_Iterations; ///< iteration number for this transport
    const bool m_DebugMode; ///< if true: additional checks and exceptions

    int m_MPIRank = 0; ///< current MPI rank process
    int m_MPISize = 1; ///< current MPI processes size


    /**
     * Base constructor that all derived classes pass
     * @param mpiComm passed to m_MPIComm
     * @param debugMode passed to m_DebugMode
     */
    CTransport( const std::string method, MPI_Comm mpiComm, const bool debugMode ):
        m_Method{ method },
        m_MPIComm{ mpiComm },
        m_DebugMode{ debugMode }
    {
        MPI_Comm_rank( m_MPIComm, &m_MPIRank );
        MPI_Comm_size( m_MPIComm, &m_MPISize );
    }


    virtual ~CTransport( )
    { }

    /**
     * Open Output file accesing a mode
     * @param streamName name of file
     * @param accessMode r or read, w or write, a or append
     */
    virtual void Open( const std::string streamName, const std::string accessMode ) = 0;

    /**
     * Sets the buffer and bufferSize for certain transport methods
     * @param buffer to be set to transport
     */
    virtual void SetBuffer( const std::vector<char>& buffer )
    { };

    virtual void Write( std::vector<char>& buffer )
    { };

    virtual void Close( ) = 0; //here think what needs to be passed

    virtual void Finalize( )
    { };

};



} //end namespace



#endif /* CTRANSPORT_H_ */
