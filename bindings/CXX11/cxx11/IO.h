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

#include "adios2/ADIOSMPICommOnly.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/ADIOSTypes.h"

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
    ~IO() = default;

    /**
     * Checks if IO exists in a config file passed to ADIOS object that
     * created this IO
     * @return true: in config file, false: not in config file
     */
    bool InConfigFile() const noexcept;

    /**
     * @brief Sets the engine type for this IO class object
     * @param engineType predefined engine type, default is bpfile
     */
    void SetEngine(const std::string engineType) noexcept;

    /**
     * @brief Sets a single parameter overwriting value if key exists;
     * @param key parameter key
     * @param value parameter value
     */
    void SetParameter(const std::string key, const std::string value) noexcept;

    /**
     * @brief Version that passes a map to fill out parameters
     * initializer list = { "param1", "value1" },  {"param2", "value2"},
     * Replaces any existing parameter. Otherwise use SetParameter for adding
     * new parameters.
     * @param parameters adios::Params = std::map<std::string, std::string>
     * key/value parameters
     */
    void SetParameters(const adios2::Params &parameters = Params()) noexcept;

    /**
     * Return current parameters set from either SetParameters/SetParameter
     * functions or from config XML for currrent IO object
     * @return string key/value map of current parameters (not modifiable)
     */
    const Params &GetParameters() const noexcept;

    /**
     * @brief Adds a transport and its parameters for the IO Engine
     * @param type must be a supported transport type for a particular Engine.
     * CAN'T use the keyword "Transport" or "transport"
     * @param params acceptable parameters for a particular transport
     * @return transportIndex handler
     * @exception std::invalid_argument if type=transport
     */
    unsigned int AddTransport(const std::string type,
                              const Params &parameters = Params());

    /**
     * @brief Sets a single parameter to an existing transport identified
     * with a
     * transportIndex handler from AddTransport.
     * This function overwrites existing parameter with the same key.
     * @param transportIndex index handler from AddTransport
     * @param key parameter key
     * @param value parameter value
     * @exception std::invalid_argument if transportIndex not valid, e.g. not a
     * handler from AddTransport.
     */
    void SetTransportParameter(const unsigned int transportIndex,
                               const std::string key, const std::string value);

    /**
     * Define a Variable<T> object within current IO object
     * @param name unique variable identifier
     * @param shape global dimension
     * @param start local offset
     * @param count local dimension
     * @param constantDims true: shape, start, count won't change, false:
     * shape, start, count will change over time
     * @return Variable<T> object
     */
    template <class T>
    Variable<T>
    DefineVariable(const std::string &name, const Dims &shape = Dims(),
                   const Dims &start = Dims(), const Dims &count = Dims(),
                   const bool constantDims = false);

    /**
     * Returns a Variable object within current IO object
     * @param name unique variable identifier within IO object
     * @return if found Variable object is true and has functionality, else
     * false and has no functionality
     */
    template <class T>
    Variable<T> InquireVariable(const std::string &name) noexcept;

    /**
     * @brief Define array attribute
     * @param name must be unique for the IO object
     * @param array pointer to user data
     * @param size number of data elements
     * @return reference to internal Attribute
     * @exception std::invalid_argument if Attribute with unique name is
     * already defined, in debug mode only
     */
    template <class T>
    Attribute<T> DefineAttribute(const std::string &name, const T *data,
                                 const size_t size);

    /**
     * @brief Define single value attribute
     * @param name must be unique for the IO object
     * @param value single data value
     * @return reference to internal Attribute
     * @exception std::invalid_argument if Attribute with unique name is already
     * defined, in debug mode only
     */
    template <class T>
    Attribute<T> DefineAttribute(const std::string &name, const T &value);

    /**
     * Gets an existing attribute of primitive type by name
     * @param name of attribute to be retrieved
     * @return object to an existing attribute in current IO, object is false if
     * not found
     */
    template <class T>
    Attribute<T> InquireAttribute(const std::string &name) noexcept;

    /**
     * @brief Removes an existing Variable in current IO object.
     * Dangerous function since corresponding Variable<T> object is invalidated
     * after this call.
     * @param name unique Variable input
     * @return true: found and removed variable, false: not found, nothing
     * to remove
     */
    bool RemoveVariable(const std::string &name) noexcept;

    /**
     * @brief Removes all existing variables in current IO object.
     * Dangerous function since invalidates all Variable<T> objects.
     */
    void RemoveAllVariables() noexcept;

    /**
     * @brief Removes an existing Attribute in current IO object.
     * Dangerous function since corresponding Attribute<T> object is invalidated
     * after this call.
     * @param name unique Attribute identifier
     * @return true: found and removed attribute, false: not found, nothing to
     * remove
     */
    bool RemoveAttribute(const std::string &name) noexcept;

    /**
     * @brief Removes all existing attributes in current IO object.
     * Dangerous function since invalidates all Attribute<T> objects.
     */
    void RemoveAllAttributes() noexcept;

    /**
     * Open an Engine to start heavy-weight input/output operations.
     * New MPI communicator version
     * @param name unique engine identifier
     * @param mode
     * @param comm
     * @return engine object
     */
    Engine Open(const std::string &name, const Mode mode, MPI_Comm comm);

    /**
     * Open an Engine to start heavy-weight input/output operations.
     * Reuses ADIOS object communicator ADIOS>IO>Engine
     * @param name unique engine identifier
     * @param mode adios2::Mode::Write,adios2::Mode::Read or
     * adios2::Mode::Append
     * @return engine object
     */
    Engine Open(const std::string &name, const Mode mode);

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
    std::map<std::string, Params> AvailableVariables() noexcept;

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
    std::map<std::string, Params> AvailableAttributes() noexcept;

    /**
     * EXPERIMENTAL: carries information about an Operation added with
     * AddOperation
     */
    struct Operation
    {
        const Operator Op;
        const Params Parameters;
        const Params Info;
    };

    /**
     * EXPERIMENTAL: Adds operation and parameters to current IO object
     * @param op operator to be added
     * @param parameters key/value settings particular to the IO, not to
     * be confused by op own parameters
     * @return operation index handler in Operations()
     */
    size_t AddOperation(const Operator op,
                        const Params &parameters = Params()) noexcept;

    /**
     * Inspect current engine type
     * @return current engine type
     */
    std::string EngineType() const noexcept;

private:
    IO(core::IO &io);
    core::IO &m_IO;
};

// Explicit declaration of the public template methods
// Limits the types
#define declare_template_instantiation(T)                                      \
    extern template Variable<T> IO::DefineVariable(const std::string &,        \
                                                   const Dims &, const Dims &, \
                                                   const Dims &, const bool);  \
                                                                               \
    extern template Variable<T> IO::InquireVariable<T>(                        \
        const std::string &) noexcept;

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

#define declare_template_instantiation(T)                                      \
    extern template Attribute<T> IO::DefineAttribute(const std::string &,      \
                                                     const T *, const size_t); \
                                                                               \
    extern template Attribute<T> IO::DefineAttribute(const std::string &,      \
                                                     const T &);               \
                                                                               \
    extern template Attribute<T> IO::InquireAttribute<T>(                      \
        const std::string &) noexcept;
ADIOS2_FOREACH_ATTRIBUTE_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX11_CXX11_IO_H_ */
