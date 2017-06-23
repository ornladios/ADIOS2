/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * IO.h factory class of Parameters, Variables, Transports to Engines
 *
 *  Created on: Dec 16, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_IO_H_
#define ADIOS2_CORE_IO_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <map>
#include <memory> //std:shared_ptr
#include <set>
#include <string>
#include <utility> //std::pair
#include <vector>
/// \endcond

#include "adios2/ADIOSConfig.h"
#include "adios2/ADIOSMPICommOnly.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/core/Variable.h"
#include "adios2/core/VariableCompound.h"

namespace adios2
{

// forward declaration needed as IO is passed to Engine derived
// classes
class Engine;

/** Factory class IO for settings, variables, and transports to an engine */
class IO
{

public:
    /** unique identifier */
    const std::string m_Name;

    /** from ADIOS class passed to Engine created with Open
     *  if no new communicator is passed */
    MPI_Comm m_MPIComm;

    /** true: extra exceptions checks */
    const bool m_DebugMode = false;

    /** from ADIOS class passed to Engine created with Open */
    std::string m_HostLanguage = "C++";

    /** From SetParameter, parameters for a particular engine from m_Type */
    Params m_Parameters;

    /** From AddTransport, parameters in map for each transport in vector */
    std::vector<Params> m_TransportsParameters;

    /**
     * Constructor called from ADIOS factory class
     * @param name unique identifier for this IO object
     * @param mpiComm MPI communicator from ADIOS factory class
     * @param inConfigFile IO defined in config file (XML)
     * @param debugMode true: extra exception checks (recommended)
     */
    IO(const std::string name, MPI_Comm mpiComm, const bool inConfigFile,
       const bool debugMode);

    ~IO() = default;

    /**
     * Sets the engine type for this IO class object
     * @param engine
     */
    void SetEngine(const std::string engine);

    /** Set the IO mode (collective or independent)
     * @param IO mode */
    void SetIOMode(const IOMode mode);

    /**
     * Version that passes a map to fill out parameters
     * initializer list = { "param1", "value1" },  {"param2", "value2"},
     * @param params adios::Params std::map<std::string, std::string>
     */
    void SetParameters(const Params &parameters = Params());

    /**
     * Retrieve existing parameter set
     */
    const Params &GetParameters() const;

    /**
     * Adds a transport and its parameters for the method
     * @param type must be a supported transport type under /include/transport
     * @param args list of parameters for a transport with format
     * "parameter1=value1", ..., "parameterN=valueN"
     */
    unsigned int AddTransport(const std::string type,
                              const std::vector<std::string> &paramsVector);

    unsigned int AddTransport(const std::string type,
                              const Params &params = Params());

    /**
     * Define a Variable of primitive data type for I/O.
     * Default (name only) is a local single value,
     * in order to be compatible with ADIOS1.
     * @param name variable name, must be unique within Method
     * @param shape overall dimensions e.g. {Nx*size, Ny*size, Nz*size}
     * @param start point (offset) for MPI rank e.g. {Nx*rank, Ny*rank, Nz*rank}
     * @param count length for MPI rank e.g. {Nx, Ny, Nz}
     * @param constantShape true if dimensions, offsets and local sizes don't
     * change over time
     * @return reference to Variable object
     */
    template <class T>
    Variable<T> &DefineVariable(const std::string &name, const Dims shape = {},
                                const Dims start = {}, const Dims count = {},
                                const bool constantShape = false);

    /**
     * Define a Variable of primitive data type for I/O.
     * Default (name only) is a local single value,
     * in order to be compatible with ADIOS1.
     * @param name variable name, must be unique within Method
     * @param shape overall dimensions e.g. {Nx*size, Ny*size, Nz*size}
     * @param start point (offset) for MPI rank e.g. {Nx*rank, Ny*rank, Nz*rank}
     * @param count length for MPI rank e.g. {Nx, Ny, Nz}
     * @param constantShape true if dimensions, offsets and local sizes don't
     * change over time
     * @return reference to Variable object
     */

    template <class T>
    VariableCompound &
    DefineVariableCompound(const std::string &name, const Dims shape = Dims{},
                           const Dims start = Dims{}, const Dims count = Dims{},
                           const bool constantShape = false);

    /**
     * Removes an existing Variable previously created with DefineVariable or
     * DefineVariableCompound
     * @param name
     * @return true: found and removed variable, false: not found, nothing to
     * remove
     */
    bool RemoveVariable(const std::string &name) noexcept;

    /**
     * Gets an existing variable of primitive type by name
     * @param name of variable to be retrieved
     * @return reference to an existing variable created with DefineVariable
     * throws an exception if Variable is not found
     */
    template <class T>
    Variable<T> &GetVariable(const std::string &name);

    /**
     * Gets an existing variable of compound type by name
     * @param name of variable to be retrieved
     * @return reference to an existing variable created with DefineVariable
     * throws an exception if VariableCompound is not found
     */
    VariableCompound &GetVariableCompound(const std::string &name);

    /**
     * Get the type if variable (by name id) exists
     * @param name input id
     * @return type as string, if not found returns an empty string
     */
    std::string GetVariableType(const std::string &name) const;

    /**
     * Check existence in config file passed to ADIOS class
     * @return true: defined in config file
     */
    bool InConfigFile() const;

    /**
     * Creates a polymorphic object that derives the Engine class,
     * based on the SetEngine function or config file input
     * @param name unique engine identifier within IO object
     * (file name in case of File transports)
     * @param openMode write, read, append from ADIOSTypes.h OpenMode
     * @param mpiComm assigns a new communicator to the Engine
     * @return a smart pointer to a derived object of the Engine class
     */
    std::shared_ptr<Engine> Open(const std::string &name,
                                 const OpenMode openMode, MPI_Comm mpiComm);

    /**
     * Overloaded version that reuses the MPI_Comm object passed
     * from the ADIOS class to the IO class
     * @param name unique engine identifier within IO object
     * (file name in case of File transports)
     * @param openMode write, read, append from ADIOSTypes.h OpenMode
     * @return a smart pointer to a derived object of the Engine class
     */
    std::shared_ptr<Engine> Open(const std::string &name,
                                 const OpenMode openMode);

    // READ FUNCTIONS:
    void SetReadMultiplexPattern(const ReadMultiplexPattern pattern);
    void SetStreamOpenMode(const StreamOpenMode mode);

private:
    /** true: exist in config file (XML) */
    const bool m_InConfigFile = false;

    /** BPFileWriter engine default if unknown */
    std::string m_EngineType;

    /** Independent (default) or Collective */
    adios2::IOMode m_IOMode = adios2::IOMode::Independent;

    // Variables
    /**
     * Map holding variable identifiers
     * <pre>
     * key: unique variable name,
     * value: pair.first = type as string GetType<T> from adiosTemplates.h
     *        pair.second = index in fixed size map (e.g. m_Int8, m_Double)
     * </pre>
     */
    std::map<std::string, std::pair<std::string, unsigned int>> m_Variables;

    /** Variable containers based on fixed-size type */
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
    std::map<unsigned int, Variable<cfloat>> m_CFloat;
    std::map<unsigned int, Variable<cdouble>> m_CDouble;
    std::map<unsigned int, Variable<cldouble>> m_CLDouble;
    std::map<unsigned int, VariableCompound> m_Compound;

    std::map<std::string, std::string> m_AttributesString;
    std::map<std::string, double> m_AttributesNumeric;

    std::set<std::string> m_EngineNames;

    /**
     * Called from AddTransport overloads
     * @param type
     * @param parameters
     * @return transport index
     */
    unsigned int AddTransportCommon(const std::string type, Params &parameters);

    /** Gets the internal reference to a variable map for type T
     *  This function is specialized in IO.tcc */
    template <class T>
    std::map<unsigned int, Variable<T>> &GetVariableMap();

    /** Gets the internal index in variable map for an existing variable */
    unsigned int GetVariableIndex(const std::string &name) const;

    /**
     * Checks if variable exists by checking its name
     * @param name unique variable name to be checked against existing variables
     * @return true: variable name exists, false: variable name doesn't exist
     */
    bool VariableExists(const std::string &name) const;

    void CheckTransportType(const std::string type) const;
};

// Explicit declaration of the public template methods
#define declare_template_instantiation(T)                                      \
    extern template Variable<T> &IO::DefineVariable<T>(                        \
        const std::string &name, const Dims, const Dims, const Dims,           \
        const bool constantShape);                                             \
    extern template Variable<T> &IO::GetVariable<T>(const std::string &name);

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace adios

#include "adios2/core/IO.inl"

#endif /* ADIOS2_CORE_IO_H_ */
