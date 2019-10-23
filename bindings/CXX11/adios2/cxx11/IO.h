/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * IO.h :
 *
 *  Created on: Jun 4, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_CXX11_CXX11_IO_H_
#define ADIOS2_BINDINGS_CXX11_CXX11_IO_H_

#include "Attribute.h"
#include "Engine.h"
#include "Operator.h"
#include "Variable.h"

#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#endif

#include "adios2/common/ADIOSMacros.h"
#include "adios2/common/ADIOSTypes.h"

namespace adios2
{

/// \cond EXCLUDE_FROM_DOXYGEN
// forward declare
class ADIOS; // friend

namespace core
{
class IO; // private implementation
}
/// \endcond

class IO
{
    friend class ADIOS;

public:
    /**
     * Empty (default) constructor, use it as a placeholder for future
     * IO objects from ADIOS::IO functions.
     * Can be used with STL containers.
     */
    IO() = default;

    /** Use RAII */
    ~IO() = default;

    /** true: valid object, false: invalid object */
    explicit operator bool() const noexcept;

    /**
     * Inspects IO name
     * @return name
     */
    std::string Name() const;

    /**
     * @brief Checks if IO exists in a config file passed to ADIOS object that
     * created this IO
     * @return true: in config file, false: not in config file
     */
    bool InConfigFile() const;

    /**
     * @brief Sets the engine type for current IO object
     * @param engineType predefined engine type, default is bpfile
     */
    void SetEngine(const std::string engineType);

    /**
     * @brief Sets a single parameter. Overwrites value if key exists;
     * @param key parameter key
     * @param value parameter value
     */
    void SetParameter(const std::string key, const std::string value);

    /**
     * @brief Version that passes a map to fill out parameters
     * initializer list = { "param1", "value1" },  {"param2", "value2"},
     * Replaces any existing parameter. Otherwise use SetParameter for adding
     * new parameters.
     * @param parameters adios::Params = std::map<std::string, std::string>
     * key/value parameters
     */
    void SetParameters(const adios2::Params &parameters = adios2::Params());

    /**
     * @brief Version that passes a single string to fill out many parameters.
     * Replaces any existing parameter.
     * initializer string = "param1=value1 , param2 = value2"
     */
    void SetParameters(const std::string &parameters);

    /**
     * @brief Remove all existing parameters.
     * Replaces any existing parameter.
     * initializer string = "param1=value1 , param2 = value2"
     */
    void ClearParameters();

    /**
     * Return current parameters set from either SetParameters/SetParameter
     * functions or from config XML for currrent IO object
     * @return string key/value map of current parameters (not modifiable)
     */
    adios2::Params Parameters() const;

    /**
     * @brief Adds a transport and its parameters to current IO. Must be
     * supported by current EngineType().
     * @param type must be a supported transport type for a particular Engine.
     * CAN'T use the keywords "Transport" or "transport"
     * @param parameters acceptable parameters for a particular transport
     * @return transportIndex handler
     * @exception std::invalid_argument if type=transport
     */
    size_t AddTransport(const std::string type,
                        const adios2::Params &parameters = adios2::Params());

    /**
     * @brief Sets a single parameter to an existing transport identified
     * with a transportIndex handler from AddTransport.
     * Overwrites existing parameter with the same key.
     * @param transportIndex index handler from AddTransport
     * @param key parameter key
     * @param value parameter value
     * @exception std::invalid_argument if transportIndex not valid, e.g. not a
     * handler from AddTransport.
     */
    void SetTransportParameter(const size_t transportIndex,
                               const std::string key, const std::string value);

    /**
     * Define a Variable<T> object within IO
     * @param name unique variable identifier
     * @param shape global dimension
     * @param start local offset
     * @param count local dimension
     * @param constantDims true: shape, start, count won't change, false:
     * shape, start, count will change after definition
     * @return Variable<T> object
     */
    template <class T>
    Variable<T>
    DefineVariable(const std::string &name, const Dims &shape = Dims(),
                   const Dims &start = Dims(), const Dims &count = Dims(),
                   const bool constantDims = false);

    /**
     * Retrieve a Variable object within current IO object
     * @param name unique variable identifier within IO object
     * @return if found Variable object is true and has functionality, else
     * false and has no functionality
     */
    template <class T>
    Variable<T> InquireVariable(const std::string &name);

    /**
     * @brief Define attribute inside io. Array input version
     * @param name unique attribute identifier IO object or for a Variable if
     * variableName is not empty (associated to a variable)
     * @param data pointer to user data
     * @param size number of data elements
     * @param variableName default is empty, if not empty attributes is
     * associated to a variable
     * @param separator default is "/", hierarchy between variable name and
     * attribute, e.g. variableName/attribute1, variableName::attribute1. Not
     * used if variableName is empty.
     * @return object reference to internal Attribute in IO
     * @exception std::invalid_argument if Attribute with unique name (in IO or
     * Variable) is already defined, in debug mode only
     */
    template <class T>
    Attribute<T> DefineAttribute(const std::string &name, const T *data,
                                 const size_t size,
                                 const std::string &variableName = "",
                                 const std::string separator = "/");

    /**
     * @brief Define single value attribute
     * @param name must be unique for the IO object or for a Variable if
     * variableName is not empty (associated to a variable)
     * @param value single data value
     * @param variableName default is empty, if not empty attributes is
     * associated to a variable
     * @param separator default is "/", hierarchy between variable name and
     * attribute, e.g. variableName/attribute1, variableName::attribute1. Not
     * used if variableName is empty.
     * @return object reference to internal Attribute in IO
     * @exception std::invalid_argument if Attribute with unique name (in IO or
     * Variable) is already defined, in debug mode only
     */
    template <class T>
    Attribute<T> DefineAttribute(const std::string &name, const T &value,
                                 const std::string &variableName = "",
                                 const std::string separator = "/");

    /**
     * @brief Retrieve an existing attribute
     * @param name must be unique for the IO object or for a Variable if
     * variableName is not empty (associated to a variable)
     * @param variableName default is empty, if not empty attributes is expected
     * to be associated to a variable
     * @param separator default is "/", hierarchy between variable name and
     * attribute, e.g. variableName/attribute1, variableName::attribute1. Not
     * used if variableName is empty.
     * @return object reference to internal Attribute in IO, object is false if
     * Attribute is not found
     */
    template <class T>
    Attribute<T> InquireAttribute(const std::string &name,
                                  const std::string &variableName = "",
                                  const std::string separator = "/");

    /**
     * @brief DANGEROUS! Removes an existing Variable in current IO object.
     * Might create dangling objects.
     * @param name unique Variable input
     * @return true: found and removed variable, false: not found, nothing
     * to remove
     */
    bool RemoveVariable(const std::string &name);

    /**
     * @brief DANGEROUS! Removes all existing variables in current IO object.
     * Might create dangling objects.
     */
    void RemoveAllVariables();

    /**
     * @brief DANGEROUS! Removes an existing Attribute in current IO object.
     * Might create dangling objects.
     * @param name unique Attribute identifier
     * @return true: found and removed attribute, false: not found, nothing to
     * remove
     */
    bool RemoveAttribute(const std::string &name);

    /**
     * @brief DANGEROUS! Removes all existing attributes in current IO object.
     * Might create dangling objects.
     */
    void RemoveAllAttributes();

    /**
     * Open an Engine to start heavy-weight input/output operations.
     * This version reuses the ADIOS object communicator
     * MPI Collective function as it calls MPI_Comm_dup
     * @param name unique engine identifier
     * @param mode adios2::Mode::Write, adios2::Mode::Read, or
     *             adios2::Mode::Append (not yet support)
     * @return engine object
     */
    Engine Open(const std::string &name, const Mode mode);

#ifdef ADIOS2_HAVE_MPI
    /**
     * Open an Engine to start heavy-weight input/output operations.
     * This version allows passing a MPI communicator different from the one
     * used in the ADIOS object contructor
     * MPI Collective function as it calls MPI_Comm_dup
     * @param name unique engine identifier within IO
     * @param mode adios2::Mode::Write, adios2::Mode::Read, or
     *             adios2::Mode::Append (not yet support)
     * @param comm new communicator other than ADIOS object's communicator
     * @return engine object
     */
    Engine Open(const std::string &name, const Mode mode, MPI_Comm comm);
#endif

    /** Flushes all engines created with this IO with the Open function */
    void FlushAll();

    /**
     * Returns a map with variable information
     * @return map:
     * <pre>
     * key: variable name
     * value: Params
     * 		string key: variable info key
     *      string value: variable info value
     * </pre>
     */
    std::map<std::string, Params> AvailableVariables();

    /**
     * Returns a map with available attributes information associated to a
     * particular variableName
     * @param variableName unique variable name associated with resulting
     * attributes, if empty (default) return all attributes
     * @param separator optional name hierarchy separator (/, ::, _, -, \\,
     * etc.)
     * @param fullNameKeys true: return full attribute names in keys, false
     * (default): return attribute names relative to variableName
     * @return map:
     * <pre>
     * key: unique attribute name
     * value: Params
     * 		string key: attribute info key
     *      string value: attribute info value
     * </pre>
     */
    std::map<std::string, Params>
    AvailableAttributes(const std::string &variableName = "",
                        const std::string separator = "/",
                        const bool fullNameKeys = false);

    /**
     * Inspects variable type. This function can be used in conjunction with
     * MACROS in an else if (type == adios2::GetType<T>() ) {} loop
     * @param name unique variable name identifier in current IO
     * @return type as in adios2::GetType<T>() (e.g. "double", "float"),
     * empty std::string if variable not found
     */
    std::string VariableType(const std::string &name) const;

    /**
     * Inspects attribute type. This function can be used in conjunction with
     * MACROS in an else if (type == adios2::GetType<T>() ) {} loop
     * @param name unique attribute name identifier in current IO
     * @return type as in adios2::GetType<T>() (e.g. "double", "float"), empty
     * std::string if attribute not found
     */
    std::string AttributeType(const std::string &name) const;

    /**
     * EXPERIMENTAL: carries information about an Operation added with
     * AddOperation
     */
    struct Operation
    {
        /** Operator associated with this operation */
        const Operator Op;
        /** Parameters settings for this operation */
        const adios2::Params Parameters;
        /** Information associated with this operation */
        const adios2::Params Info;
    };

    /**
     * EXPERIMENTAL: Adds operation and parameters to current IO object
     * @param op operator to be added
     * @param parameters key/value settings particular to the IO, not to
     * be confused by op own parameters
     * @return operation index handler in Operations()
     */
    size_t AddOperation(const Operator op, const Params &parameters = Params());

    /**
     * Inspect current engine type from SetEngine
     * @return current engine type
     */
    std::string EngineType() const;

private:
    IO(core::IO *io);
    core::IO *m_IO = nullptr;
};

// Explicit declaration of the public template methods
// Limits the types
#define declare_template_instantiation(T)                                      \
    extern template Variable<T> IO::DefineVariable(const std::string &,        \
                                                   const Dims &, const Dims &, \
                                                   const Dims &, const bool);  \
                                                                               \
    extern template Variable<T> IO::InquireVariable<T>(const std::string &);

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

#define declare_template_instantiation(T)                                      \
    extern template Attribute<T> IO::DefineAttribute(                          \
        const std::string &, const T *, const size_t, const std::string &,     \
        const std::string);                                                    \
                                                                               \
    extern template Attribute<T> IO::DefineAttribute(                          \
        const std::string &, const T &, const std::string &,                   \
        const std::string);                                                    \
                                                                               \
    extern template Attribute<T> IO::InquireAttribute<T>(                      \
        const std::string &, const std::string &, const std::string);
ADIOS2_FOREACH_ATTRIBUTE_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

std::string ToString(const IO &io);

} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX11_CXX11_IO_H_ */
