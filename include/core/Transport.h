/*
 * Transport.h
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */

#ifndef TRANSPORT_H_
#define TRANSPORT_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
#include <vector>
/// \endcond

#ifdef ADIOS_NOMPI
  #include "mpidummy.h"
#else
  #include <mpi.h>
#endif



namespace adios
{

class Transport
{

public:

    const std::string m_Type; ///< transport type from derived class
    std::string m_Name; ///< from Open
    std::string m_AccessMode; ///< from Open
    bool m_IsOpen = false;

    MPI_Comm m_MPIComm = MPI_COMM_SELF;

    int m_RankMPI = 0; ///< current MPI rank process
    int m_SizeMPI = 1; ///< current MPI processes size

    /**
     * Base constructor that all derived classes pass
     * @param
     * @param mpiComm passed to m_MPIComm
     * @param debugMode passed to m_DebugMode
     */
    Transport( const std::string type, MPI_Comm mpiComm, const bool debugMode );


    virtual ~Transport( ); ///< empty destructor, using STL for memory management

    /**
     * Open Output file accesing a mode
     * @param name name of stream or file
     * @param accessMode r or read, w or write, a or append
     */
    virtual void Open( const std::string name, const std::string accessMode ) = 0;

    /**
     * Set buffer and size for a particular transport
     * @param buffer raw data buffer
     * @param size raw data buffer size
     */
    virtual void SetBuffer( char* buffer, std::size_t size );

    /**
     * Write function for a transport
     * @param buffer pointer to buffer to be written
     * @param size size of buffer to be written
     */
    virtual void Write( const char* buffer, std::size_t size ) = 0;

    virtual void Flush( ); ///< flushes current contents to physical medium without closing the transport

    virtual void Close( ); ///< closes current transport and flushes everything, transport becomes unreachable


protected:

    const bool m_DebugMode = false; ///< if true: additional checks and exceptions

};



} //end namespace



#endif /* TRANSPORT_H_ */
