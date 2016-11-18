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

    const bool m_DebugMode = false;

    std::map< std::string, std::vector<unsigned char> > m_Buffers; ///< buffer to be managed, key is the streamName
    std::map< std::string, size_t > m_MaxBufferSize; ///< key is the streamName, value is the maximum buffer size
    std::map< std::string, std::shared_ptr<CTransport> > m_Transports; ///< transport associated with ADIOS run, key is streamName

    std::map< std::string, std::shared_ptr<CTransform> > m_Transforms; ///< transforms associated with ADIOS run, key is transform name

    ///Maybe add a communication class object

    /**
     * Empty constructor
     */
    CCapsule( );

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
    void Open( const std::string streamName, const std::string accessMode,
               const size_t maxBufferSize, const std::string transport );

    /**
     * Writes raw data to m_Buffer
     * @param streamName key to get the corresponding buffer from m_Buffers
     * @param data pointer containing the data
     * @param size of data to be written
     */
    template<class T>
    void Write( const std::string streamName, const T* data, const size_t size, const unsigned int cores )
    {
        auto itBuffer = m_Buffers.find( streamName );
        auto itTransport = m_Transports.find( streamName );

        if( m_DebugMode == true )
        {
            if( itBuffer == m_Buffers.end() )
                throw std::invalid_argument( "ERROR: stream (file name ) " + streamName + " has not been declared" );
        }

        if( itTransport->second->m_Method == "DataMan" ) //CDataMan needs entire data in buffer
            itBuffer->second.resize( size * sizeof(T) ); //resize buffer to fit all data

        //WriteToBuffer( data, size, itBuffer->second,  );
    }

    /**
     * Closes a certain stream at the transport level
     * @param streamName passed to corresponding transport so it can be closed.
     */
    void Close( const std::string streamName );


private:

    void CreateTransport( const std::string streamName, const std::string transport );

    void CreateBuffer( const std::string streamName, const size_t maxBufferSize );

    void CreateTransform( const std::string transform );

};


} //end namespace

#endif /* CCAPSULE_H_ */
