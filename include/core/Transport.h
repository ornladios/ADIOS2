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

#ifdef HAVE_MPI
    #include <mpi.h>
#else
    #include "mpidummy.h"
#endif

#include "core/Capsule.h"


namespace adios
{

class Transport
{

public:

    const std::string m_Type; ///< transport type from derived class
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
     * @param
     * @param mpiComm passed to m_MPIComm
     * @param debugMode passed to m_DebugMode
     */
    Transport( const std::string type, MPI_Comm mpiComm, const bool debugMode );


    virtual ~Transport( ); ///< empty destructor, using STL for memory management

    /**
     * Open Output file accesing a mode
     * @param streamName name of stream or file
     * @param accessMode r or read, w or write, a or append
     */
    virtual void Open( const std::string streamName, const std::string accessMode ) = 0;

    /**
     * Write function for a transport, only called if required
     * @param buffer
     */
    virtual void Write( const Capsule& capsule );


    virtual void Close( ) = 0; ///< closes current transport and flushes everything, can't be reachable after this call


protected:

    /**
     * Initialize particular derived transport class members
     * @param arguments particular transport arguments from ADIOS Open variadic function
     */
    virtual void Init( const std::vector<std::string>& arguments );

};



} //end namespace



#endif /* TRANSPORT_H_ */
