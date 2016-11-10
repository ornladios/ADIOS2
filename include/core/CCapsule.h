/*
 * CCapsule.h
 *
 *  Created on: Nov 7, 2016
 *      Author: wfg
 */

#ifndef CCAPSULE_H_
#define CCAPSULE_H_

#include <vector>

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

    unsigned long int m_BufferSize; ///< buffer size
    std::vector<char> m_Buffer; ///< buffer to be managed, just one type for now
    std::map< std::string, std::shared_ptr<CTransform> > m_Transforms; ///< transforms associated with ADIOS run
    std::map< std::string, std::shared_ptr<CTransport> > m_Transports; ///< transports associated with ADIOS run

    /**
     * Unique constructor
     * @param mpiComm communicator passed from ADIOS
     */
    CCapsule( const MPI_Comm mpiComm, const unsigned long int bufferSize ):
        m_MPIComm{ mpiComm },
        m_BufferSize{ bufferSize }
    { }

    virtual ~CCapsule( ){ };

    /**
     * This will add to the m_Transports and m_Transforms map
     * @param group
     */
    void OpenGroupBuffer( const CGroup& group ) = 0;

    virtual void WriteVariableToBuffer( const CGroup& group, const SVariable<unsigned int>& variable );
    virtual void WriteVariableToBuffer( const CGroup& group, const SVariable<int>& variable );
    virtual void WriteVariableToBuffer( const CGroup& group, const SVariable<float>& variable );
    virtual void WriteVariableToBuffer( const CGroup& group, const SVariable<double>& variable );

    /**
     * Closes the buffer and moves it into the
     * @param group
     */
    void CloseGroupBuffer( const CGroup& group ) = 0;


protected:

    virtual void SpatialAggregation( const CGroup& group, const SVariable<unsigned int>& variable );
    virtual void SpatialAggregation( const CGroup& group, const SVariable<int>& variable );
    virtual void SpatialAggregation( const CGroup& group, const SVariable<float>& variable );
    virtual void SpatialAggregation( const CGroup& group, const SVariable<double>& variable );

    virtual void TimeAggregation( const CGroup& group, const SVariable<unsigned int>& variable );

};


} //end namespace

#endif /* CCAPSULE_H_ */
