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

#include "Types.h"
#include "Variable.h"

#include "adios2/common/ADIOSMacros.h"
#include "adios2/common/ADIOSTypes.h"

namespace adios2
{

/// \cond EXCLUDE_FROM_DOXYGEN
// forward declare
class IO;    // friend
class Query; // friend
namespace core
{
class Engine; // private implementation
}
/// \endcond

class Engine
{
    friend class IO;
    friend class QueryWorker;

public:
    /**
     * Empty (default) constructor, use it as a placeholder for future
     * engines from IO::Open.
     * Can be used with STL containers.
     */
    Engine() = default;

    /** Using RAII STL containers only */
    ~Engine() = default;

    /** true: valid engine, false: invalid, not created with IO::Open or post
     * IO::Close*/
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
     * and mode = Read
     * Check each engine documentation for MPI collective/non-collective
     * behavior.
     * @return current step status
     */
    StepStatus BeginStep();

    /**
     * Begin a logical adios2 step, overloaded version for advanced stream
     * control
     * Check each engine documentation for MPI collective/non-collective
     * behavior.
     * @param mode see enum adios2::StepMode for options, Read is the
     * common use case
     * @param timeoutSeconds
     * @return current step status
     */
    StepStatus BeginStep(const StepMode mode,
                         const float timeoutSeconds = -1.f);

    /**
     * Inspect current logical step
     * @return current logical step
     */
    size_t CurrentStep() const;

    /**
     * Put signature that provides access to the internal engine buffer for a
     * pre-allocated variable including a fill value. Returns a fixed size Span
     * (based on C++20 std::span) so applications can populate data value after
     * this Put. Requires a call to PerformPuts, EndStep, or Close to extract
     * the Min/Max bounds.
     * @param variable input variable
     * @param bufferID if engine has multiple buffers, input 0 when this
     * information is not known
     * @param value provide an initial fill value
     * @return span to variable data in engine internal buffer
     */
    template <class T>
    typename Variable<T>::Span Put(Variable<T> variable, const size_t bufferID,
                                   const T &value);

    /**
     * Put signature that provides access to an internal engine buffer (decided
     * by the engine) for a pre-allocated variable with the default fill value
     * T().
     * @param variable input variable
     * @return span to variable data in engine internal buffer
     */
    template <class T>
    typename Variable<T>::Span Put(Variable<T> variable);

    /**
     * Put data associated with a Variable in the Engine
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
     * Put data associated with a Variable in the Engine
     * Overloaded version that accepts a variable name string.
     * @param variableName find variable by name inside IO that created this
     * Engine with Open
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
     * Put data associated with a Variable in the Engine
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
     * Put data associated with a Variable in the Engine
     * Overloaded version that accepts variables names, and r-values and single
     * variable data.
     * @param variableName find variable by name inside IO that created this
     * Engine with Open
     * @param datum user data to be associated with a variable r-value or single
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

    /**
     * Get data associated with a Variable from the Engine
     * @param variable contains variable metadata information
     * @param data user data to be associated with a variable, it must be
     * pre-allocated
     * @param launch mode policy
     * <pre>
     * 		Mode::Deferred, lazy evaluation, do not use data until
     * first PerformGets, EndStep, or Close. This is the preferred way.
     * 		Mode::Sync, data is obtained by the Engine and can be used
     * immediately.
     * Special case, only use if necessary.
     * </pre>
     * @exception std::invalid_argument for invalid variable or nullptr data
     */
    template <class T>
    void Get(Variable<T> variable, T *data, const Mode launch = Mode::Deferred);

    /**
     * Get data associated with a Variable from the Engine. Overloaded version
     * to get variable by name.
     * @param variableName find variable by name inside IO that created this
     * Engine with Open
     * @param data user data to be associated with a variable. It must be
     * pre-allocated
     * @param launch mode policy
     * <pre>
     * 		Mode::Deferred, lazy evaluation, do not use data until
     * first PerformGets, EndStep, or Close. This is the preferred way.
     * 		Mode::Sync, data is obtained by the Engine and can be
     * used
     * immediately.
     * Special case, only use if necessary.
     * </pre>
     * @exception std::invalid_argument for invalid variableName (variable
     * doesn't exist in IO) or nullptr data
     */
    template <class T>
    void Get(const std::string &variableName, T *data,
             const Mode launch = Mode::Deferred);

    /**
     * Get single value data associated with a Variable from the Engine
     * Overloaded version that accepts r-values and single variable data.
     * @param variable contains variable metadata information
     * @param datum user data to be populated, r-value or single data value
     * @param launch mode policy, optional for API consistency, internally
     * is always sync
     * @exception std::invalid_argument if variable is invalid or nullptr &datum
     */
    template <class T>
    void Get(Variable<T> variable, T &datum,
             const Mode launch = Mode::Deferred);

    /**
     * Get single value data associated with a Variable from the Engine
     * Overloaded version that accepts r-values and single variable data.
     * @param variableName find variable by name inside IO that created this
     * Engine with Open
     * @param datum user data to be populated, r-value or single data value
     * @param launch mode policy, optional for API consistency, internally
     * is always sync
     * @exception std::invalid_argument for invalid variableName (variable
     * doesn't exist in IO) or nullptr data
     */
    template <class T>
    void Get(const std::string &variableName, T &datum,
             const Mode launch = Mode::Deferred);

    /**
     * Get data associated with a Variable from the Engine.
     * Overloaded version that accepts a std::vector without pre-allocation.
     * @param variable contains variable metadata information
     * @param dataV user data vector to be associated with a variable, it
     * doesn't need to be pre-allocated. Engine will resize.
     * @param launch mode policy
     * <pre>
     * 		Mode::Deferred, lazy evaluation, do not use data until
     * first PerformGets, EndStep, or Close. This is the preferred way.
     * 		Mode::Sync, data is obtained by the Engine and can be used
     * immediately.
     * Special case, only use if necessary.
     * </pre>
     * @exception std::invalid_argument for invalid variable
     */
    template <class T>
    void Get(Variable<T> variable, std::vector<T> &dataV,
             const Mode launch = Mode::Deferred);

    /**
     * Get data associated with a Variable from the Engine.
     * Overloaded version that accepts a std::vector without pre-allocation.
     * @param variableName find variable by name inside IO that created this
     * Engine with Open or BeginStep (streaming mode)
     * @param dataV user data vector to be associated with a variable, it
     * doesn't need to be pre-allocated. Engine will resize.
     * @param launch mode policy
     * <pre>
     * 		Mode::Deferred, lazy evaluation, do not use data until
     * first PerformGets, EndStep, or Close. This is the preferred way.
     * 		Mode::Sync, data is obtained by the Engine and can be
     * used
     * immediately.
     * Special case, only use if necessary.
     * </pre>
     * @exception std::invalid_argument for invalid variableName (variable
     * doesn't exist in IO)
     */
    template <class T>
    void Get(const std::string &variableName, std::vector<T> &dataV,
             const Mode launch = Mode::Deferred);

    /**
     * Get data associated with a Variable from the Engine. Data is
     * associated with a block selection, and data is retrieved from
     * variable's BlockInfo.
     * @note Preliminary, experimental API, may change soon.
     * @param variable contains variable metadata information
     * @param info block info struct associated with block selection,
     *   call will link with implementation's block info.
     * @param launch mode policy
     * <pre>
     *      Mode::Deferred, lazy evaluation, do not use data until
     * first PerformGets, EndStep, or Close. This is the preferred way.
     *      Mode::Sync, data is obtained by the Engine and can be used
     * immediately.
     * Special case, only use if necessary.
     * </pre>
     * @exception std::invalid_argument for invalid variable or nullptr data
     */
    template <class T>
    void Get(Variable<T> variable, typename Variable<T>::Info &info,
             const Mode launch = Mode::Deferred);
    /**
     * Get data associated with a Variable from the Engine. Data is
     * associated with a block selection, and data is retrieved from
     * variable's BlockInfo. Overloaded version
     * to get variable by name.
     * @note Preliminary, experimental API, may change soon.
     * @param variableName find variable by name inside IO that created this
     * Engine with Open or BeginStep (streaming mode)
     * @param info block info struct associated with block selection,
     *   call will link with implementation's block info.
     * @param launch mode policy
     * <pre>
     *      Mode::Deferred, lazy evaluation, do not use data until
     * first PerformGets, EndStep, or Close. This is the preferred way.
     *      Mode::Sync, data is obtained by the Engine and can be used
     * immediately.
     * Special case, only use if necessary.
     * </pre>
     * @exception std::invalid_argument for invalid variableName (variable
     * doesn't exist in IO)
     */
    template <class T>
    void Get(const std::string &variableName, typename Variable<T>::Info &info,
             const Mode launch = Mode::Deferred);

    /** Perform all Get calls in Deferred mode up to this point */
    void PerformGets();

    /**
     * Ends current step, by default calls PerformsPut/Get internally
     * Check each engine documentation for MPI collective/non-collective
     * behavior.
     */
    void EndStep();

    /**
     * Manually flush to underlying transport to guarantee data is moved
     * @param transportIndex
     */
    void Flush(const int transportIndex = -1);

    /**
     * Closes current engine, after this call an engine becomes invalid
     * MPI Collective, calls MPI_Comm_free for duplicated communicator at Open
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

    /**
     * Inspect total number of available steps, use for file engines in read
     * mode only
     * @return available steps in engine
     */
    size_t Steps() const;

    /**
     * @brief Promise that no more definitions or changes to defined variables
     * will occur. Useful information if called before the first EndStep() of an
     * output Engine, as it will know that the definitions are complete and
     * constant for the entire lifetime of the output and may optimize metadata
     * handling.
     */
    void LockWriterDefinitions();

    /**
     * @brief Promise that the reader data selections of are fixed and
     * will not change in future timesteps. This information, provided
     * before the EndStep() representing a fixed read pattern, may be
     * utilized by the input Engine to optimize data flow.
     */
    void LockReaderSelections();

private:
    Engine(core::Engine *engine);
    core::Engine *m_Engine = nullptr;
};

#define declare_template_instantiation(T)                                      \
                                                                               \
    extern template typename Variable<T>::Span Engine::Put(                    \
        Variable<T>, const size_t, const T &);                                 \
                                                                               \
    extern template typename Variable<T>::Span Engine::Put(Variable<T>);

ADIOS2_FOREACH_PRIMITIVE_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

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
    extern template void Engine::Get<T>(                                       \
        Variable<T>, typename Variable<T>::Info & info, const Mode);           \
    extern template void Engine::Get<T>(                                       \
        const std::string &, typename Variable<T>::Info &info, const Mode);    \
                                                                               \
    extern template std::map<size_t, std::vector<typename Variable<T>::Info>>  \
    Engine::AllStepsBlocksInfo(const Variable<T> variable) const;              \
                                                                               \
    extern template std::vector<typename Variable<T>::Info>                    \
    Engine::BlocksInfo(const Variable<T> variable, const size_t step) const;

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

std::string ToString(const Engine &engine);

} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX11_CXX11_ENGINE_H_ */
