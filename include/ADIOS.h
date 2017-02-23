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
#include <memory> //shared_ptr
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

class Engine;

/**
 * @brief Unique class interface between user application and ADIOS library
 */
class ADIOS
{

public: // PUBLIC Constructors and Functions define the User Interface with ADIOS

    #ifdef HAVE_MPI
    MPI_Comm m_MPIComm = MPI_COMM_NULL; ///< only used as reference to MPI communicator passed from parallel constructor, MPI_Comm is a pointer itself. Public as called from C
    #else
    MPI_Comm m_MPIComm = 0; ///< only used as reference to MPI communicator passed from parallel constructor, MPI_Comm is a pointer itself. Public as called from C
    #endif

    int m_RankMPI = 0; ///< current MPI rank process
    int m_SizeMPI = 1; ///< current MPI processes size

    std::string m_HostLanguage = "C++";

    /**
     * @brief ADIOS empty constructor. Used for non XML config file API calls.
     */
    ADIOS( const bool debugMode = false );


    /**
     * @brief Serial constructor for config file
     * @param configFileName passed to m_ConfigFile
     * @param debugMode true: on throws exceptions and do additional checks, false: off (faster, but unsafe)
     */
    ADIOS( const std::string configFileName, const bool debugMode = false );


    /**
     * @brief Parallel constructor for XML config file and MPI
     * @param configFileName passed to m_XMLConfigFile
     * @param mpiComm MPI communicator ...const to be discussed
     * @param debugMode true: on, false: off (faster, but unsafe)
     */
    ADIOS( const std::string configFileName, const MPI_Comm mpiComm, const bool debugMode = false );


    /**
     * @brief Parallel MPI communicator without XML config file
     * @param mpiComm MPI communicator passed to m_MPIComm*
     * @param debugMode true: on, false: off (faster)
     */
    ADIOS( const MPI_Comm mpiComm, const bool debugMode = false );


    ~ADIOS( ); ///< empty, using STL containers for memory management


    void InitMPI( ); ///< sets rank and size in m_rank and m_Size, respectively.


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
     * Declares a new method
     * @param name must be unique
     * @param type supported type : "Writer" (default), "DataMan"...future: "Sirius"
     */
    Method& DeclareMethod( const std::string methodName, const std::string type = "BPWriter" );


    /**
     * @brief Open to Write, Read. Creates a new engine from previously defined method
     * @param streamName unique stream or file name
     * @param accessMode "w" or "write", "r" or "read", "a" or "append"
     * @param mpiComm option to modify communicator from ADIOS class constructor
     * @param method looks for corresponding Method object in ADIOS to initialize the engine
     * @param cores optional parameter for threaded operations
     * @return Derived class of base Engine depending on Method parameters, shared_ptr for potential flexibility
     */
    std::shared_ptr<Engine> Open( const std::string streamName, const std::string accessMode, MPI_Comm mpiComm,
                                  const Method& method, const unsigned int cores = 1 );


    /**
     * @brief Open to Write, Read. Creates a new engine from previously defined method.
     * Reuses MPI communicator from ADIOS constructor.
     * @param streamName unique stream or file name
     * @param accessMode "w" or "write", "r" or "read", "a" or "append"
     * @param method contains engine parameters
     * @param cores optional parameter for threaded operations
     * @return Derived class of base Engine depending on Method parameters, shared_ptr for potential flexibility
     */
    std::shared_ptr<Engine> Open( const std::string streamName, const std::string accessMode, const Method& method,
                                  const unsigned int cores = 1 );


    /**
     * Version required by the XML config file implementation, searches method inside ADIOS through a unique name
     * @param streamName unique stream or file name
     * @param accessMode "w" or "write", "r" or "read", "a" or "append"
     * @param mpiComm mpi Communicator
     * @param methodName used to search method object inside ADIOS object
     * @param cores optional parameter for threaded operations
     * @return Derived class of base Engine depending on Method parameters, shared_ptr for potential flexibility
     */
    std::shared_ptr<Engine> Open( const std::string streamName, const std::string accessMode, MPI_Comm mpiComm,
                                  const std::string methodName, const unsigned int cores = 1 );

    /**
     * Version required by the XML config file implementation, searches method inside ADIOS through a unique name.
     * Reuses ADIOS MPI Communicator from constructor.
     * @param streamName unique stream or file name
     * @param accessMode "w" or "write", "r" or "read", "a" or "append"
     * @param methodName used to search method object inside ADIOS object
     * @param cores optional parameter for threaded operations
     * @return Derived class of base Engine depending on Method parameters, shared_ptr for potential flexibility
     */
    std::shared_ptr<Engine> Open( const std::string streamName, const std::string accessMode,
                                  const std::string methodName, const unsigned int cores = 1 );

    /**
     * @brief Dumps groups information to a file stream or standard output.
     * Note that either the user closes this fileStream or it's closed at the end.
     * @param logStream either std::cout standard output, or a std::ofstream file
     */
    void MonitorVariables( std::ostream& logStream );



private: //no const to allow default empty and copy constructors

    std::vector< Variable<char> > m_Char; ///< Key: variable name, Value: variable of type char
    std::vector< Variable<unsigned char> > m_UChar; ///< Key: variable name, Value: variable of type unsigned char
    std::vector< Variable<short> > m_Short; ///< Key: variable name, Value: variable of type short
    std::vector< Variable<unsigned short> > m_UShort; ///< Key: variable name, Value: variable of type unsigned short
    std::vector< Variable<int> > m_Int; ///< Key: variable name, Value: variable of type int
    std::vector< Variable<unsigned int> > m_UInt; ///< Key: variable name, Value: variable of type unsigned int
    std::vector< Variable<long int> > m_LInt; ///< Key: variable name, Value: variable of type long int
    std::vector< Variable<unsigned long int> > m_ULInt; ///< Key: variable name, Value: variable of type unsigned long int
    std::vector< Variable<long long int> > m_LLInt; ///< Key: variable name, Value: variable of type long long int
    std::vector< Variable<unsigned long long int> > m_ULLInt; ///< Key: variable name, Value: variable of type unsigned long long int
    std::vector< Variable<float> > m_Float; ///< Key: variable name, Value: variable of type float
    std::vector< Variable<double> > m_Double; ///< Key: variable name, Value: variable of type double
    std::vector< Variable<long double> > m_LDouble; ///< Key: variable name, Value: variable of type double
    std::vector< Variable<std::complex<float>> > m_CFloat; ///< Key: variable name, Value: variable of type complex<float>
    std::vector< Variable<std::complex<double>> > m_CDouble; ///< Key: variable name, Value: variable of type complex<float>
    std::vector< Variable<std::complex<long double>> > m_CLDouble; ///< Key: variable name, Value: variable of type complex<float>
    std::vector< VariableCompound > m_Compound; ///< Key: variable name, Value: compound type variable

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
    m_Char.emplace_back( name, dimensions, globalDimensions, globalOffsets, m_DebugMode );
    m_Variables.emplace( name, std::make_pair( GetType<char>(), m_Char.size()-1 ) );
    return m_Char.back();
}


template<> inline
Variable<unsigned char>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                                const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    m_UChar.emplace_back( name, dimensions, globalDimensions, globalOffsets, m_DebugMode );
    m_Variables.emplace( name, std::make_pair( GetType<unsigned char>(), m_UChar.size()-1 ) );
    return m_UChar.back();
}


template<> inline
Variable<short>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                        const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    m_Short.emplace_back( name, dimensions, globalDimensions, globalOffsets, m_DebugMode );
    m_Variables.emplace( name, std::make_pair( GetType<unsigned char>(), m_Short.size()-1 ) );
    return m_Short.back();
}


template<> inline
Variable<unsigned short>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                                 const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    m_UShort.emplace_back( name, dimensions, globalDimensions, globalOffsets, m_DebugMode );
    m_Variables.emplace( name, std::make_pair( GetType<unsigned short>(), m_UShort.size()-1 ) );
    return m_UShort.back();
}


template<> inline
Variable<int>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                      const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    m_Int.emplace_back( name, dimensions, globalDimensions, globalOffsets, m_DebugMode );
    m_Variables.emplace( name, std::make_pair( GetType<int>(), m_Int.size()-1 ) );
    return m_Int.back();
}


template<> inline
Variable<unsigned int>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                               const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    m_UInt.emplace_back( name, dimensions, globalDimensions, globalOffsets, m_DebugMode );
    m_Variables.emplace( name, std::make_pair( GetType<unsigned int>(), m_UInt.size()-1 ) );
    return m_UInt.back();
}


template<> inline
Variable<long int>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                           const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    m_LInt.emplace_back( name, dimensions, globalDimensions, globalOffsets, m_DebugMode );
    m_Variables.emplace( name, std::make_pair( GetType<long int>(), m_LInt.size()-1 ) );
    return m_LInt.back();
}


template<> inline
Variable<unsigned long int>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                                    const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    m_ULInt.emplace_back( name, dimensions, globalDimensions, globalOffsets, m_DebugMode );
    m_Variables.emplace( name, std::make_pair( GetType<unsigned long int>(), m_ULInt.size()-1 ) );
    return m_ULInt.back();
}


template<> inline
Variable<long long int>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                                const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    m_LLInt.emplace_back( name, dimensions, globalDimensions, globalOffsets, m_DebugMode );
    m_Variables.emplace( name, std::make_pair( GetType<long long int>(), m_LLInt.size()-1 ) );
    return m_LLInt.back();
}


template<> inline
Variable<unsigned long long int>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                                         const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    m_ULLInt.emplace_back( name, dimensions, globalDimensions, globalOffsets, m_DebugMode );
    m_Variables.emplace( name, std::make_pair( GetType<unsigned long long int>(), m_ULLInt.size()-1 ) );
    return m_ULLInt.back();
}

template<> inline
Variable<float>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                        const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    m_Float.emplace_back( name, dimensions, globalDimensions, globalOffsets, m_DebugMode );
    m_Variables.emplace( name, std::make_pair( GetType<float>(), m_Float.size()-1 ) );
    return m_Float.back();
}


template<> inline
Variable<double>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                         const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    m_Double.emplace_back( name, dimensions, globalDimensions, globalOffsets, m_DebugMode );
    m_Variables.emplace( name, std::make_pair( GetType<double>(), m_Double.size()-1 ) );
    return m_Double.back();
}


template<> inline
Variable<long double>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                              const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    m_LDouble.emplace_back( name, dimensions, globalDimensions, globalOffsets, m_DebugMode );
    m_Variables.emplace( name, std::make_pair( GetType<long double>(), m_LDouble.size()-1 ) );
    return m_LDouble.back();
}


template<> inline
Variable<std::complex<float>>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                                      const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    m_CFloat.emplace_back( name, dimensions, globalDimensions, globalOffsets, m_DebugMode );
    m_Variables.emplace( name, std::make_pair( GetType<std::complex<float>>(), m_CFloat.size()-1 ) );
    return m_CFloat.back();
}


template<> inline
Variable<std::complex<double>>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                                       const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    m_CDouble.emplace_back( name, dimensions, globalDimensions, globalOffsets, m_DebugMode );
    m_Variables.emplace( name, std::make_pair( GetType<std::complex<double>>(), m_CDouble.size()-1 ) );
    return m_CDouble.back();
}


template<> inline
Variable<std::complex<long double>>& ADIOS::DefineVariable( const std::string name, const Dims dimensions,
                                                            const Dims globalDimensions, const Dims globalOffsets )
{
    CheckVariableInput( name, dimensions );
    m_CLDouble.emplace_back( name, dimensions, globalDimensions, globalOffsets, m_DebugMode );
    m_Variables.emplace( name, std::make_pair( GetType<std::complex<double>>(), m_CLDouble.size()-1 ) );
    return m_CLDouble.back();
}


//Get template specialization
template<> inline
Variable<char>& ADIOS::GetVariable( const std::string name )
{ return m_Char[ GetVariableIndex<char>(name) ]; }

template<> inline
Variable<unsigned char>& ADIOS::GetVariable( const std::string name )
{ return m_UChar[ GetVariableIndex<unsigned char>(name) ]; }

template<> inline
Variable<short>& ADIOS::GetVariable( const std::string name )
{ return m_Short[ GetVariableIndex<short>(name) ]; }

template<> inline
Variable<unsigned short>& ADIOS::GetVariable( const std::string name )
{ return m_UShort[ GetVariableIndex<unsigned short>(name) ]; }

template<> inline
Variable<int>& ADIOS::GetVariable( const std::string name )
{ return m_Int[ GetVariableIndex<int>(name) ]; }

template<> inline
Variable<unsigned int>& ADIOS::GetVariable( const std::string name )
{ return m_UInt[ GetVariableIndex<unsigned int>(name) ]; }

template<> inline
Variable<long int>& ADIOS::GetVariable( const std::string name )
{ return m_LInt[ GetVariableIndex<unsigned int>(name) ]; }

template<> inline
Variable<unsigned long int>& ADIOS::GetVariable( const std::string name )
{ return m_ULInt[ GetVariableIndex<unsigned long int>(name) ]; }

template<> inline
Variable<long long int>& ADIOS::GetVariable( const std::string name )
{ return m_LLInt[ GetVariableIndex<long long int>(name) ]; }

template<> inline
Variable<unsigned long long int>& ADIOS::GetVariable( const std::string name )
{ return m_ULLInt[ GetVariableIndex<unsigned long long int>(name) ]; }

template<> inline
Variable<float>& ADIOS::GetVariable( const std::string name )
{ return m_Float[ GetVariableIndex<float>(name) ]; }

template<> inline
Variable<double>& ADIOS::GetVariable( const std::string name )
{ return m_Double[ GetVariableIndex<double>(name) ]; }

template<> inline
Variable<long double>& ADIOS::GetVariable( const std::string name )
{ return m_LDouble[ GetVariableIndex<long double>(name) ]; }

template<> inline
Variable<std::complex<float>>& ADIOS::GetVariable( const std::string name )
{ return m_CFloat[ GetVariableIndex<std::complex<float>>(name) ]; }

template<> inline
Variable<std::complex<double>>& ADIOS::GetVariable( const std::string name )
{ return m_CDouble[ GetVariableIndex<std::complex<double>>(name) ]; }

template<> inline
Variable<std::complex<long double>>& ADIOS::GetVariable( const std::string name )
{ return m_CLDouble[ GetVariableIndex<std::complex<long double>>(name) ]; }



} //end namespace


#endif /* ADIOS_H_ */
