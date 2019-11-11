/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS.h : ADIOS library starting point, factory class for IO objects
 *  Created on: Oct 3, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_ADIOS_H_
#define ADIOS2_CORE_ADIOS_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <functional> //std::function
#include <map>
#include <memory> //std::shared_ptr
#include <string>
#include <vector>
/// \endcond

#include "adios2/common/ADIOSConfig.h"
#include "adios2/common/ADIOSTypes.h"
#include "adios2/core/Operator.h"
#include "adios2/helper/adiosComm.h"

namespace adios2
{
namespace core
{

class IO;

/** @brief Point of entry class for an application.
 *         Serves as factory of IO class objects and Transforms */
class ADIOS
{
public:
    /** if true will do more checks, exceptions, warnings, expect slower code */
    const bool m_DebugMode = true;

    /** Get the communicator passed to constructor for parallel case.  */
    helper::Comm const &GetComm() const { return m_Comm; }

    /** Changed by language bindings in constructor */
    const std::string m_HostLanguage = "C++";

    /**
     * @brief Constructor for MPI applications WITH a XML config file
     * @param configFile XML format (maybe support different formats in the
     * future (json)?)
     * @param mpiComm MPI communicator from application, make sure is valid
     * through the scope of adios2 calls
     * @param debugMode true (default): extra exception checks (recommended),
     * false: optional feature to turn off checks on user input data,
     * recommended in stable flows
     */
    ADIOS(const std::string configFile, helper::Comm comm, const bool debugMode,
          const std::string hostLanguage);

    /**
     * @brief Constructor for non-MPI applications WITH a XML config file (it
     * must end with extension .xml)
     * @param configFile XML format (maybe support different formats in the
     * future (json)?)
     * @param debugMode true (default): extra exception checks (recommended),
     * false: optional feature to turn off checks on user input data,
     * recommended in stable flows
     */
    ADIOS(const std::string configFile, const bool debugMode,
          const std::string hostLanguage);

    /**
     * @brief Constructor for MPI apps WITHOUT a XML config file
     * @param mpiComm MPI communicator from application
     * @param debugMode true (default): extra exception checks (recommended),
     * false: optional feature to turn off checks on user input data,
     * recommended in stable flows
     */
    ADIOS(helper::Comm comm, const bool debugMode,
          const std::string hostLanguage);

    /**
     *  @brief ADIOS no-MPI default empty constructor
     * @param debugMode true (default): extra exception checks (recommended),
     * false: optional feature to turn off checks on user input data,
     * recommended in stable flows
     */
    ADIOS(const bool debugMode, const std::string hostLanguage);

    /**
     * Delete copy constructor explicitly. Objects shouldn't be allowed to be
     * redefined. Use smart pointers if this is absolutely necessary.
     * @param adios reference to another adios object
     */
    ADIOS(const ADIOS &adios) = delete;

    ~ADIOS();

    /**
     * Declares a new IO class object and returns a reference to that object.
     * @param ioName must be unique
     * @return reference to newly created IO object inside current ADIOS object
     * @exception std::invalid_argument if IO with unique name is already
     * declared, in debug mode only
     */
    IO &DeclareIO(const std::string name);

    /**
     * Retrieve a reference to an existing IO object created with DeclareIO.
     * Follow the C++11 STL containers at function.
     * @param name of IO to look for
     * @return if IO exists returns a reference to existing IO object inside
     * ADIOS
     * @exception std::invalid_argument if IO was not created with DeclareIO, in
     * debug mode only
     */
    IO &AtIO(const std::string name);

    /**
     * Flushes all engines in all IOs created with the current ADIOS object
     * using DeclareIO and IO.Open.
     * If no IO or engine is created it does nothing.
     * @exception std::runtime_error if any engine Flush fails
     */
    void FlushAll();

    /**
     * Declares a derived class of the Operator abstract class. If object is
     * defined in the user
     * config file, by name, it will be already created during the processing of
     * the config file. So this function returns a reference to that object.
     * @param name must be unique for each operator created with DefineOperator
     * @param type from derived class
     * @param parameters optional parameters
     * @return reference to Operator object
     * @exception std::invalid_argument if Operator with unique name is already
     * defined, in debug mode only
     */
    Operator &DefineOperator(const std::string &name, const std::string type,
                             const Params &parameters = Params());
    /**
     * Retrieve a reference pointer to an existing Operator object
     * created with DefineOperator.
     * @return if IO exists returns a reference to existing IO object inside
     * ADIOS, otherwise a nullptr
     */
    Operator *InquireOperator(const std::string &name) noexcept;

/** define CallBack1 */
#define declare_type(T)                                                        \
    Operator &DefineCallBack(                                                  \
        const std::string name,                                                \
        const std::function<void(const T *, const std::string &,               \
                                 const std::string &, const std::string &,     \
                                 const size_t, const Dims &, const Dims &,     \
                                 const Dims &)> &function,                     \
        const Params &parameters);

    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    /** define CallBack2 */
    Operator &DefineCallBack(
        const std::string name,
        const std::function<void(void *, const std::string &,
                                 const std::string &, const std::string &,
                                 const size_t, const Dims &, const Dims &,
                                 const Dims &)> &function,
        const Params &parameters);

    /**
     * DANGER ZONE: removes a particular IO. This will effectively eliminate any
     * parameter from the config.xml file
     * @param name io input name
     * @return true: IO was found and removed, false: IO not found and not
     * removed
     */
    bool RemoveIO(const std::string name);

    /**
     * DANGER ZONE: removes all IOs created with DeclareIO. This will
     * effectively eliminate any parameter from the config.xml file
     */
    void RemoveAllIOs() noexcept;

private:
    /** Communicator given to parallel constructor. */
    helper::Comm m_Comm;

    /** XML File to be read containing configuration information */
    const std::string m_ConfigFile;

    /**
     * @brief List of IO class objects defined from either ADIOS
     * configuration file (XML) or the DeclareIO function explicitly.
     * Using map (binary tree) to preserve references returned by DeclareIO.
     * <pre>
     *     Key: unique method name
     *     Value: IO class object
     * </pre>
     */
    std::map<std::string, IO> m_IOs;

    /** operators created with DefineOperator */
    std::map<std::string, std::shared_ptr<Operator>> m_Operators;

    void CheckOperator(const std::string name) const;

    void XMLInit(const std::string &configFileXML);

    void YAMLInit(const std::string &configFileYAML);
};

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ADIOS_H_ */
