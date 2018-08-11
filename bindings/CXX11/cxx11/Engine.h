/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Engine.h :
 *
 *  Created on: Jun 4, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_CXX11_CXX11_ENGINE_H_
#define ADIOS2_BINDINGS_CXX11_CXX11_ENGINE_H_

#include "Variable.h"

#include "adios2/ADIOSMacros.h"
#include "adios2/ADIOSTypes.h"

namespace adios2
{

/// \cond EXCLUDE_FROM_DOXYGEN
// forward declare
class IO; // friend

namespace core
{
class Engine; // private implementation
}
/// \endcond

class Engine
{
    friend class IO;

public:
    Engine() = default;

    ~Engine() = default;

    /**
     * true: valid engine, false: invalid, not created with IO::Open
     */
    explicit operator bool() const noexcept;

    /**
     * Inspect engine name
     * @return name from IO::Open
     */
    std::string Name() const;

    /**
     * From ADIOS2 engine type: "bpfile", "sst", "dataman", "insitumpi", "hdf5"
     * @return engine type as lower case string
     */
    std::string Type() const;

    /**
     * Begin a logical adios2 step, overloaded version with timeoutSeconds = 0
     * and mode = NextAvailable
     * @return current step status
     */
    StepStatus BeginStep();

    /**
     * Begin a logical adios2 step, overloaded version for advanced stream
     * control
     * @param mode how
     * @param timeoutSeconds
     * @return current step status
     */
    StepStatus BeginStep(const StepMode mode, const float timeoutSeconds = 0.f);

    /**
     * Inspect current logical step
     * @return current logical step
     */
    size_t CurrentStep() const;

    /** Informs the engine that the BeginStep Put/Get EndStep schedule is fixed
     * for the entire run. Simplifies synchronized between reader and writers
     */
    void FixedSchedule();

    /**
     * Put data associated with a Variable in the adios2 library
     * @param variable contains variable metadata information
     * @param data user data to be associated with a variable
     * @param launch mode policy
     * <pre>
     * 		Mode::Deferred, lazy evaulation, do not use data until first
     * PerformPuts, EndStep, or Close. THis is the preferred way.
     * 		Mode::Sync, data is consumed by the adios2 library and can be
     * reused immediately. Special case, only use if necessary.
     * </pre>
     * @exception std::invalid_argument for invalid variable or nullptr data
     */
    template <class T>
    void Put(Variable<T> variable, const T *data,
             const Mode launch = Mode::Deferred);

    /**
     * Put data associated with a Variable in the adios2 library
     * Overloaded version that accepts a variable name string.
     * @param variable contains variable metadata information
     * @param data user data to be associated with a variable
     * @param launch mode policy
     * <pre>
     * 		Mode::Deferred, lazy evaulation, do not use data until first
     * PerformPuts, EndStep, or Close. THis is the preferred way.
     * 		Mode::Sync, data is consumed by the adios2 library and can be
     * reused immediately. Special case, only use if necessary.
     * </pre>
     * @exception std::invalid_argument if variable not found or nullptr data
     */
    template <class T>
    void Put(const std::string &variableName, const T *data,
             const Mode launch = Mode::Deferred);

    /**
     * Put data associated with a Variable in the adios2 library
     * Overloaded version that accepts r-values and single variable data.
     * @param variable contains variable metadata information
     * @param datum user data to be associated with a variable, r-value or
     * single data value
     * @param launch mode policy, optional for API consistency, internally is
     * always sync
     * @exception std::invalid_argument if variable is invalid or nullptr &datum
     */
    template <class T>
    void Put(Variable<T> variable, const T &datum,
             const Mode launch = Mode::Deferred);

    /**
     * Put data associated with a Variable in the adios2 library
     * Overloaded version that accepts variables names, and r-values and single
     * variable data.
     * @param variable contains variable metadata information
     * @param data user data to be associated with a variable r-value or single
     * data value
     * @param launch mode policy, optional for API consistency, internally is
     * always sync
     * @exception std::invalid_argument if variable is invalid or nullptr &datum
     */
    template <class T>
    void Put(const std::string &variableName, const T &datum,
             const Mode launch = Mode::Deferred);

    /** Perform all Put calls in Deferred mode up to this point */
    void PerformPuts();

    template <class T>
    void Get(Variable<T> variable, T *data, const Mode launch = Mode::Deferred);

    template <class T>
    void Get(const std::string &variableName, T *data,
             const Mode launch = Mode::Deferred);

    template <class T>
    void Get(Variable<T> variable, T &datum,
             const Mode launch = Mode::Deferred);

    template <class T>
    void Get(const std::string &variableName, T &datum,
             const Mode launch = Mode::Deferred);

    template <class T>
    void Get(Variable<T> variable, std::vector<T> &dataV,
             const Mode launch = Mode::Deferred);

    template <class T>
    void Get(const std::string &variableName, std::vector<T> &dataV,
             const Mode launch = Mode::Deferred);

    /** Perform all Get calls in Deferred mode up to this point */
    void PerformGets();

    /**
     * Ends current step, by default consumes all Put/Get data in deferred mode
     */
    void EndStep();

    /**
     * Manually flush to underlying transport to guarantee data is moved
     * @param transportIndex
     */
    void Flush(const int transportIndex = -1);

    /**
     * Closes current engine, after this call an engine becomes invalid
     * @param transportIndex
     */
    void Close(const int transportIndex = -1);

    /**
     * Extracts all available blocks information for a particular
     * variable. This can be an expensive function, memory scales up with
     * metadata: steps and blocks per step
     * Valid in read mode only.
     * @param variable
     * @return map with all variable blocks information
     * <pre>
     * 	  key: step
     * 	  value: vector of blocks with info for each block per step
     * </pre>
     */
    template <class T>
    std::map<size_t, std::vector<typename Variable<T>::Info>>
    AllStepsBlocksInfo(const Variable<T> variable) const;

    /**
     * Extracts all available blocks information for a particular
     * variable and step.
     * Valid in read mode only.
     * @param variable input variable
     * @param step input from which block information is extracted
     * @return vector of blocks with info for each block per step, if step not
     * found it returns an empty vector
     */
    template <class T>
    std::vector<typename Variable<T>::Info>
    BlocksInfo(const Variable<T> variable, const size_t step) const;

private:
    Engine(core::Engine *engine);
    core::Engine *m_Engine = nullptr;
};

#define declare_template_instantiation(T)                                      \
    extern template void Engine::Put<T>(Variable<T>, const T *, const Mode);   \
    extern template void Engine::Put<T>(const std::string &, const T *,        \
                                        const Mode);                           \
    extern template void Engine::Put<T>(Variable<T>, const T &, const Mode);   \
    extern template void Engine::Put<T>(const std::string &, const T &,        \
                                        const Mode);                           \
                                                                               \
    extern template void Engine::Get<T>(Variable<T>, T *, const Mode);         \
    extern template void Engine::Get<T>(const std::string &, T *, const Mode); \
    extern template void Engine::Get<T>(Variable<T>, T &, const Mode);         \
    extern template void Engine::Get<T>(const std::string &, T &, const Mode); \
                                                                               \
    extern template void Engine::Get<T>(Variable<T>, std::vector<T> &,         \
                                        const Mode);                           \
    extern template void Engine::Get<T>(const std::string &, std::vector<T> &, \
                                        const Mode);                           \
                                                                               \
    extern template std::map<size_t, std::vector<typename Variable<T>::Info>>  \
    Engine::AllStepsBlocksInfo(const Variable<T> variable) const;              \
                                                                               \
    extern template std::vector<typename Variable<T>::Info>                    \
    Engine::BlocksInfo(const Variable<T> variable, const size_t step) const;

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX11_CXX11_ENGINE_H_ */
