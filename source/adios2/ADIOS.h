/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS.h
 *  Created on: Oct 3, 2016
 *      Author: wfg
 */

#ifndef ADIOS_H_
#define ADIOS_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <complex>
#include <map>
#include <memory> //std::shared_ptr
#include <ostream>
#include <set>
#include <string>
#include <vector>
/// \endcond

#include "adios2/ADIOSConfig.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/ADIOS_MPI.h"
#include "adios2/core/Method.h"
#include "adios2/core/Support.h"
#include "adios2/core/Transform.h"
#include "adios2/core/Variable.h"
#include "adios2/core/VariableCompound.h"

namespace adios
{

class Engine;

/**
 * @brief Unique class interface between user application and ADIOS library
 */
class ADIOS
{
public:
    /**
     * Passed from parallel constructor, MPI_Comm is a pointer itself.
     *  Public as called from C
     */
    MPI_Comm m_MPIComm = MPI_COMM_SELF;

    int m_RankMPI = 0; ///< current MPI rank process
    int m_SizeMPI = 1; ///< current MPI processes size

    std::string m_HostLanguage = "C++"; ///< changed by language bindings

    /**
     * @brief ADIOS empty constructor. Used for non XML config file API calls.
     */
    ADIOS(const Verbose verbose = Verbose::WARN, const bool debugMode = false);

    /**
     * @brief Serial constructor for config file, only allowed and compiled in
     * libadios_nompi.a
     * @param configFile XML config file (maybe support different formats in the
     * future?)
     * @param debugMode true: on throws exceptions and do additional checks,
     * false: off (faster, but unsafe)
     */
    ADIOS(const std::string configFile, const Verbose verbose = Verbose::WARN,
          const bool debugMode = false);

    /**
     * @brief Parallel constructor for XML config file and MPI
     * @param config XML config file (maybe support different formats in the
     * future?)
     * @param mpiComm MPI communicator ...const to be discussed
     * @param debugMode true: on, false: off (faster, but unsafe)
     */
    ADIOS(const std::string configFile, MPI_Comm mpiComm,
          const Verbose verbose = Verbose::WARN, const bool debugMode = false);

    /**
     * @brief Parallel MPI communicator without XML config file
     * @param mpiComm MPI communicator passed to m_MPIComm*
     * @param debugMode true: on, false: off (faster)
     */
    ADIOS(MPI_Comm mpiComm, const Verbose verbose = Verbose::WARN,
          const bool debugMode = false);

    ~ADIOS() = default;

    void InitMPI(); ///< sets rank and size in m_rank and m_Size, respectively.

    /**
     * Define a Variable for I/O. Default is a local scalar to be compatible
     * with ADIOS1
     * @param name variable name, must be unique
     * @param dimensions
     * @param selections
     * @param offsets
     * @return reference to Variable object
     */
    template <class T>
    Variable<T> &DefineVariable(const std::string &name,
                                const Dims localDimensions = Dims{1},
                                const Dims globalDimensions = Dims{},
                                const Dims offsets = Dims{});

    template <class T>
    Variable<T> &GetVariable(const std::string &name);

    template <class T>
    VariableCompound &DefineVariableCompound(
        const std::string &name, const Dims globalDimensions = Dims{},
        const Dims localDimensions = Dims{1}, const Dims offsets = Dims{});

    VariableCompound &GetVariableCompound(const std::string &name);

    /**
     * Declares a new method. If the method is defined in the user config file,
     * it will be already created during processing the config file,
     * the method is set up with the user settings and this function just
     * returns
     * that method.
     * Otherwise it will create and return a new Method with default settings.
     * Use method.isUserDefined() to distinguish between the two cases.
     * @param methodName must be unique
     */
    Method &DeclareMethod(const std::string methodName);

    /**
     * @brief Open to Write, Read. Creates a new engine from previously defined
     * method
     * @param streamName unique stream or file name
     * @param accessMode "w" or "write", "r" or "read", "a" or "append", "u" or
     * "update"
     * @param mpiComm option to modify communicator from ADIOS class constructor
     * @param method looks for corresponding Method object in ADIOS to
     * initialize
     * the engine
     * @param iomode Independent or collective open/advance by writers/readers?
     * Write() operations are always independent.
     * @param timeout_sec Wait some time before reporting on missing stream
     * (i.e.
     * wait for it for a while)
      * @return Derived class of base Engine depending on Method parameters,
     * shared_ptr for potential flexibility
     */
    std::shared_ptr<Engine> Open(const std::string &streamName,
                                 const std::string accessMode, MPI_Comm mpiComm,
                                 const Method &method);

    /**
     * @brief Open to Write, Read. Creates a new engine from previously defined
     * method.
     * Reuses MPI communicator from ADIOS constructor.
     * @param streamName unique stream or file name
     * @param accessMode "w" or "write", "r" or "read", "a" or "append", "u" or
     * "update"
     * @param method contains engine parameters
     * @param iomode Independent or collective open/advance by writers/readers?
     * Write() operations are always independent.
     * @param timeout_sec Wait some time before reporting on missing stream
     * (i.e.
     * wait for it for a while)
      * @return Derived class of base Engine depending on Method parameters,
     * shared_ptr for potential flexibility
     */
    std::shared_ptr<Engine> Open(const std::string &streamName,
                                 const std::string accessMode,
                                 const Method &method);

    /**
     * Version required by the XML config file implementation, searches method
     * inside ADIOS through a unique name
     * @param streamName unique stream or file name
     * @param accessMode "w" or "write", "r" or "read", "a" or "append"
     * @param mpiComm mpi Communicator
     * @param methodName used to search method object inside ADIOS object
     * @param iomode Independent or collective open/advance by writers/readers?
     * Write() operations are always independent.
     * @param timeout_sec Wait some time before reporting on missing stream
     * (i.e.
     * wait for it for a while)
     * @return Derived class of base Engine depending on Method parameters,
     * shared_ptr for potential flexibility
     */
    std::shared_ptr<Engine> Open(const std::string &streamName,
                                 const std::string accessMode, MPI_Comm mpiComm,
                                 const std::string methodName);

    /**
     * Version required by the XML config file implementation, searches method
     * inside ADIOS through a unique name.
     * Reuses ADIOS MPI Communicator from constructor.
     * @param streamName unique stream or file name
     * @param accessMode "w" or "write", "r" or "read", "a" or "append"
     * @param methodName used to search method object inside ADIOS object
     * @param iomode Independent or collective open/advance by writers/readers?
     * Write() operations are always independent.
     * @param timeout_sec Wait some time before reporting on missing stream
     * (i.e.
     * wait for it for a while)
     * @return Derived class of base Engine depending on Method parameters,
     * shared_ptr for potential flexibility
     */
    std::shared_ptr<Engine> Open(const std::string &streamName,
                                 const std::string accessMode,
                                 const std::string methodName);

    /**
     * @brief Open to Read all steps from a file. No streaming, advancing is
     * possible here. All steps in the file
     * are immediately available for reading. Creates a new engine from
     * previously
     * defined method.
     * @param fileName file name
     * @param mpiComm option to modify communicator from ADIOS class constructor
     * @param method looks for corresponding Method object in ADIOS to
     * initialize
     * the engine
     * @param iomode Independent or collective open/advance by writers/readers?
     * Write() operations are always independent.
     * @return Derived class of base Engine depending on Method parameters,
     * shared_ptr for potential flexibility
     */
    std::shared_ptr<Engine> OpenFileReader(const std::string &fileName,
                                           MPI_Comm mpiComm,
                                           const Method &method);

    /**
     * @brief Open to Read all steps from a file. No streaming, advancing is
     * possible here. All steps in the file
     * are immediately available for reading. Creates a new engine from
     * previously
     * defined method.
     * Version required by the XML config file implementation, searches method
     * inside ADIOS through a unique name.
     * @param fileName file name
     * @param mpiComm option to modify communicator from ADIOS class constructor
     * @param methodName used to search method object inside ADIOS object
      * @param iomode Independent or collective open/advance by writers/readers?
     * Write() operations are always independent.
     * @return Derived class of base Engine depending on Method parameters,
     * shared_ptr for potential flexibility
     */
    std::shared_ptr<Engine> OpenFileReader(const std::string &fileName,
                                           MPI_Comm mpiComm,
                                           const std::string methodName);

    /**
     * @brief Dumps groups information to a file stream or standard output.
     * Note that either the user closes this fileStream or it's closed at the
     * end.
     * @param logStream either std::cout standard output, or a std::ofstream
     * file
     */
    void MonitorVariables(std::ostream &logStream);

protected: // no const to allow default empty and copy constructors
    std::map<unsigned int, Variable<char>> m_Char;
    std::map<unsigned int, Variable<unsigned char>> m_UChar;
    std::map<unsigned int, Variable<short>> m_Short;
    std::map<unsigned int, Variable<unsigned short>> m_UShort;
    std::map<unsigned int, Variable<int>> m_Int;
    std::map<unsigned int, Variable<unsigned int>> m_UInt;
    std::map<unsigned int, Variable<long int>> m_LInt;
    std::map<unsigned int, Variable<unsigned long int>> m_ULInt;
    std::map<unsigned int, Variable<long long int>> m_LLInt;
    std::map<unsigned int, Variable<unsigned long long int>> m_ULLInt;
    std::map<unsigned int, Variable<float>> m_Float;
    std::map<unsigned int, Variable<double>> m_Double;
    std::map<unsigned int, Variable<long double>> m_LDouble;
    std::map<unsigned int, Variable<std::complex<float>>> m_CFloat;
    std::map<unsigned int, Variable<std::complex<double>>> m_CDouble;
    std::map<unsigned int, Variable<std::complex<long double>>> m_CLDouble;
    std::map<unsigned int, VariableCompound> m_Compound;

    ///< XML File to be read containing configuration information
    std::string m_ConfigFile;

    ///< if true will do more checks, exceptions, warnings, expect slower code
    bool m_DebugMode = false;

    // Variables
    ///< Makes variable name unique, key: variable name,value: pair.first =
    /// type, pair.second = index in corresponding vector of Variable
    std::map<std::string, std::pair<std::string, unsigned int>> m_Variables;

    ///< transforms associated with ADIOS run
    std::vector<std::shared_ptr<Transform>> m_Transforms;

    /**
     * @brief List of Methods (engine metadata) defined from either ADIOS XML
     * configuration file or the DeclareMethod function.
     * <pre>
     *     Key: std::string unique method name
     *     Value: Method class
     * </pre>
     */
    std::map<std::string, Method> m_Methods;

    ///< set used to check Engine name uniqueness in debug mode
    std::set<std::string> m_EngineNames;

    /**
     * @brief Checks for group existence in m_Groups, if failed throws
     * std::invalid_argument exception
     * @param itGroup m_Groups iterator, usually from find function
     * @param groupName unique name, passed for thrown exception only
     * @param hint adds information to thrown exception
     */
    void CheckVariableInput(const std::string &name,
                            const Dims &dimensions) const;

    /**
     * Checks for variable name, if not found throws an invalid exception
     * @param itVariable iterator pointing to the variable name in m_Variables
     * @param name variable name
     * @param hint message to be thrown for debugging purporses
     */
    void CheckVariableName(
        std::map<std::string,
                 std::pair<std::string, unsigned int>>::const_iterator
            itVariable,
        const std::string &name, const std::string hint) const;

    /**
     * @brief Checks for method existence in m_Methods, if failed throws
     * std::invalid_argument exception
     * @param itMethod m_Methods iterator, usually from find function
     * @param methodName unique name, passed for thrown exception only
     * @param hint adds information to thrown exception
     */
    void CheckMethod(std::map<std::string, Method>::const_iterator itMethod,
                     const std::string methodName,
                     const std::string hint) const;

    template <class T>
    unsigned int GetVariableIndex(const std::string &name);

    // Helper function for DefineVariable
    template <class T>
    std::map<unsigned int, Variable<T>> &GetVariableMap();
};

//------------------------------------------------------------------------------

// Explicit declaration of the template methods
#define declare_template_instantiation(T)                                      \
    extern template Variable<T> &ADIOS::DefineVariable<T>(                     \
        const std::string &name, const Dims, const Dims, const Dims);          \
                                                                               \
    extern template Variable<T> &ADIOS::GetVariable<T>(const std::string &);

ADIOS_FOREACH_TYPE_1ARG(declare_template_instantiation)
extern template unsigned int ADIOS::GetVariableIndex<void>(const std::string &);
#undef declare_template_instantiation

} // end namespace adios

// Include the inline implementations for the public interface
#include "adios2/ADIOS.inl"

#endif /* ADIOS_H_ */
