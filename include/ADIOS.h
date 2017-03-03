/*
 * ADIOS.h
 *
 *  Created on: Oct 3, 2016
 *      Author: wfg
 */

#ifndef ADIOS_H_
#define ADIOS_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
#include <vector>
#include <memory> //std::shared_ptr
#include <ostream>
#include <set>
#include <map>
#include <complex>
/// \endcond

#ifdef HAVE_MPI
  #include <mpi.h>
#else
  #include "mpidummy.h"
#endif

#include "core/Transform.h"
#include "core/Variable.h"
#include "core/VariableCompound.h"
#include "core/Method.h"
#include "core/Support.h"
#include "functions/adiosTemplates.h"


namespace adios
{

/** Use these values in Dims() when defining variables
 */
typedef enum {
    VARYING_DIMENSION = -1,//!< VARYING_DIMENSION
    LOCAL_VALUE = 0,       //!< LOCAL_VALUE
    GLOBAL_VALUE = 1       //!< GLOBAL_VALUE
};

typedef enum { ERROR = 0, WARN = 1, INFO = 2, DEBUG = 3 } VerboseFlag;

typedef enum { INDEPENDENT_IO = 0, COLLECTIVE_IO = 1 } IOMode;


class Engine;

/**
 * @brief Unique class interface between user application and ADIOS library
 */
class ADIOS
{

public: // PUBLIC Constructors and Functions define the User Interface with ADIOS

    /**
     * @brief ADIOS empty constructor. Used for non XML config file API calls.
     */
    ADIOS( const adios::VerboseFlag verbose = WARN, const bool debugMode = false );


    /**
     * @brief Serial constructor for config file
     * @param configFileName passed to m_ConfigFile
     * @param debugMode true: on throws exceptions and do additional checks, false: off (faster, but unsafe)
     */
    ADIOS( const std::string configFileName, const adios::VerboseFlag verbose = WARN, const bool debugMode = false );


    /**
     * @brief Parallel constructor for XML config file and MPI
     * @param configFileName passed to m_XMLConfigFile
     * @param mpiComm MPI communicator ...const to be discussed
     * @param debugMode true: on, false: off (faster, but unsafe)
     */
    ADIOS( const std::string configFileName, const MPI_Comm mpiComm, const adios::VerboseFlag verbose = WARN, const bool debugMode = false );


    /**
     * @brief Parallel MPI communicator without XML config file
     * @param mpiComm MPI communicator passed to m_MPIComm*
     * @param debugMode true: on, false: off (faster)
     */
    ADIOS( const MPI_Comm mpiComm, const adios::VerboseFlag verbose = WARN, const bool debugMode = false );


    ~ADIOS( ); ///< empty, using STL containers for memory management


     /**
     * Look for template specialization
     * @param name
     * @param dimensions
     * @param globalDimensions
     * @param globalOffsets
     * @return
     */
    template<class T> inline
    Variable<T>& DefineVariable( const std::string name, const Dims dimensions = Dims{1},
                                 const Dims globalDimensions = Dims( ),
                                 const Dims globalOffsets = Dims() )
    {
        throw std::invalid_argument( "ERROR: type not supported for variable " + name + " in call to DefineVariable\n" );
    }

    template<class T> inline
    Variable<T>& GetVariable( const std::string name )
    {
        throw std::invalid_argument( "ERROR: type not supported for variable " + name + " in call to GetVariable\n" );
    }


    template<class T>
    VariableCompound& DefineVariableCompound( const std::string name, const Dims dimensions = Dims{1},
                                              const Dims globalDimensions = Dims(),
                                              const Dims globalOffsets = Dims() )
    {
        CheckVariableInput( name, dimensions );
        m_Compound.emplace_back( name, sizeof(T), dimensions, globalDimensions, globalOffsets, m_DebugMode );
        m_Variables.emplace( name, std::make_pair( GetType<T>(), m_Compound.size()-1 ) );
        return m_Compound.back();
    }

    VariableCompound& GetVariableCompound( const std::string name );


    /**
     * Declares a new method. If the method is defined in the user config file,
     * it will be already created during processing the config file,
     * the method is set up with the user settings and this function just returns that method.
     * Otherwise it will create and return a new Method with default settings.
     * Use method.isUserDefined() to distinguish between the two cases.
     * @param methodName must be unique
     * @param type supported type : "BP" (default), "DataMan"...future: "Sirius"
     */
    Method& DeclareMethod( const std::string methodName, const std::string type = "" );


    /**
     * @brief Open to Write, Read. Creates a new engine from previously defined method
     * @param streamName unique stream or file name
     * @param accessMode "w" or "write", "r" or "read", "a" or "append", "u" or "update"
     * @param mpiComm option to modify communicator from ADIOS class constructor
     * @param method looks for corresponding Method object in ADIOS to initialize the engine
     * @param iomode Independent or collective open/advance by writers/readers? Write() operations are always independent.
     * @return Derived class of base Engine depending on Method parameters, shared_ptr for potential flexibility
     */
    std::shared_ptr<Engine> Open( const std::string streamName, const std::string accessMode, MPI_Comm mpiComm,
                                  const Method& method, const enum IOMode iomode );

    /**
     * @brief Open to Read with a timeout value specified. Creates a new engine from previously defined method
     * @param streamName unique stream or file name
     * @param accessMode "w" or "write", "r" or "read", "a" or "append", "u" or "update"
     * @param mpiComm option to modify communicator from ADIOS class constructor
     * @param method looks for corresponding Method object in ADIOS to initialize the engine
     * @param iomode Independent or collective open/advance by writers/readers? Write() operations are always independent.
     * @param timeout_sec Wait some time before reporting on missing stream (i.e. wait for it for a while)
     * @return Derived class of base Engine depending on Method parameters, shared_ptr for potential flexibility
     */
    std::shared_ptr<Engine> Open( const std::string streamName, const std::string accessMode, MPI_Comm mpiComm,
                                  const Method& method, const enum IOMode iomode,
                                  const float timeout_sec );
    /**
     * @brief Open to Write, Read. Creates a new engine from previously defined method.
     * Reuses MPI communicator from ADIOS constructor.
     * @param streamName unique stream or file name
     * @param accessMode "w" or "write", "r" or "read", "a" or "append", "u" or "update"
     * @param method contains engine parameters
     * @param iomode Independent or collective open/advance by writers/readers? Write() operations are always independent.
     * @return Derived class of base Engine depending on Method parameters, shared_ptr for potential flexibility
     */
    std::shared_ptr<Engine> Open( const std::string streamName, const std::string accessMode, const Method& method,
                                  const enum IOMode iomode );


    /**
     * @brief Open to Read with a timeout value specified. Creates a new engine from previously defined method
     * Reuses MPI communicator from ADIOS constructor.
     * @param streamName unique stream or file name
     * @param accessMode "w" or "write", "r" or "read", "a" or "append", "u" or "update"
     * @param method looks for corresponding Method object in ADIOS to initialize the engine
     * @param iomode Independent or collective open/advance by writers/readers? Write() operations are always independent.
     * @param timeout_sec Wait some time before reporting on missing stream (i.e. wait for it for a while)
     * @return Derived class of base Engine depending on Method parameters, shared_ptr for potential flexibility
     */
    std::shared_ptr<Engine> Open( const std::string streamName, const std::string accessMode, const Method& method,
                                  const enum IOMode iomode,
                                  const float timeout_sec );


    /**
     * @brief Open to Read all steps from a file. No streaming, advancing is possible here. All steps in the file
     * are immediately available for reading. Creates a new engine from previously defined method.
     * @param fileName file name
     * @param mpiComm option to modify communicator from ADIOS class constructor
     * @param method looks for corresponding Method object in ADIOS to initialize the engine
     * @param iomode Independent or collective open/advance by writers/readers? Write() operations are always independent.
     * @return Derived class of base Engine depending on Method parameters, shared_ptr for potential flexibility
     */
    std::shared_ptr<Engine> OpenFileReader( const std::string streamName, MPI_Comm mpiComm,
                                            const Method& method, const enum IOMode iomode );

    /**
     * @brief Dumps groups information to a file stream or standard output.
     * Note that either the user closes this fileStream or it's closed at the end.
     * @param logStream either std::cout standard output, or a std::ofstream file
     */
    void MonitorVariables( std::ostream& logStream );



private: //no const to allow default empty and copy constructors

};

} //end namespace


#endif /* ADIOS_H_ */
