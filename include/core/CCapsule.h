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

    std::map< std::string, std::vector<char> > m_Buffers; ///< buffer to be managed, key is the streamName
    std::map< std::string, std::shared_ptr<CTransport> > m_Transports; ///< transport associated with ADIOS run

    std::map< std::string, std::shared_ptr<CTransform> > m_Transforms; ///< transforms associated with ADIOS run

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

    void SetTransform( const std::string transform );

    void SetTransport( const std::string streamName, const std::string transport, const bool debugMode );

    void SetBuffer( const std::string streamName, const unsigned long long int maxBufferSize );

    /**
     * Open a certain stream based on transport method
     * @param streamName associated file or stream
     * @param accessMode "w": write, "a": append, need more info on this
     */
    void Open( const std::string streamName, const std::string accessMode );

    /**
     * Writes raw data to m_Buffer, call for local variables
     * @param streamName key to get the corresponding buffer from m_Buffers
     * @param data pointer containing the data
     * @param dataSize sizeof each element of data
     * @param localDimensions if scalar it will have one number, if multidimensional it will start with the slowest moving dimension
     */
    void WriteDataToBuffer( const std::string streamName, const void* data, const size_t dataSize,
                            const std::vector<unsigned long long int>& localDimensions );

    /**
     * Writes raw data to m_Buffer, call for global variables
     * @param data pointer containing the data
     * @param dataSize sizeof each element of data
     * @param localDimensions if scalar it will have one number, if multidimensional it will start with the slowest moving dimension
     * @param globalDimensions global dimensions, if multidimensional it will start with the slowest moving dimension
     * @param globalOffsets global offsets, if multidimensional it will start with the slowest moving dimension offset
     */
    void WriteDataToBuffer( const void* data, const size_t dataSize,
                            const std::vector<unsigned long long int>& localDimensions,
                            const std::vector<unsigned long long int>& globalDimensions,
                            const std::vector<unsigned long long int>& globalOffsets  );

    /**
     * Closes a certain stream at the transport level
     * @param streamName passed to corresponding transport so it can be closed.
     */
    void CloseStream( const std::string streamName );

};


} //end namespace

#endif /* CCAPSULE_H_ */
