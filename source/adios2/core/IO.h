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
#include "adios2/core/Attribute.h"
#include "adios2/core/Variable.h"
#include "adios2/core/VariableCompound.h"

namespace adios2
{

/** used for Variables and Attributes */
using DataMap = std::map<std::string, std::pair<std::string, unsigned int>>;

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

    /**
     * Gets the engine type for this IO class object
     * @param engine
     */
    const std::string GetEngine() const;

    /** Set the IO mode (collective or independent)
     * @param IO mode */
    void SetIOMode(const IOMode mode);

    /**
     * Get the IO mode (collective or independent)
     */
    const IOMode GetIOMode() const;

    /**
     * Version that passes a map to fill out parameters
     * initializer list = { "param1", "value1" },  {"param2", "value2"},
     * @param params adios::Params std::map<std::string, std::string>
     */
    void SetParameters(const Params &parameters = Params());

    /**
     * Sets a single parameter overwriting value if key exists;
     * @param key parameter key
     * @param value parameter value
     */
    void SetSingleParameter(const std::string key,
                            const std::string value) noexcept;

    /**
     * Retrieve existing parameter set
     */
    Params &GetParameters();
    const Params &GetParameters() const;

    /**
     * Adds a transport and its parameters for the IO Engine
     * @param type must be a supported transport type
     * @param params acceptable parameters for a particular transport
     * @return transportIndex handler
     */
    unsigned int AddTransport(const std::string type,
                              const Params &params = Params());

    const std::vector<Params> &GetTransportParameters() const;

    /**
     * Set a single parameter to an existing transport identified with a
     * transportIndex handler from AddTransport. This function overwrites
     * existing parameter.
     * @param transportIndex index handler from AddTransport
     * @param key parameter key
     * @param value parameter value
     */
    void SetTransportSingleParameter(const unsigned int transportIndex,
                                     const std::string key,
                                     const std::string value);

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
    Variable<T> &
    DefineVariable(const std::string &name, const Dims &shape = Dims{},
                   const Dims &start = Dims{}, const Dims &count = Dims{},
                   const bool constantDims = false);

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
    VariableCompound &DefineVariableCompound(const std::string &name,
                                             const Dims &shape = Dims{},
                                             const Dims &start = Dims{},
                                             const Dims &count = Dims{},
                                             const bool constantDims = false);

    VariableCompound &DefineVariableCompound(const std::string &name,
                                             const size_t sizeOfVariable,
                                             const Dims &shape = Dims{},
                                             const Dims &start = Dims{},
                                             const Dims &count = Dims{},
                                             const bool constantDims = false);

    /**
     * Define attribute from contiguous data array owned by an application
     * @param name must be unique for the IO object
     * @param array pointer to user data
     * @param elements number of data elements
     * @return reference to internal Attribute
     */
    template <class T>
    Attribute<T> &DefineAttribute(const std::string &name, const T *array,
                                  const size_t elements);

    /**
     * Define attribute from a single variable making a copy
     * @param name must be unique for the IO object
     * @param value single data value
     * @return reference to internal Attribute
     */
    template <class T>
    Attribute<T> &DefineAttribute(const std::string &name, const T &value);

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
     * Runtime function: return a pointer to VariableBase
     * @param name unique variable identifier
     * @return nullptr if not found, pointer to VariableBase if variable is
     * found
     */
    VariableBase *GetVariableBase(const std::string &name) noexcept;

    /**
     * Gets an existing variable of compound type by name
     * @param name of variable to be retrieved
     * @return reference to an existing variable created with DefineVariable
     * throws an exception if VariableCompound is not found
     */
    VariableCompound &GetVariableCompound(const std::string &name);

    /**
     * Return  map with attributes name and type info
     * @return m_Attributes
     */
    const DataMap &GetAttributesDataMap() const noexcept;

    /**
     * Gets an existing attribute of primitive type by name
     * @param name of attribute to be retrieved
     * @return reference to an existing attribute created with DefineAttribute
     * throws an exception if Attribute is not found
     */
    template <class T>
    Attribute<T> &GetAttribute(const std::string &name);

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
    DataMap m_Variables;

    /** Variable containers based on fixed-size type */
    std::map<unsigned int, Variable<char>> m_Char;
    std::map<unsigned int, Variable<signed char>> m_SChar;
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

    /** Gets the internal reference to a variable map for type T
     *  This function is specialized in IO.tcc */
    template <class T>
    std::map<unsigned int, Variable<T>> &GetVariableMap();

    /**
     * Map holding attribute identifiers
     * <pre>
     * key: unique attribute name,
     * value: pair.first = type as string GetType<T> from
     *                     helper/adiosTemplates.h
     *        pair.second = index in fixed size map (e.g. m_Int8, m_Double)
     * </pre>
     */
    DataMap m_Attributes;

    std::map<unsigned int, Attribute<std::string>> m_StringA;
    std::map<unsigned int, Attribute<char>> m_CharA;
    std::map<unsigned int, Attribute<signed char>> m_SCharA;
    std::map<unsigned int, Attribute<unsigned char>> m_UCharA;
    std::map<unsigned int, Attribute<short>> m_ShortA;
    std::map<unsigned int, Attribute<unsigned short>> m_UShortA;
    std::map<unsigned int, Attribute<int>> m_IntA;
    std::map<unsigned int, Attribute<unsigned int>> m_UIntA;
    std::map<unsigned int, Attribute<long int>> m_LIntA;
    std::map<unsigned int, Attribute<unsigned long int>> m_ULIntA;
    std::map<unsigned int, Attribute<long long int>> m_LLIntA;
    std::map<unsigned int, Attribute<unsigned long long int>> m_ULLIntA;
    std::map<unsigned int, Attribute<float>> m_FloatA;
    std::map<unsigned int, Attribute<double>> m_DoubleA;
    std::map<unsigned int, Attribute<long double>> m_LDoubleA;

    template <class T>
    std::map<unsigned int, Attribute<T>> &GetAttributeMap();

    /**
     * Gets map index for Variables or Attributes
     * @param name
     * @param dataMap m_Variables or m_Attributes
     * @param hint "Variable", "Attribute", or "VariableCompound"
     * @return index in type map
     */
    unsigned int GetMapIndex(const std::string &name, const DataMap &dataMap,
                             const std::string hint) const;

    /** Checks if attribute exists, called from DefineAttribute different
     * signatures */
    void CheckAttributeCommon(const std::string &name) const;

    std::set<std::string> m_EngineNames;

    /**
     * Checks if iterator points to end. Used for Variables and Attributes.
     * @param itDataMap iterator to be tested
     * @param dataMap map
     * @return true: itDataMap == dataMap.end(), false otherwise
     */
    bool IsEnd(DataMap::const_iterator itDataMap, const DataMap &dataMap) const;

    void CheckTransportType(const std::string type) const;
};

// Explicit declaration of the public template methods
#define declare_template_instantiation(T)                                      \
    extern template Variable<T> &IO::DefineVariable<T>(                        \
        const std::string &, const Dims &, const Dims &, const Dims &,         \
        const bool);                                                           \
    extern template Variable<T> &IO::GetVariable<T>(const std::string &name);

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

#define declare_template_instantiation(T)                                      \
    extern template Attribute<T> &IO::DefineAttribute<T>(                      \
        const std::string &, const T *, const size_t);                         \
    extern template Attribute<T> &IO::DefineAttribute<T>(const std::string &,  \
                                                         const T &);           \
    extern template Attribute<T> &IO::GetAttribute(const std::string &);

ADIOS2_FOREACH_ATTRIBUTE_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace adios2

#include "adios2/core/IO.inl"

#endif /* ADIOS2_CORE_IO_H_ */
