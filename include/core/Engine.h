/*
 * Capsule.h
 *
 *  Created on: Nov 7, 2016
 *      Author: wfg
 */

#ifndef ENGINE_H_
#define ENGINE_H_


/// \cond EXCLUDE_FROM_DOXYGEN
#include <vector>
#include <string>
#include <memory> //std::shared_ptr
#include <map>
/// \endcond

#ifdef HAVE_MPI
  #include <mpi.h>
#else
  #include "mpidummy.h"
#endif

#include "core/Method.h"
#include "core/Group.h"
#include "core/Variable.h"
#include "core/Transform.h"
#include "core/Transport.h"
#include "core/Capsule.h"


namespace adios
{

/**
 * Base class for Engine operations managing shared-memory, and buffer and variables transform and transport operations
 */
class Engine
{

public:

    #ifdef HAVE_MPI
    MPI_Comm m_MPIComm = NULL; ///< only used as reference to MPI communicator passed from parallel constructor, MPI_Comm is a pointer itself. Public as called from C
    #else
    MPI_Comm m_MPIComm = 0; ///< only used as reference to MPI communicator passed from parallel constructor, MPI_Comm is a pointer itself. Public as called from C
    #endif

    Group* m_CurrentGroup = nullptr; ///< associated group to look for variable information
    int m_RankMPI = 0; ///< current MPI rank process
    int m_SizeMPI = 1; ///< current MPI processes size

    /**
     * Multiple argument constructor
     * @param accessMode
     * @param mpiComm
     * @param debugMode
     */

    Engine( const std::string streamName, const std::string accessMode, const MPI_Comm mpiComm,
            const Method& method );

    virtual ~Engine( );

    virtual void Write( Group& group, Variable<char>& variable );
    virtual void Write( Group& group, Variable<unsigned char>& variable );
    virtual void Write( Group& group, Variable<short>& variable );
    virtual void Write( Group& group, Variable<unsigned short>& variable );
    virtual void Write( Group& group, Variable<int>& variable );
    virtual void Write( Group& group, Variable<unsigned int>& variable );
    virtual void Write( Group& group, Variable<long int>& variable );
    virtual void Write( Group& group, Variable<unsigned long int>& variable );
    virtual void Write( Group& group, Variable<long long int>& variable );
    virtual void Write( Group& group, Variable<unsigned long long int>& variable );
    virtual void Write( Group& group, Variable<float>& variable );
    virtual void Write( Group& group, Variable<double>& variable );
    virtual void Write( Group& group, Variable<long double>& variable );

    virtual void Close( int transportIndex ); ///< Closes a particular transport

private:

    std::vector< std::shared_ptr<Transport> > m_Transports; ///< transports managed
    std::vector< Capsule > m_Buffers; ///< managed Buffers
    const bool m_DebugMode = false;

    std::string GetName( const std::vector<std::string>& arguments ) const;

};


} //end namespace

#endif /* ENGINE_H_ */
