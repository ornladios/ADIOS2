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
#include <string>
#include <unordered_map>
#include <utility> //std::pair
#include <vector>
/// \endcond

#include "adios2/common/ADIOSConfig.h"
#include "adios2/common/ADIOSMacros.h"
#include "adios2/common/ADIOSTypes.h"
#include "adios2/core/ADIOS.h"
#include "adios2/core/Attribute.h"
#include "adios2/core/Variable.h"
#include "adios2/core/VariableCompound.h"

namespace adios2
{

namespace core
{

/** used for Variables and Attributes, name, type, type-index */
using DataMap =
    std::unordered_map<std::string, std::pair<std::string, unsigned int>>;

// forward declaration needed as IO is passed to Engine derived
// classes
class Engine;

/** Factory class IO for settings, variables, and transports to an engine */
class IO
{

public:
    /** reference to object that created current IO */
    ADIOS &m_ADIOS;

    /** unique identifier */
    const std::string m_Name;

    /** true: extra exceptions checks */
    const bool m_DebugMode = false;

    /** from ADIOS class passed to Engine created with Open */
    const std::string m_HostLanguage = "C++";

    /**
     * Map holding variable identifiers
     * <pre>
     * key: unique variable name,
     * value: pair.first = type as string GetType<T> from adiosTemplates.h
     *        pair.second = index in fixed size map (e.g. m_Int8, m_Double)
     * </pre>
     */
    DataMap m_Variables;

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

    /** From SetParameter, parameters for a particular engine from m_Type */
    Params m_Parameters;

    /** From AddTransport, parameters in map for each transport in vector */
    std::vector<Params> m_TransportsParameters;

    /** Carries information about operations added with AddOperation */
    struct Operation
    {
        Operator *Op;
        Params Parameters;
        Params Info;
    };

    /** From AddOperation, contains operators added to this IO */
    std::vector<Operation> m_Operations;

    /** BP3 engine default if unknown */
    std::string m_EngineType = "File";

    /** at read for file engines: true: in streaming (step-by-step) mode, or
     * false: random-access mode (files) */
    bool m_ReadStreaming = false;

    /** used if m_Streaming is true by file reader engines */
    size_t m_EngineStep = 0;

    /** placeholder when reading XML file variable operations, executed until
     * DefineVariable in code */
    std::map<std::string, std::vector<Operation>> m_VarOpsPlaceholder;

    /** true: prefix variables/attributes are cached per variable
     *   when function m_IsPrefixedNames called */
    bool m_IsPrefixedNames = false;

    /**
     * @brief Constructor called from ADIOS factory class DeclareIO function.
     * Not to be used direclty in applications.
     * @param adios reference to ADIOS object that owns current IO
     * @param name unique identifier for this IO object
     * @param inConfigFile IO defined in config file (XML)
     * @param hostLanguage current language using the adios2 library
     * @param debugMode true: extra exception checks (recommended)
     */
    IO(ADIOS &adios, const std::string name, const bool inConfigFile,
       const std::string hostLanguage, const bool debugMode);

    ~IO() = default;

    /**
     * @brief Sets the engine type for this IO class object
     * @param engine predefined engine type, default is bpfile
     */
    void SetEngine(const std::string engine) noexcept;

    /**
     * @brief Set the IO mode (collective or independent), not yet implemented
     * @param IO mode */
    void SetIOMode(const IOMode mode);

    /**
     * @brief Version that passes a map to fill out parameters
     * initializer list = { "param1", "value1" },  {"param2", "value2"},
     * @param params adios::Params std::map<std::string, std::string>
     */
    void SetParameters(const Params &parameters = Params()) noexcept;

    /**
     * @brief Version that passes a single string to fill out many parameters.
     * initializer string = "param1=value1 , param2 = value2"
     * This function will throw std::invalid_argument for entries that
     * cannot be parsed into key=value pairs.
     */
    void SetParameters(const std::string &parameters);

    /**
     * @brief Sets a single parameter overwriting value if key exists;
     * @param key parameter key
     * @param value parameter value
     */
    void SetParameter(const std::string key, const std::string value) noexcept;

    /** @brief Retrieve current parameters map */
    Params &GetParameters() noexcept;

    /** @brief Delete all parameters */
    void ClearParameters() noexcept;

    /**
     * @brief Adds a transport and its parameters for the IO Engine
     * @param type must be a supported transport type
     * @param params acceptable parameters for a particular transport
     * @return transportIndex handler
     */
    size_t AddTransport(const std::string type,
                        const Params &parameters = Params());

    /**
     * @brief Sets a single parameter to an existing transport identified with a
     * transportIndex handler from AddTransport.
     * This function overwrites existing parameter with the same key.
     * @param transportIndex index handler from AddTransport
     * @param key parameter key
     * @param value parameter value
     */
    void SetTransportParameter(const size_t transportIndex,
                               const std::string key, const std::string value);

    /**
     * @brief Define a Variable of primitive data type for current IO.
     * Default (name only) is a local single value,
     * in order to be compatible with ADIOS1.
     * @param name variable name, must be unique within Method
     * @param shape overall dimensions e.g. {Nx*size, Ny*size, Nz*size}
     * @param start point (offset) for MPI rank e.g. {Nx*rank, Ny*rank, Nz*rank}
     * @param count length for MPI rank e.g. {Nx, Ny, Nz}
     * @param constantShape true if dimensions, offsets and local sizes don't
     * change over time
     * @return reference to Variable object
     * @exception std::invalid_argument if Variable with unique name is already
     * defined, in debug mode only
     */
    template <class T>
    Variable<T> &
    DefineVariable(const std::string &name, const Dims &shape = Dims(),
                   const Dims &start = Dims(), const Dims &count = Dims(),
                   const bool constantDims = false);

    /**
     * @brief Define array attribute
     * @param name must be unique for the IO object
     * @param array pointer to user data
     * @param elements number of data elements
     * @param variableName optionally associates the attribute to a Variable
     * @return reference to internal Attribute
     * @exception std::invalid_argument if Attribute with unique name is already
     * defined, in debug mode only
     */
    template <class T>
    Attribute<T> &DefineAttribute(const std::string &name, const T *array,
                                  const size_t elements,
                                  const std::string &variableName = "",
                                  const std::string separator = "/");

    /**
     * @brief Define single value attribute
     * @param name must be unique for the IO object
     * @param value single data value
     * @return reference to internal Attribute
     * @exception std::invalid_argument if Attribute with unique name is already
     * defined, in debug mode only
     */
    template <class T>
    Attribute<T> &DefineAttribute(const std::string &name, const T &value,
                                  const std::string &variableName = "",
                                  const std::string separator = "/");

    /**
     * @brief Removes an existing Variable in current IO object.
     * Dangerous function since references and
     * pointers can be dangling after this call.
     * @param name unique identifier input
     * @return true: found and removed variable, false: not found, nothing to
     * remove
     */
    bool RemoveVariable(const std::string &name) noexcept;

    /**
     * @brief Removes all existing variables in current IO object.
     * Dangerous function since references and
     * pointers can be dangling after this call.
     */
    void RemoveAllVariables() noexcept;

    /**
     * @brief Removes an existing Attribute in current IO object.
     * Dangerous function since references and
     * pointers can be dangling after this call.
     * @param name unique identifier input
     * @return true: found and removed attribute, false: not found, nothing to
     * remove
     */
    bool RemoveAttribute(const std::string &name) noexcept;

    /**
     * @brief Removes all existing attributes in current IO object.
     * Dangerous function since references and
     * pointers can be dangling after this call.
     */
    void RemoveAllAttributes() noexcept;

    /**
     * @brief Retrieve map with variables info. Use when reading.
     * @return map with current variables and info
     * keys: Type, Min, Max, Value, AvailableStepsStart,
     * AvailableStepsCount, Shape, Start, Count, SingleValue
     */
    std::map<std::string, Params> GetAvailableVariables() noexcept;

    /**
     * @brief Gets an existing variable of primitive type by name
     * @param name of variable to be retrieved
     * @return pointer to an existing variable in current IO, nullptr if not
     * found
     */
    template <class T>
    Variable<T> *InquireVariable(const std::string &name) noexcept;

    /**
     * @brief Returns the type of an existing variable as an string
     * @param name input variable name
     * @return type primitive type
     */
    std::string InquireVariableType(const std::string &name) const noexcept;

    /**
     * Overload that accepts a const iterator into the m_Variables map if found
     * @param itVariable
     * @return type primitive type
     */
    std::string
    InquireVariableType(const DataMap::const_iterator itVariable) const
        noexcept;

    /**
     * Retrieves hash holding internal variable identifiers
     * @return
     * <pre>
     * key: unique variable name,
     * value: pair.first = string type
     *        pair.second = order in the type bucket
     * </pre>
     */
    const DataMap &GetVariablesDataMap() const noexcept;

    /**
     * Retrieves hash holding internal Attributes identifiers
     * @return
     * <pre>
     * key: unique attribute name,
     * value: pair.first = string type
     *        pair.second = order in the type bucket
     * </pre>
     */
    const DataMap &GetAttributesDataMap() const noexcept;

    /**
     * Gets an existing attribute of primitive type by name
     * @param name of attribute to be retrieved
     * @return pointer to an existing attribute in current IO, nullptr if not
     * found
     */
    template <class T>
    Attribute<T> *InquireAttribute(const std::string &name,
                                   const std::string &variableName = "",
                                   const std::string separator = "/") noexcept;

    /**
     * @brief Returns the type of an existing attribute as an string
     * @param name input attribute name
     * @return type if found returns type as string, otherwise an empty string
     */
    std::string InquireAttributeType(const std::string &name,
                                     const std::string &variableName = "",
                                     const std::string separator = "/") const
        noexcept;

    /**
     * @brief Retrieve map with attributes info. Use when reading.
     * @return map with current attributes and info
     * keys: Type, Elements, Value
     */
    std::map<std::string, Params>
    GetAvailableAttributes(const std::string &variableName = std::string(),
                           const std::string separator = "/",
                           const bool fullNamesKeys = false) noexcept;

    /**
     * @brief Check existence in config file passed to ADIOS class constructor
     * @return true: defined in config file, false: not found in config file
     */
    bool InConfigFile() const noexcept;

    /**
     * Sets declared to true if IO exists in code created with ADIOS DeclareIO
     */
    void SetDeclared() noexcept;

    /**
     * Check if declared in code
     * @return true: created with ADIOS DeclareIO, false: dummy from config file
     */
    bool IsDeclared() const noexcept;

    /**
     * Adds an operator defined by the ADIOS class. Could be a variable set
     * transform, callback function, etc.
     * @param op operator created by the ADIOS class
     * @param parameters specific parameters for current IO
     * @return operation handler
     */
    size_t AddOperation(Operator &op,
                        const Params &parameters = Params()) noexcept;

    /**
     * @brief Creates a polymorphic object that derives the Engine class,
     * based on the SetEngine function or config file input
     * @param name unique engine identifier within IO object
     * (e.g. file name in case of Files)
     * @param mode write, read, append from ADIOSTypes.h Mode
     * @param mpiComm assigns a new communicator to the Engine
     * @return a reference to a derived object of the Engine class
     * @exception std::invalid_argument if Engine with unique name is already
     * created with another Open, in debug mode only
     */
    Engine &Open(const std::string &name, const Mode mode, helper::Comm comm);

    /**
     * Overloaded version that reuses the MPI_Comm object passed
     * from the ADIOS class to the IO class
     * @param name unique engine identifier within IO object
     * (file name in case of File transports)
     * @param mode write, read, append from ADIOSTypes.h OpenMode
     * @return a reference to a derived object of the Engine class
     * @exception std::invalid_argument if Engine with unique name is already
     * created with another Open, in debug mode only
     */
    Engine &Open(const std::string &name, const Mode mode);

    /**
     * Retrieve an engine by name
     */
    Engine &GetEngine(const std::string &name);

    /**
     * Flushes all engines created with the current IO object using Open.
     * If no engine is created it does nothing.
     * @exception std::runtime_error if any engine Flush fails
     */
    void FlushAll();

    // READ FUNCTIONS, not yet implemented:
    /**
     * not yet implented
     * @param pattern
     */
    void SetReadMultiplexPattern(const ReadMultiplexPattern pattern);

    /**
     * not yet implemented
     * @param mode
     */
    void SetStreamOpenMode(const StreamOpenMode mode);

    /**
     * Resets all variables m_StepsStart and m_StepsCount
     * @param alwaysZero true: always m_StepsStart = 0, false: capture
     */
    void ResetVariablesStepSelection(const bool zeroStart = false,
                                     const std::string hint = "");

    void SetPrefixedNames(const bool isStep) noexcept;

    /** Gets the internal reference to a variable map for type T */
    template <class T>
    std::map<unsigned int, Variable<T>> &GetVariableMap() noexcept;

    /** Gets the internal reference to an attribute map for type T */
    template <class T>
    std::map<unsigned int, Attribute<T>> &GetAttributeMap() noexcept;

private:
    /** true: exist in config file (XML) */
    const bool m_InConfigFile = false;

    bool m_IsDeclared = false;

    /** Independent (default) or Collective */
    adios2::IOMode m_IOMode = adios2::IOMode::Independent;

/** Variable containers based on fixed-size type */
#define declare_map(T, NAME) std::map<unsigned int, Variable<T>> m_##NAME;
    ADIOS2_FOREACH_STDTYPE_2ARGS(declare_map)
#undef declare_map

#define declare_map(T, NAME) std::map<unsigned int, Attribute<T>> m_##NAME##A;
    ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_2ARGS(declare_map)
#undef declare_map

    std::map<unsigned int, VariableCompound> m_Compound;

    std::map<std::string, std::shared_ptr<Engine>> m_Engines;

    /**
     * Gets map index for Variables or Attributes
     * @param name
     * @param dataMap m_Variables or m_Attributes
     * @return index in type map, -1 if not found
     */
    int GetMapIndex(const std::string &name, const DataMap &dataMap) const
        noexcept;

    /** Checks if attribute exists, called from DefineAttribute different
     *  signatures */
    void CheckAttributeCommon(const std::string &name) const;

    /**
     * Checks if iterator points to end. Used for Variables and Attributes.
     * @param itDataMap iterator to be tested
     * @param dataMap map
     * @return true: itDataMap == dataMap.end(), false otherwise
     */
    bool IsEnd(DataMap::const_iterator itDataMap, const DataMap &dataMap) const;

    void CheckTransportType(const std::string type) const;

    template <class T>
    bool IsAvailableStep(const size_t step,
                         const unsigned int variableIndex) noexcept;
};

// Explicit declaration of the public template methods
#define declare_template_instantiation(T)                                      \
    extern template Variable<T> &IO::DefineVariable<T>(                        \
        const std::string &, const Dims &, const Dims &, const Dims &,         \
        const bool);                                                           \
    extern template Variable<T> *IO::InquireVariable<T>(                       \
        const std::string &name) noexcept;

ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

#define declare_template_instantiation(T)                                      \
    extern template Attribute<T> &IO::DefineAttribute<T>(                      \
        const std::string &, const T *, const size_t, const std::string &,     \
        const std::string);                                                    \
    extern template Attribute<T> &IO::DefineAttribute<T>(                      \
        const std::string &, const T &, const std::string &,                   \
        const std::string);                                                    \
    extern template Attribute<T> *IO::InquireAttribute<T>(                     \
        const std::string &, const std::string &, const std::string) noexcept;

ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_IO_H_ */
