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

#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#endif

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
#ifdef ADIOS2_HAVE_MPI
    /**
     * Starting point for MPI apps. Creates an ADIOS object.
     * MPI Collective Operation as it call MPI_Comm_dup
     * @param comm defines domain scope from application
     * @param debugMode true: extra user-input debugging information, false: run
     * without checking user-input (stable workflows)
     * @exception std::invalid_argument in debugMode = true if user input is
     * incorrect
     */
    ADIOS(MPI_Comm comm, const bool debugMode = true);

    /**
     * Starting point for MPI apps. Creates an ADIOS object allowing a
     * runtime config file.
     * MPI collective and it calls MPI_Comm_dup and MPI_Bcast to pass the
     * configFile contents
     * @param configFile runtime config file
     * @param comm defines domain scope from application
     * @param debugMode true: extra user-input debugging information, false:
     * run without checking user-input (stable workflows)
     * @exception std::invalid_argument in debugMode = true if user input is
     * incorrect
     */
    ADIOS(const std::string &configFile = "", MPI_Comm comm = MPI_COMM_SELF,
          const bool debugMode = true);
#else

    /**
     * Starting point for non-MPI serial apps. Creates an ADIOS object allowing
     * a runtime config file.
     * @param configFile runtime config file
     * @param debugMode true: extra user-input debugging information, false: run
     * without checking user-input (stable workflows)
     * @exception std::invalid_argument in debugMode = true if user input is
     * incorrect
     */
    ADIOS(const std::string &configFile, const bool debugMode = true);

    /**
     * Starting point for non-MPI apps. Creates an ADIOS object
     * @param debugMode true: extra user-input debugging information, false: run
     * without checking user-input (stable workflows)
     * @exception std::invalid_argument in debugMode = true if user input is
     * incorrect
     */
    ADIOS(const bool debugMode = true);
#endif

    /** object inspection true: valid object, false: invalid object */
    explicit operator bool() const noexcept;

    /**
     * DELETED Copy Constructor. ADIOS is the only object that manages its own
     * memory. Create a separate object for independent tasks */
    ADIOS(const ADIOS &) = delete;

    /**
     * default move constructor exists to allow for
     * auto ad = ADIOS(...) initialization
     */
    ADIOS(ADIOS &&) = default;

    /**
     * MPI Collective calls MPI_Comm_free
     * Uses RAII for all other members */
    ~ADIOS() = default;

    /**
     * copy assignment is forbidden for the same reason as copy constructor
     */
    ADIOS &operator=(const ADIOS &) = delete;

    /**
     * move assignment is allowed, though, to be consistent with move
     * constructor
     */
    ADIOS &operator=(ADIOS &&) = default;

    /**
     * Declares a new IO class object
     * @param name unique IO name identifier within current ADIOS object
     * @return reference to newly created IO object inside current ADIOS
     * object
     * @exception std::invalid_argument if IO with unique name is already
     * declared, in ADIOS debug mode only
     */
    IO DeclareIO(const std::string name);

    /**
     * Retrieve an existing IO object previously created with DeclareIO.
     * @param name IO unique identifier key in current ADIOS object
     * @return if IO exists returns a reference to existing IO object inside
     * ADIOS, else throws an exception. IO objects can't be invalid.
     * @exception std::invalid_argument if IO was not created with
     * DeclareIO, in debug mode only
     */
    IO AtIO(const std::string name);

    /**
     * Defines an adios2 supported operator by its type.
     * @param name unique operator name identifier within current ADIOS object
     * @param type supported ADIOS2 operator type: zfp, sz
     * @param parameters key/value parameters at the operator object level
     * @return Operator object
     * @exception std::invalid_argument if adios2 can't support current
     * operator due to missing dependency or unsupported type
     */
    Operator DefineOperator(const std::string name, const std::string type,
                            const Params &parameters = Params());

    /**
     * Defines an adios2 supported operator by its type. Variadic template
     * version for Operators of type Callback function with signatures suported
     * in ADIOS2. For new signature support open an issue on github.
     * @param name unique operator name within ADIOS object
     * @param function C++11 callable target
     * @param parameters key/value parameters at the operator level
     * @return Operator object
     * @exception std::invalid_argument if adios2 can't support current
     * operator due to missing dependency or unsupported type
     */
    template <class R, class... Args>
    Operator DefineOperator(const std::string name,
                            const std::function<R(Args...)> &function,
                            const Params &parameters = Params());

    /**
     * Retrieve an existing Operator object in current ADIOS object
     * @param name Operator unique identifier key in current ADIOS object
     * @return object to an existing operator in current ADIOS object, Operator
     * object is false if name is not found, in debugMode only
     */
    Operator InquireOperator(const std::string name);

    /**
     * Flushes all engines in write mode in all IOs created with the current
     * ADIOS object. If no IO or Engine exist, it does nothing.
     * @exception std::runtime_error if any engine Flush fails
     */
    void FlushAll();

    /**
     * DANGER ZONE: removes a particular IO. This will effectively eliminate
     * any parameter from the config.xml file
     * @param name io input name
     * @return true: IO was found and removed, false: IO not found and not
     * removed
     */
    bool RemoveIO(const std::string name);

    /**
     * DANGER ZONE: removes all IOs created with DeclareIO. This will
     * effectively eliminate any parameter from the config.xml file also.
     */
    void RemoveAllIOs() noexcept;

private:
    std::shared_ptr<core::ADIOS> m_ADIOS;

    void CheckPointer(const std::string hint);

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
