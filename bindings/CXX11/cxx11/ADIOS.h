/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS.h : public ADIOS class using PIMPL for C++11 bindings
 *
 *  Created on: Jun 4, 2018
 *      Author: William F Godoy
 */

#ifndef ADIOS2_BINDINGS_CXX11_CXX11_ADIOS_H_
#define ADIOS2_BINDINGS_CXX11_CXX11_ADIOS_H_

#include <functional>
#include <memory>
#include <string>

#include "IO.h"
#include "Operator.h"

#include "adios2/ADIOSMPICommOnly.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/ADIOSTypes.h"

namespace adios2
{

/// \cond EXCLUDE_FROM_DOXYGEN
// forward declare
namespace core
{
class ADIOS; // private implementation
}
/// \endcond

class ADIOS
{

public:
    /**
     * adios2 library starting point. Creates an ADIOS object allowing a runtime
     * config file.
     * @param configFile runtime config file
     * @param mpiComm defines domain scope from application
     * @param debugMode true: extra user-input debugging information, false: run
     * without checking user-input (stable workflows)
     * @exception std::invalid_argument in debugMode = true if user input is
     * incorrect
     */
    ADIOS(const std::string &configFile, MPI_Comm mpiComm,
          const bool debugMode = true);

    /**
     * adios2 library starting point. Creates an ADIOS object.
     * @param mpiComm defines domain scope from application
     * @param debugMode true: extra user-input debugging information, false: run
     * without checking user-input (stable workflows)
     * @exception std::invalid_argument in debugMode = true if user input is
     * incorrect
     */
    ADIOS(MPI_Comm mpiComm, const bool debugMode = true);

    /**
     * adios2 NON-MPI library starting point. Creates an ADIOS object allowing a
     * runtime config file.
     * @param configFile runtime config file
     * @param debugMode true: extra user-input debugging information, false: run
     * without checking user-input (stable workflows)
     * @exception std::invalid_argument in debugMode = true if user input is
     * incorrect
     */
    ADIOS(const std::string &configFile, const bool debugMode = true);

    /**
     * adios2 NON-MPI library starting point. Creates an ADIOS object
     * @param debugMode true: extra user-input debugging information, false: run
     * without checking user-input (stable workflows)
     * @exception std::invalid_argument in debugMode = true if user input is
     * incorrect
     */
    ADIOS(const bool debugMode = true);

    /** object inspection true: valid object, false: invalid object */
    explicit operator bool() const noexcept;

    /**
     * DELETED Copy Constructor.
     * ADIOS is the only object that manages its own memory.
     * Create a separate for independent tasks */
    ADIOS(const ADIOS &) = delete;

    ~ADIOS() = default;

    /**
     * Declares a new IO class object and returns a reference to that
     * object.
     * @param ioName unique IO name identifier within current ADIOS object
     * @return reference to newly created IO object inside current ADIOS
     * object
     * @exception std::invalid_argument if IO with unique name is already
     * declared, in ADIOS debug mode only
     */
    IO DeclareIO(const std::string name);

    /**
     * Retrieve a reference to an existing IO object created with DeclareIO.
     * @param name of IO to look for in current ADIOS object
     * @return if IO exists returns a reference to existing IO object inside
     * ADIOS
     * @exception std::invalid_argument if IO was not created with
     * DeclareIO, in debug mode only
     */
    IO AtIO(const std::string name);

    /**
     * Flushes all engines in all IOs created with the current ADIOS object
     * using DeclareIO and IO.Open.
     * If no IO or engine is created it does nothing.
     * @exception std::runtime_error if any engine Flush fails
     */
    void FlushAll();

    /**
     * Defines an ADIOS2 supported operator by its type.
     * @param name unique operator name within ADIOS object
     * @param type supported ADIOS2 operator
     * @param parameters key/value parameters at the operator level
     * @return Operator object
     * @exception std::invalid_argument if library can't support current
     * operator due to missing dependency
     */
    Operator DefineOperator(const std::string name, const std::string type,
                            const Params &parameters = Params());

    /**
     * Variadic template version for Operators of type Callback function
     * with signatures suported in ADIOS2
     * @param name unique operator name within ADIOS object
     * @param function C++11 callable target
     * @param parameters key/value parameters at the operator level
     * @return Operator object
     * @exception std::invalid_argument if library can't support current
     * operator due to missing dependency or unsupported signature
     */
    template <class R, class... Args>
    Operator DefineOperator(const std::string name,
                            const std::function<R(Args...)> &function,
                            const Params &parameters = Params());

    /**
     * Returns an existing Operator identified by its name
     * @param name of Operator to be retrieved
     * @return object to an existing operator in current ADIOS object, Operator
     * object is false if name is not found
     */
    Operator InquireOperator(const std::string name) noexcept;

private:
    std::shared_ptr<core::ADIOS> m_ADIOS;

/* CallBack1 signature */
#define declare_type(T)                                                        \
    Operator DefineCallBack(                                                   \
        const std::string name,                                                \
        const std::function<void(const T *, const std::string &,               \
                                 const std::string &, const std::string &,     \
                                 const size_t, const Dims &, const Dims &,     \
                                 const Dims &)> &function,                     \
        const Params &parameters);
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    /* CallBack2 signature */
    Operator DefineCallBack(
        const std::string name,
        const std::function<void(void *, const std::string &,
                                 const std::string &, const std::string &,
                                 const size_t, const Dims &, const Dims &,
                                 const Dims &)> &function,
        const Params &parameters);
};

} // end namespace adios2

#include "ADIOS.inl"

#endif /* ADIOS2_BINDINGS_CXX11_CXX11_ADIOS_H_ */
