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

    std::string m_CurrentGroup; ///< associated group to look for variable information

    CCapsule( ); ///< Default Empty constructor used with ADIOS class empty constructor

    /**
     * Multiple argument constructor
     * @param accessMode
     * @param mpiComm
     * @param debugMode
     */

    CCapsule( const MPI_Comm mpiComm, const bool debugMode, const std::string streamName, const std::string accessMode,
              const std::string transport, const std::vector<std::string>& arguments );

    ~CCapsule( );

    /**
     * Add a transport to a stream
     * @param streamName
     * @param accessMode
     * @param isDefault
     * @param transport
     * @param arguments
     * @return transport index
     */
    int AddTransport( const std::string streamName, const std::string accessMode, const bool isDefault,
                      const std::string transport, const std::vector<std::string>& arguments );

    /**
     * Writes raw data to m_Buffer
     * @param data pointer containing the data
     * @param size of data to be written
     * @param transportIndex calls the Write function to this transport only, if -1 (default), call all transports
     */
    template<class T>
    void Write( const T* data, const size_t size, int transportIndex )
    {
        if( m_DebugMode == true )
        {
            if( transportIndex >= m_Transports.size() )
                throw std::invalid_argument( "ERROR: transport index " + std::to_string( transportIndex ) +
                                              " does not point to a transport method in call to Write\n" );
        }
        WriteToBuffer( data, size, transportIndex, m_Transports, m_MaxBufferSize, m_Buffer );
    }

    void Close( int transportIndex ); ///< Closes current Transport


private:

    std::vector< std::shared_ptr<CTransport> > m_Transports;
    std::string m_AccessMode;
    std::vector<char> m_Buffer;
    size_t m_MaxBufferSize = 0;
    const bool m_DebugMode = false;
    std::string GetName( const std::vector<std::string>& arguments ) const;
};


} //end namespace

#endif /* CCAPSULE_H_ */
