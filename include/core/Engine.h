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

    const std::string m_EngineType; ///< from derived class
    const std::string m_Name; ///< name used for this engine
    const std::string m_AccessMode; ///< accessMode for buffers used by this engine
    Method* m_Method = nullptr; ///< associated method containing engine metadata
    Group* m_Group = nullptr; ///< associated group to look for variable information

    int m_RankMPI = 0; ///< current MPI rank process
    int m_SizeMPI = 1; ///< current MPI processes size

    /**
     * Unique constructor based on a method (engine metadata)
     * @param engineType given by derived classes
     * @param name engine name
     * @param accessMode
     * @param mpiComm
     * @param method
     */
    Engine( const std::string engineType, const std::string name, const std::string accessMode, const MPI_Comm mpiComm,
            const Method& method, const bool debugMode );


    virtual ~Engine( );


    template< class T >
    void Write( Group& group, const std::string variableName, const T* values );

    /**
     * @brief Write functions can be overridden by derived classes. Base class behavior is to:
     * 1) Write to Variable values (m_Values) in a group
     * 2) Transform the data
     * 3) Write to all capsules -> data and metadata
     * @param group
     * @param variableName
     * @param values coming from user app
     */
    virtual void Write( Group& group, const std::string variableName, const char* values );
    virtual void Write( Group& group, const std::string variableName, const unsigned char* values );
    virtual void Write( Group& group, const std::string variableName, const short* values );
    virtual void Write( Group& group, const std::string variableName, const unsigned short* values );
    virtual void Write( Group& group, const std::string variableName, const int* values );
    virtual void Write( Group& group, const std::string variableName, const unsigned int* values );
    virtual void Write( Group& group, const std::string variableName, const long int* values );
    virtual void Write( Group& group, const std::string variableName, const unsigned long int* values );
    virtual void Write( Group& group, const std::string variableName, const long long int* values );
    virtual void Write( Group& group, const std::string variableName, const unsigned long long int* values );
    virtual void Write( Group& group, const std::string variableName, const float* values );
    virtual void Write( Group& group, const std::string variableName, const double* values );
    virtual void Write( Group& group, const std::string variableName, const long double* values );

    virtual void Write( const std::string variableName, const char* values );
    virtual void Write( const std::string variableName, const unsigned char* values );
    virtual void Write( const std::string variableName, const short* values );
    virtual void Write( const std::string variableName, const unsigned short* values );
    virtual void Write( const std::string variableName, const int* values );
    virtual void Write( const std::string variableName, const unsigned int* values );
    virtual void Write( const std::string variableName, const long int* values );
    virtual void Write( const std::string variableName, const unsigned long int* values );
    virtual void Write( const std::string variableName, const long long int* values );
    virtual void Write( const std::string variableName, const unsigned long long int* values );
    virtual void Write( const std::string variableName, const float* values );
    virtual void Write( const std::string variableName, const double* values );
    virtual void Write( const std::string variableName, const long double* values );

    virtual void Close( int transportIndex = -1  ); ///< Closes a particular transport


protected:

    std::vector< std::shared_ptr<Transport> > m_Transports; ///< transports managed
    std::vector< std::shared_ptr<Capsule> > m_Capsules; ///< managed Capsules
    const bool m_DebugMode = false;

    virtual void SetCapsules( ); ///< Initialize transports from Method, called from constructor.
    virtual void SetTransports( ); ///< Initialize transports from Method, called from constructor.

    /**
     * Performs preliminary checks before writing a variable. Throws an exception if checks fail.
     * Returns an index to variable type container in Group
     * @param group variable group owner object
     * @param variableName variable to be checked
     * @param types from Support class, collection of type aliases
     * @param hint added information if exception is thrown
     * @return index to variable in group type container
     */
    const unsigned int PreSetVariable( Group& group, const std::string variableName,
                                       const std::set<std::string>& types,
                                       const std::string hint ) const;


    std::string GetName( const std::vector<std::string>& arguments ) const; //might move this to adiosFunctions

};


} //end namespace

#endif /* ENGINE_H_ */
