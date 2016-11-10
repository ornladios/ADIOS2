/*
 * CMMap.h capsule that uses mapped files for shared memory
 *
 *  Created on: Nov 8, 2016
 *      Author: wfg
 */

#ifndef CMMAP_H_
#define CMMAP_H_

#include "core/CCapsule.h"


namespace adios
{

/**
 * Capsule based on mmap (mapped files)
 */
class CMMap : public CCapsule
{

public:

    #ifdef HAVE_MPI
    MPI_Comm m_MPIComm = NULL; ///< only used as reference to MPI communicator passed from parallel constructor, MPI_Comm is a pointer itself. Public as called from C
    #else
    MPI_Comm m_MPIComm = 0; ///< only used as reference to MPI communicator passed from parallel constructor, MPI_Comm is a pointer itself. Public as called from C
    #endif

    std::vector<char> m_Buffer; ///< buffer to be managed, just one type for now

    CMMap( ); ///< default empty constructor

    ~CMMap( );

    /**
     * This will add to the m_Transports and m_Transforms map
     * @param group
     */
    void OpenGroupBuffer( const CGroup& group );


    template< class T>
    void WriteVariableToBuffer( const CGroup& group, const SVariable<T>& variable );

    /**
     * Closes the buffer and moves it into the
     * @param group
     */
    void CloseGroupBuffer( const CGroup& group );

private:

    template< class T>
    void SpatialAggregation( const CGroup& group, const SVariable<T>& variable )
    { }

    template< class T>
    void TimeAggregation( const CGroup& group, const SVariable<T>& variable )
    { }

};

} //end namespace

#endif /* CMMAP_H_ */
