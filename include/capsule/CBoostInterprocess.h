/*
 * CInterprocess.h Capsule that uses boost interprocess for shared memory operations
 *
 *  Created on: Nov 8, 2016
 *      Author: wfg
 */

#ifndef CBOOSTINTERPROCESS_H_
#define CBOOSTINTERPROCESS_H_

#include "core/CCapsule.h"


namespace adios
{

/**
 * Base class for Capsule operations
 */
class CBoostInterprocess : public CCapsule
{

public:

    #ifdef HAVE_MPI
    MPI_Comm m_MPIComm = NULL; ///< only used as reference to MPI communicator passed from parallel constructor, MPI_Comm is a pointer itself. Public as called from C
    #else
    MPI_Comm m_MPIComm = 0; ///< only used as reference to MPI communicator passed from parallel constructor, MPI_Comm is a pointer itself. Public as called from C
    #endif

    std::vector<char> m_Buffer; ///< buffer to be managed, just one type for now

    CBoostInterprocess( ); ///< default empty constructor

    ~CBoostInterprocess( );

    /**
     * This will add to the m_Transports and m_Transforms map
     * @param group
     */
    void OpenGroupBuffer( const CGroup& group );


    template< class T>
    void WriteVariableToBuffer( const CGroup& group, const SVariable<T>& variable )
    {
//        while
//            {
//
//            send
//            receive status
//            if( status )
//              switch Transport
//            }
    }

    /**
     * Closes the buffer and moves it into the
     * @param group
     */
    void CloseGroupBuffer( const CGroup& group );

};


} //end namespace



#endif /* CBOOSTINTERPROCESS_H_ */
