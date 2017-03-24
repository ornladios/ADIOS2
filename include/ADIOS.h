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

#ifdef ADIOS_NOMPI
  #include "mpidummy.h"
#else
  #include <mpi.h>
#endif

#include "ADIOSTypes.h"
#include "core/Transform.h"
#include "core/Variable.h"
#include "core/VariableCompound.h"
#include "core/Method.h"
#include "core/Support.h"
#include "functions/adiosTemplates.h"


namespace adios
{

class Engine;

/**
 * @brief Unique class interface between user application and ADIOS library
 */
class ADIOS
{

public: // PUBLIC Constructors and Functions define the User Interface with ADIOS

    MPI_Comm m_MPIComm = MPI_COMM_SELF; ///< only used as reference to MPI communicator passed from parallel constructor, MPI_Comm is a pointer itself. Public as called from C

    int m_RankMPI = 0; ///< current MPI rank process
    int m_SizeMPI = 1; ///< current MPI processes size

    std::string m_HostLanguage = "C++";

    /**
     * @brief ADIOS empty constructor. Used for non XML config file API calls.
     */
    ADIOS( const Verbose verbose = Verbose::WARN, const bool debugMode = false );


    /**
     * @brief Serial constructor for config file, only allowed and compiled in libadios_nompi.a
     * @param configFileName passed to m_ConfigFile
     * @param debugMode true: on throws exceptions and do additional checks, false: off (faster, but unsafe)
     */
    ADIOS( const std::string configFileName, const Verbose verbose = Verbose::WARN, const bool debugMode = false );

    /**
     * @brief Parallel constructor for XML config file and MPI
     * @param configFileName passed to m_XMLConfigFile
     * @param mpiComm MPI communicator ...const to be discussed
     * @param debugMode true: on, false: off (faster, but unsafe)
     */

    ADIOS( const std::string configFileName, MPI_Comm mpiComm, const Verbose verbose = Verbose::WARN, const bool debugMode = false );


    /**
     * @brief Parallel MPI communicator without XML config file
     * @param mpiComm MPI communicator passed to m_MPIComm*
     * @param debugMode true: on, false: off (faster)
     */
    ADIOS(  MPI_Comm mpiComm, const Verbose verbose = Verbose::WARN, const bool debugMode = false );



    ~ADIOS( ); ///< empty, using STL containers for memory management

    void InitMPI( ); ///< sets rank and size in m_rank and m_Size, respectively.

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
        const unsigned int size = m_Compound.size();
        m_Compound.emplace( size, VariableCompound( name, sizeof(T), dimensions, globalDimensions, globalOffsets, m_DebugMode ) );
        m_Variables.emplace( name, std::make_pair( GetType<T>(), size ) );
        return m_Compound.at( size );
    }

    VariableCompound& GetVariableCompound( const std::string name );


    /**
     * Declares a new method. If the method is defined in the user config file,
     * it will be already created during processing the config file,
     * the method is set up with the user settings and this function just returns that method.
     * Otherwise it will create and return a new Method with default settings.
     * Use method.isUserDefined() to distinguish between the two cases.
     * @param methodName must be unique
     */
    Method& DeclareMethod( const std::string methodName );


    /**
     * @brief Open to Write, Read. Creates a new engine from previously defined method
     * @param streamName unique stream or file name
     * @param accessMode "w" or "write", "r" or "read", "a" or "append", "u" or "update"
     * @param mpiComm option to modify communicator from ADIOS class constructor
     * @param method looks for corresponding Method object in ADIOS to initialize the engine
     * @param iomode Independent or collective open/advance by writers/readers? Write() operations are always independent.
     * @param timeout_sec Wait some time before reporting on missing stream (i.e. wait for it for a while)
      * @return Derived class of base Engine depending on Method parameters, shared_ptr for potential flexibility
     */
    std::shared_ptr<Engine> Open( const std::string streamName, const std::string accessMode, MPI_Comm mpiComm,
                                  const Method& method, const IOMode iomode, const float timeout_sec = 0.0 );

    /**
     * @brief Open to Write, Read. Creates a new engine from previously defined method.
     * Reuses MPI communicator from ADIOS constructor.
     * @param streamName unique stream or file name
     * @param accessMode "w" or "write", "r" or "read", "a" or "append", "u" or "update"
     * @param method contains engine parameters
     * @param iomode Independent or collective open/advance by writers/readers? Write() operations are always independent.
     * @param timeout_sec Wait some time before reporting on missing stream (i.e. wait for it for a while)
      * @return Derived class of base Engine depending on Method parameters, shared_ptr for potential flexibility
     */
    std::shared_ptr<Engine> Open( const std::string streamName, const std::string accessMode, 
    							  const Method& method, const IOMode iomode, const float timeout_sec = 0.0 );


    /**
     * Version required by the XML config file implementation, searches method inside ADIOS through a unique name
     * @param streamName unique stream or file name
     * @param accessMode "w" or "write", "r" or "read", "a" or "append"
     * @param mpiComm mpi Communicator
     * @param methodName used to search method object inside ADIOS object
     * @param iomode Independent or collective open/advance by writers/readers? Write() operations are always independent.
     * @param timeout_sec Wait some time before reporting on missing stream (i.e. wait for it for a while)
     * @return Derived class of base Engine depending on Method parameters, shared_ptr for potential flexibility
     */
    std::shared_ptr<Engine> Open( const std::string streamName, const std::string accessMode, MPI_Comm mpiComm,
                                  const std::string methodName, const IOMode iomode, const float timeout_sec = 0.0 );

    /**
     * Version required by the XML config file implementation, searches method inside ADIOS through a unique name.
     * Reuses ADIOS MPI Communicator from constructor.
     * @param streamName unique stream or file name
     * @param accessMode "w" or "write", "r" or "read", "a" or "append"
     * @param methodName used to search method object inside ADIOS object
     * @param iomode Independent or collective open/advance by writers/readers? Write() operations are always independent.
     * @param timeout_sec Wait some time before reporting on missing stream (i.e. wait for it for a while)
     * @return Derived class of base Engine depending on Method parameters, shared_ptr for potential flexibility
     */
    std::shared_ptr<Engine> Open( const std::string streamName, const std::string accessMode,
                                  const std::string methodName, const IOMode iomode, const float timeout_sec = 0.0 );

    /**
     * @brief Open to Read all steps from a file. No streaming, advancing is possible here. All steps in the file
     * are immediately available for reading. Creates a new engine from previously defined method.
     * @param fileName file name
     * @param mpiComm option to modify communicator from ADIOS class constructor
     * @param method looks for corresponding Method object in ADIOS to initialize the engine
     * @param iomode Independent or collective open/advance by writers/readers? Write() operations are always independent.
     * @return Derived class of base Engine depending on Method parameters, shared_ptr for potential flexibility
     */
    std::shared_ptr<Engine> OpenFileReader( const std::string fileName, MPI_Comm mpiComm,
                                            const Method& method, const IOMode iomode );

    /**
     * @brief Open to Read all steps from a file. No streaming, advancing is possible here. All steps in the file
     * are immediately available for reading. Creates a new engine from previously defined method.
     * Version required by the XML config file implementation, searches method inside ADIOS through a unique name.
     * @param fileName file name
     * @param mpiComm option to modify communicator from ADIOS class constructor
     * @param methodName used to search method object inside ADIOS object
      * @param iomode Independent or collective open/advance by writers/readers? Write() operations are always independent.
     * @return Derived class of base Engine depending on Method parameters, shared_ptr for potential flexibility
     */
    std::shared_ptr<Engine> OpenFileReader( const std::string fileName, MPI_Comm mpiComm,
                                            const std::string methodName, const IOMode iomode );

    /**
     * @brief Dumps groups information to a file stream or standard output.
     * Note that either the user closes this fileStream or it's closed at the end.
     * @param logStream either std::cout standard output, or a std::ofstream file
     */
    void MonitorVariables( std::ostream& logStream );



protected: //no const to allow default empty and copy constructors

    std::map<unsigned int, Variable<char> > m_Char;
    std::map<unsigned int, Variable<unsigned char> > m_UChar;
    std::map<unsigned int, Variable<short> > m_Short;
    std::map<unsigned int, Variable<unsigned short> > m_UShort;
    std::map<unsigned int, Variable<int> > m_Int;
    std::map<unsigned int, Variable<unsigned int> > m_UInt;
    std::map<unsigned int, Variable<long int> > m_LInt;
    std::map<unsigned int, Variable<unsigned long int> > m_ULInt;
    std::map<unsigned int, Variable<long long int> > m_LLInt;
    std::map<unsigned int, Variable<unsigned long long int> > m_ULLInt;
    std::map<unsigned int, Variable<float> > m_Float;
    std::map<unsigned int, Variable<double> > m_Double;
    std::map<unsigned int, Variable<long double> > m_LDouble;
    std::map<unsigned int, Variable<std::complex<float>> > m_CFloat;
    std::map<unsigned int, Variable<std::complex<double>> > m_CDouble;
    std::map<unsigned int, Variable<std::complex<long double>> > m_CLDouble;
    std::map<unsigned int, VariableCompound > m_Compound;

    std::string m_ConfigFile; ///< XML File to be read containing configuration information
    bool m_DebugMode = false; ///< if true will do more checks, exceptions, warnings, expect slower code

    //Variables
    std::map< std::string, std::pair< std::string, unsigned int > > m_Variables; ///< Makes variable name unique, key: variable name, value: pair.first = type, pair.second = index in corresponding vector of Variable

    std::vector< std::shared_ptr<Transform> > m_Transforms; ///< transforms associated with ADIOS run


    /**
     * @brief List of Methods (engine metadata) defined from either ADIOS XML configuration file or the DeclareMethod function.
     * <pre>
     *     Key: std::string unique method name
     *     Value: Method class
     * </pre>
     */
    std::map< std::string, Method > m_Methods;
    std::set< std::string > m_EngineNames; ///< set used to check Engine name uniqueness in debug mode

    /**
     * @brief Checks for group existence in m_Groups, if failed throws std::invalid_argument exception
     * @param itGroup m_Groups iterator, usually from find function
     * @param groupName unique name, passed for thrown exception only
     * @param hint adds information to thrown exception
     */
    void CheckVariableInput( const std::string name, const Dims& dimensions ) const;

    /**
     * Checks for variable name, if not found throws an invalid exception
     * @param itVariable iterator pointing to the variable name in m_Variables
     * @param name variable name
     * @param hint message to be thrown for debugging purporses
     */
    void CheckVariableName( std::map< std::string, std::pair< std::string, unsigned int > >::const_iterator itVariable,
                            const std::string name, const std::string hint ) const;

    /**
     * @brief Checks for method existence in m_Methods, if failed throws std::invalid_argument exception
     * @param itMethod m_Methods iterator, usually from find function
     * @param methodName unique name, passed for thrown exception only
     * @param hint adds information to thrown exception
     */
    void CheckMethod( std::map< std::string, Method >::const_iterator itMethod,
                      const std::string methodName, const std::string hint ) const;

    template< class T >
    unsigned int GetVariableIndex( const std::string name )
    {
        auto itVariable = m_Variables.find( name );
        CheckVariableName( itVariable, name, "in call to GetVariable<" + GetType<T>() + ">, or call to GetVariableCompound if <T> = <compound>\n" );
        return itVariable->second.second;
    }

};

//template specializations of DefineVariable:
template<> inline
Variable<char>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                       const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    const unsigned int size = m_Char.size();
    m_Char.emplace( size, Variable<char>( name, dimensions, globalDimensions, globalOffsets, m_DebugMode ) );
    m_Variables.emplace( name, std::make_pair( GetType<char>(), size ) );
    return m_Char.at( size );
}


template<> inline
Variable<unsigned char>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                                const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    const unsigned int size = m_UChar.size();
    m_UChar.emplace( size, Variable<unsigned char>( name, dimensions, globalDimensions, globalOffsets, m_DebugMode ) );
    m_Variables.emplace( name, std::make_pair( GetType<unsigned char>(), size ) );
    return m_UChar.at( size );
}


template<> inline
Variable<short>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                        const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    const unsigned int size = m_Short.size();
    m_Short.emplace( size, Variable<short>( name, dimensions, globalDimensions, globalOffsets, m_DebugMode ) );
    m_Variables.emplace( name, std::make_pair( GetType<unsigned char>(), size ) );
    return m_Short.at( size );
}


template<> inline
Variable<unsigned short>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                                 const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    const unsigned int size = m_UShort.size();
    m_UShort.emplace( size, Variable<unsigned short>( name, dimensions, globalDimensions, globalOffsets, m_DebugMode ) );
    m_Variables.emplace( name, std::make_pair( GetType<unsigned short>(), size ) );
    return m_UShort.at( size );
}


template<> inline
Variable<int>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                      const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    const unsigned int size = m_Int.size();
    m_Int.emplace( size, Variable<int>( name, dimensions, globalDimensions, globalOffsets, m_DebugMode ) );
    m_Variables.emplace( name, std::make_pair( GetType<int>(), size ) );
    return m_Int.at( size );
}


template<> inline
Variable<unsigned int>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                               const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    const unsigned int size = m_UInt.size();
    m_UInt.emplace( size, Variable<unsigned int>( name, dimensions, globalDimensions, globalOffsets, m_DebugMode ) );
    m_Variables.emplace( name, std::make_pair( GetType<unsigned int>(), size ) );
    return m_UInt.at( size );
}


template<> inline
Variable<long int>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                           const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    const unsigned int size = m_LInt.size();
    m_LInt.emplace( size, Variable<long int>( name, dimensions, globalDimensions, globalOffsets, m_DebugMode ) );
    m_Variables.emplace( name, std::make_pair( GetType<long int>(), size ) );
    return m_LInt.at( size );
}


template<> inline
Variable<unsigned long int>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                                    const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    const unsigned int size = m_LInt.size();
    m_ULInt.emplace( size, Variable<unsigned long int>( name, dimensions, globalDimensions, globalOffsets, m_DebugMode ) );
    m_Variables.emplace( name, std::make_pair( GetType<unsigned long int>(), size ) );
    return m_ULInt.at( size );
}


template<> inline
Variable<long long int>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                                const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    const unsigned int size = m_LLInt.size();
    m_LLInt.emplace( size, Variable<long long int>( name, dimensions, globalDimensions, globalOffsets, m_DebugMode ) );
    m_Variables.emplace( name, std::make_pair( GetType<long long int>(), size ) );
    return m_LLInt.at( size );
}


template<> inline
Variable<unsigned long long int>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                                         const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    const unsigned int size = m_ULLInt.size();
    m_ULLInt.emplace( size, Variable<unsigned long long int>( name, dimensions, globalDimensions, globalOffsets, m_DebugMode ) );
    m_Variables.emplace( name, std::make_pair( GetType<unsigned long long int>(), size ) );
    return m_ULLInt.at( size );
}

template<> inline
Variable<float>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                        const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    const unsigned int size = m_Float.size();
    m_Float.emplace( size, Variable<float>( name, dimensions, globalDimensions, globalOffsets, m_DebugMode ) );
    m_Variables.emplace( name, std::make_pair( GetType<float>(), size ) );
    return m_Float.at( size );
}


template<> inline
Variable<double>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                         const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    const unsigned int size = m_Double.size();
    m_Double.emplace( size, Variable<double>( name, dimensions, globalDimensions, globalOffsets, m_DebugMode ) );
    m_Variables.emplace( name, std::make_pair( GetType<double>(), size ) );
    return m_Double.at( size );
}


template<> inline
Variable<long double>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                              const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    const unsigned int size = m_LDouble.size();
    m_LDouble.emplace( size, Variable<long double>( name, dimensions, globalDimensions, globalOffsets, m_DebugMode ) );
    m_Variables.emplace( name, std::make_pair( GetType<long double>(), size ) );
    return m_LDouble.at( size );
}


template<> inline
Variable<std::complex<float>>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                                      const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    const unsigned int size = m_CFloat.size();
    m_CFloat.emplace( size, Variable<std::complex<float>>( name, dimensions, globalDimensions, globalOffsets, m_DebugMode ) );
    m_Variables.emplace( name, std::make_pair( GetType<std::complex<float>>(), size ) );
    return m_CFloat.at( size );
}


template<> inline
Variable<std::complex<double>>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                                       const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    const unsigned int size = m_CDouble.size();
    m_CDouble.emplace( size, Variable<std::complex<double>>( name, dimensions, globalDimensions, globalOffsets, m_DebugMode ) );
    m_Variables.emplace( name, std::make_pair( GetType<std::complex<double>>(), size ) );
    return m_CDouble.at( size );
}


template<> inline
Variable<std::complex<long double>>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                                            const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    const unsigned int size = m_CLDouble.size();
    m_CLDouble.emplace( size, Variable<std::complex<long double>>( name, dimensions, globalDimensions, globalOffsets, m_DebugMode ) );
    m_Variables.emplace( name, std::make_pair( GetType<std::complex<long double>>(), size ) );
    return m_CLDouble.at( size );
}


//Get template specialization
template<> inline
Variable<char>& ADIOS::GetVariable( const std::string name )
{ return m_Char.at( GetVariableIndex<char>(name) ); }

template<> inline
Variable<unsigned char>& ADIOS::GetVariable( const std::string name )
{ return m_UChar.at( GetVariableIndex<unsigned char>(name) ); }

template<> inline
Variable<short>& ADIOS::GetVariable( const std::string name )
{ return m_Short.at( GetVariableIndex<short>(name) ); }

template<> inline
Variable<unsigned short>& ADIOS::GetVariable( const std::string name )
{ return m_UShort.at( GetVariableIndex<unsigned short>(name) ); }

template<> inline
Variable<int>& ADIOS::GetVariable( const std::string name )
{ return m_Int.at( GetVariableIndex<int>(name) ); }

template<> inline
Variable<unsigned int>& ADIOS::GetVariable( const std::string name )
{ return m_UInt.at( GetVariableIndex<unsigned int>(name) ); }

template<> inline
Variable<long int>& ADIOS::GetVariable( const std::string name )
{ return m_LInt.at( GetVariableIndex<unsigned int>(name) ); }

template<> inline
Variable<unsigned long int>& ADIOS::GetVariable( const std::string name )
{ return m_ULInt.at( GetVariableIndex<unsigned long int>(name) ); }

template<> inline
Variable<long long int>& ADIOS::GetVariable( const std::string name )
{ return m_LLInt.at( GetVariableIndex<long long int>(name) ); }

template<> inline
Variable<unsigned long long int>& ADIOS::GetVariable( const std::string name )
{ return m_ULLInt.at( GetVariableIndex<unsigned long long int>(name) ); }

template<> inline
Variable<float>& ADIOS::GetVariable( const std::string name )
{ return m_Float.at( GetVariableIndex<float>(name) ); }

template<> inline
Variable<double>& ADIOS::GetVariable( const std::string name )
{ return m_Double.at( GetVariableIndex<double>(name) ); }

template<> inline
Variable<long double>& ADIOS::GetVariable( const std::string name )
{ return m_LDouble.at( GetVariableIndex<long double>(name) ); }

template<> inline
Variable<std::complex<float>>& ADIOS::GetVariable( const std::string name )
{ return m_CFloat.at( GetVariableIndex<std::complex<float>>(name) ); }

template<> inline
Variable<std::complex<double>>& ADIOS::GetVariable( const std::string name )
{ return m_CDouble.at( GetVariableIndex<std::complex<double>>(name) ); }

template<> inline
Variable<std::complex<long double>>& ADIOS::GetVariable( const std::string name )
{ return m_CLDouble.at( GetVariableIndex<std::complex<long double>>(name) ); }


} //end namespace


#endif /* ADIOS_H_ */
