/*
 * CCapsule.h
 *
 *  Created on: Nov 7, 2016
 *      Author: wfg
 */

#ifndef CCAPSULE_H_
#define CCAPSULE_H_


/// \cond EXCLUDE_FROM_DOXYGEN
#include <vector>
#include <string>
#include <memory>
#include <map>
/// \endcond

#ifdef HAVE_MPI
  #include <mpi.h>
#else
  #include "public/mpidummy.h"
#endif

#include "core/CGroup.h"
#include "core/SVariable.h"
#include "core/CTransform.h"
#include "core/CTransport.h"
#include "core/SStream.h"
#include "functions/CCapsuleTemplates.h"


namespace adios
{

/**
 * Base class for Capsule operations managing shared-memory, and buffer and variables transform and transport operations
 */
class CCapsule
{

public:

    #ifdef HAVE_MPI
    MPI_Comm m_MPIComm = NULL; ///< only used as reference to MPI communicator passed from parallel constructor, MPI_Comm is a pointer itself. Public as called from C
    #else
    MPI_Comm m_MPIComm = 0; ///< only used as reference to MPI communicator passed from parallel constructor, MPI_Comm is a pointer itself. Public as called from C
    #endif

    int m_RankMPI = 0; ///< current MPI rank process
    int m_SizeMPI = 1; ///< current MPI processes size

    std::map< std::string, std::shared_ptr<CTransport> > m_Transport;
    std::vector<char> Buffer;
    size_t m_MaxBufferSize = 0;
    std::string m_GroupName; ///< associated group to look for variable information

    /**
     *
     * @param streamName
     * @param accessMode
     * @param mpiComm
     * @param debugMode
     * @param arguments
     */
    CCapsule( const std::string streamName, const std::string accessMode, const MPI_Comm mpiComm,
              const bool debugMode, const std::vector<std::string>& arguments );



    CCapsule( const std::string transport, const size_t maxBufferSize, const MPI_Comm mpiComm, const bool debugMode ):
        MaxBufferSize{ maxBufferSize }
    {
        if( transport == "POSIX" )
            Transport = std::make_shared<CPOSIX>( mpiComm, debugMode );

        else if( transport == "FStream" )
            Transport = std::make_shared<CFStream>( mpiComm, debugMode );

        else if( transport == "DataMan" )
            Transport = std::make_shared<CDataMan>( mpiComm, debugMode );
    }

    CCapsule( ); ///< Default Empty constructor with ADIOS class

    /**
     * Debug mode
     * @param debugMode
     */
    CCapsule( const bool debugMode );

    /**
     * Unique constructor
     * @param mpiComm communicator passed from ADIOS
     * @param debugMode true: on throws exceptions and do additional checks, false: off (faster, but unsafe)
     */
    CCapsule( MPI_Comm mpiComm, const bool debugMode );


    ~CCapsule( );

    /**
     * Open streamName by assigning a buffer, maxBufferSize and a transport mode
     * @param streamName
     * @param accessMode
     * @param maxBufferSize
     * @param transport
     */



    void SetStreamGroup( const std::string streamName, const std::string group );

    /**
     * Writes raw data to m_Buffer
     * @param streamName key to get the corresponding buffer from m_Buffers
     * @param data pointer containing the data
     * @param size of data to be written
     */
    template<class T>
    void Write( const T* data, const size_t size, const unsigned int cores = 1 )
    {
        if( stream.Transport->m_Method == "DataMan" ) //CDataMan needs entire data in buffer
            stream.Buffer.resize( size * sizeof(T) ); //resize buffer to fit all data

        WriteToBuffer( data, size, stream, cores );
    }

    /**
     * Closes a certain stream at the transport level
     * @param streamName passed to corresponding transport so it can be closed.
     */
    void Close( const std::string streamName );

};


} //end namespace

#endif /* CCAPSULE_H_ */
