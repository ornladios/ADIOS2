/*
 * CEmptyCapsule.h
 *
 *  Created on: Nov 9, 2016
 *      Author: wfg
 */

#ifndef CEMPTYCAPSULE_H_
#define CEMPTYCAPSULE_H_

namespace adios
{

/**
 * Base class for Capsule operations
 */
class CEmptyCapsule : public CCapsule
{

public:

    CEmptyCapsule( MPI_Comm mpiComm ); ///< default empty constructor

    ~CEmptyCapsule( );

    /**
     * This will add to the m_Transports and m_Transforms map
     * @param group
     */
    void OpenGroupBuffer( const CGroup& group );



    void WriteVariableToBuffer( const CGroup& group, const SVariable<T>& variable );

    /**
     * Closes the buffer and moves it into the
     * @param group
     */
    void CloseGroupBuffer( const CGroup& group );

};


} //end namespace



#endif /* CEMPTYCAPSULE_H_ */
