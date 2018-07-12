/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Engine.h Base abstract class for the core Open, Write/Read, Advance, Close
 * functionality. Use toolkit components to build your own Engine extending this
 * class.
 * Examples of derived classes in: adios2/engine/
 *
 *  Created on: Nov 7, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_ENGINE_H_
#define ADIOS2_CORE_ENGINE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <functional> //std::function
#include <memory>     //std::shared_ptr
#include <set>
#include <string>
#include <vector>
/// \endcond

#include "adios2/ADIOSConfig.h"
#include "adios2/ADIOSMPICommOnly.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/core/IO.h"
#include "adios2/core/Variable.h"
#include "adios2/core/VariableCompound.h"

namespace adios2
{
namespace core
{

/** Base Abstract class for IO operations:  Read/Write, Schedule, Advance and
 * Close */
class Engine
{
public:
    using AdvanceAsyncCallback =
        std::function<void(std::shared_ptr<core::Engine>)>;

public:
    /** from derived class */
    const std::string m_EngineType;

    /** Unique name for this Engine within m_IO */
    const std::string m_Name;

    /** open mode from ADIOSTypes.h OpenMode */
    const Mode m_OpenMode;

    /**
     * Unique Base class constructor
     * @param engineType derived class identifier
     * @param io object that generates this Engine
     * @param name unique engine name within IO class object
     * @param mode  open mode from ADIOSTypes.h Mode
     * @param mpiComm new communicator passed at Open or from ADIOS class
     */
    Engine(const std::string engineType, IO &io, const std::string &name,
           const Mode mode, MPI_Comm mpiComm);

    virtual ~Engine();

    explicit operator bool() const noexcept;

    /**
     * Gets the factory IO object
     * @return reference to IO object that created this engine
     */
    IO &GetIO() noexcept;

    /**
     * Returns the Mode used at Open for current Engine
     * @return
     */
    Mode OpenMode() const noexcept;

    StepStatus BeginStep();

    /**
     * Indicates the beginning of a step. Typically used for streaming and
     * inside step loops.
     * @param mode stepping mode
     * @param timeoutSeconds (not yet implemented)
     * @return current step status
     */
    virtual StepStatus BeginStep(StepMode mode,
                                 const float timeoutSeconds = 0.f);

    /**
     * Returns current step information for each engine.
     * @return current step
     */
    virtual size_t CurrentStep() const;

    /**
     * @brief Put associates variable and data into adios2 in Engine Write mode.
     * Check your Engine documentation for specific behavior.
     * In general, it will register variable metadata and data for buffering.
     * @param variable contains metadata
     * @param data contains user defined data
     * @param executeMode
     * <pre>
     * Deferred (default): lazy evaluation, can't reuse data until EndStep
     * 		               Close, or PerformPuts.
     * Sync: data can be reused after this call
     * </pre>
     * @exception
     * <pre>
     * std::invalid_argument: in debug mode, additional checks for user inputs
     * std::runtime_error: always checks for system failures
     * </pre>
     */
    template <class T>
    void Put(Variable<T> &variable, const T *data,
             const Mode launch = Mode::Deferred);

    /**
     * @brief Put version that accepts a variable name as input parameter.
     * Throws an exception if variable is not found in IO that created the
     * current engine.
     * @param variableName input variable name (Variable must exist in IO that
     * created current Engine with Open)
     * @param data contains user defined data
     * @param executeMode
     * <pre>
     * Deferred (default): lazy evaluation, can't reuse data until EndStep
     * 		               Close, or PerformPuts.
     * Sync: data can be reused after this call
     * </pre>
     * @exception
     * <pre>
     * std::invalid_argument: in debug mode, additional checks for user inputs
     * 						  also thrown if variable is not
     * found.
     * std::runtime_error: always checks for system failures
     * </pre>
     */
    template <class T>
    void Put(const std::string &variableName, const T *data,
             const Mode launch = Mode::Deferred);

    /**
     * Put version for single value datum, can accept on-the-fly values
     * e.g. Put<float>(variable, 10.f);
     * Mode is always Sync since there might be no pointer associated with the
     * single value datum (r-values)
     * @param variable contains metadata
     * @param datum contains user defined single value
     */
    template <class T>
    void Put(Variable<T> &variable, const T &datum);

    /**
     * @brief Put version for single value datum using variable name. Throws
     * an exception if variable is not found in IO that created the
     * current engine.
     *
     * Can accept on-the-fly values e.g. Put<float>("myVar", 10.f);
     * Mode is always Sync since there might be no pointer associated with
     * the single value datum (r-values)
     *
     * @param variableName input variable name (Variable must exist in IO that
     * created current Engine with Open)
     * @param datum contains user defined single value
     * @exception
     * <pre>
     * std::invalid_argument: in debug mode, additional checks for user inputs
     * 						  also thrown if variable is not
     * found.
     * std::runtime_error: always checks for system failures
     * </pre>
     */
    template <class T>
    void Put(const std::string &variableName, const T &datum);

    /**
     * @brief Get associates an existing variable selections and populates data
     * from adios2 Engine in Read Mode.
     *
     * Polymorphic function.
     * Check your Engine documentation for specific behavior.
     * In general, it will register variable metadata and data for populating
     * data values at Read.
     * @param variable contains metadata and selections for getting the variable
     * @param data user pre-allocated memory space
     * @param executeMode
     * <pre>
     * Deferred (default): lazy evaluation, data is not populated until EndStep
     * 		Close, or PerformPuts
     * Sync: data is ready after this call
     * </pre>
     * @exception
     * <pre>
     * std::invalid_argument: in debug mode, additional checks for user
     * inputs
     * std::runtime_error: always if system failures are caught
     * </pre>
     */
    template <class T>
    void Get(Variable<T> &variable, T *data,
             const Mode launch = Mode::Deferred);

    /**
     * @brief Get version that accepts a variableName as input.
     *
     * Throws an exception if variable is not found in IO that created the
     * current engine.
     *
     * @param variableName input variable name (Variable must exist in IO that
     * created current Engine with Open)
     * @param data user pre-allocated memory space
     * @param executeMode
     * <pre>
     * Deferred (default): lazy evaluation, data is not populated until EndStep
     * 		Close, or PerformPuts.
     * Sync: data is ready after this call
     * </pre>
     * @exception
     * <pre>
     * std::invalid_argument: in debug mode, additional checks for user
     * inputs, also thrown if variable is not
     * found.
     * std::runtime_error: always if system failures are caught
     * </pre>
     */
    template <class T>
    void Get(const std::string &variableName, T *data,
             const Mode launch = Mode::Deferred);

    /**
     * @brief Get version for single value datum
     * Mode is always Sync since there might be no pointer associated with
     * the
     * single value datum (r-values)
     * @param variable contains metadata
     * @param datum to be populated with corresponding value
     */
    template <class T>
    void Get(Variable<T> &variable, T &datum,
             const Mode launch = Mode::Deferred);

    /**
     * @brief Get version for single value datum using variable name
     * @param variableName input variable name (Variable must exist in IO that
     * created current Engine with Open)
     * @param datum to be populated with corresponding value
     */
    template <class T>
    void Get(const std::string &variableName, T &datum,
             const Mode launch = Mode::Deferred);

    /**
     * Convenience function, C++ only that allocates and populates a vector with
     * the requested values
     * @param variable
     * @param dataV
     * @param launch
     */
    template <class T>
    void Get(Variable<T> &variable, std::vector<T> &dataV,
             const Mode launch = Mode::Deferred);

    /**
     * Convenience function, C++ only that allocates and populates a vector with
     * the requested values
     * @param variable
     * @param dataV
     * @param launch
     */
    template <class T>
    void Get(const std::string &variableName, std::vector<T> &dataV,
             const Mode launch = Mode::Deferred);

    /**
     * Reader application indicates that no more data will be read from the
     * current stream before advancing.
     * This is necessary to allow writers to advance as soon as possible.
     */
    virtual void EndStep();

    /** Execute all Put (in deferred launch mode) starting from a previous
     * PerformPuts, BeginStep or Open */
    virtual void PerformPuts();

    /** Execute all Get (in deferred launch mode) starting from a previous
     * PerformGets, BeginStep or Open */
    virtual void PerformGets();

    /**
     * Closes a particular transport, or all if transportIndex = -1 (default).
     * @param transportIndex index returned from IO AddTransport, default (-1) =
     * all
     */
    void Close(const int transportIndex = -1);

    /**
     * Flushes data and metadata (if on) to a particular transport, or all if -1
     * (default).
     * @param transportIndex index returned from IO AddTransport, default (-1) =
     * all
     */
    virtual void Flush(const int transportIndex = -1);

    /**
     * @brief Promise that no more definitions or changes to defined variables
     * will occur. Useful information if called before the first EndStep() of an
     * output Engine, as it will know that the definitions are complete and
     * constant for the entire lifetime of the output and may optimize metadata
     * handling.
     */
    void FixedSchedule() noexcept;

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
    AllStepsBlocksInfo(const Variable<T> &variable) const;

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
    BlocksInfo(const Variable<T> &variable, const size_t step) const;

protected:
    /** IO class object that creates this Engine at Open */
    IO &m_IO;

    /** from ADIOS class passed to Engine created with Open
     *  if no new communicator is passed */
    MPI_Comm m_MPIComm;

    /** true: additional exceptions */
    const bool m_DebugMode = false;

    /** added to exceptions to improve debugging */
    std::string m_EndMessage;

    /** keeps track of current advance status */
    StepStatus m_AdvanceStatus = StepStatus::OK;

    /** keep track if the current Engine is marked for destruction in IO */
    bool m_IsClosed = false;

    /** true: No more definitions or changes to existing variables are allowed
     */
    bool m_FixedLocalSchedule = false;

    /** true: We know that the source/targe has a fixed write/read schedule
     * and this engine can utilize this fact for optimizing I/O
     */
    bool m_FixedRemoteSchedule = false;

    /** Called from constructors */
    virtual void Init();

    /** From IO SetParameters */
    virtual void InitParameters();

    /** From IO AddTransport */
    virtual void InitTransports();

#define declare_type(T)                                                        \
    virtual void DoPutSync(Variable<T> &, const T *);                          \
    virtual void DoPutDeferred(Variable<T> &, const T *);
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

// Get
#define declare_type(T)                                                        \
    virtual void DoGetSync(Variable<T> &, T *);                                \
    virtual void DoGetDeferred(Variable<T> &, T *);
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    virtual void DoClose(const int transportIndex) = 0;

    /**
     * Called by string Put/Get versions and deferred modes
     * @param variableName variable to be searched
     * @param hint extra exception information
     * @return Variable<T>& reference if found, else throws an exception in
     * debug mode
     */
    template <class T>
    Variable<T> &FindVariable(const std::string &variableName,
                              const std::string hint);

#define declare_type(T)                                                        \
    virtual std::map<size_t, std::vector<typename Variable<T>::Info>>          \
    DoAllStepsBlocksInfo(const Variable<T> &variable) const;                   \
                                                                               \
    virtual std::vector<typename Variable<T>::Info> DoBlocksInfo(              \
        const Variable<T> &variable, const size_t step) const;

    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

private:
    /** Throw exception by Engine virtual functions not implemented/supported by
     *  a derived  class */
    void ThrowUp(const std::string function) const;

    /**
     * Execute common checks in Put and Get
     * @param variable input variable
     * @param data input data
     * @param modes acceptable modes
     * @param hint extra exception info
     */
    template <class T>
    void CommonChecks(Variable<T> &variable, const T *data,
                      const std::set<Mode> &modes,
                      const std::string hint) const;

    /**
     * Checks if Engine was opened using the right Open mode for a particular
     * function
     * @param modes acceptable modes
     * @param hint extra exception info
     */
    void CheckOpenModes(const std::set<Mode> &modes,
                        const std::string hint) const;
};

#define declare_template_instantiation(T)                                      \
                                                                               \
    extern template void Engine::Put<T>(Variable<T> &, const T *, const Mode); \
    extern template void Engine::Put<T>(const std::string &, const T *,        \
                                        const Mode);                           \
                                                                               \
    extern template void Engine::Put<T>(Variable<T> &, const T &);             \
    extern template void Engine::Put<T>(const std::string &, const T &);       \
                                                                               \
    extern template void Engine::Get<T>(Variable<T> &, T *, const Mode);       \
    extern template void Engine::Get<T>(const std::string &, T *, const Mode); \
                                                                               \
    extern template void Engine::Get<T>(Variable<T> &, T &, const Mode);       \
    extern template void Engine::Get<T>(const std::string &, T &, const Mode); \
                                                                               \
    extern template void Engine::Get<T>(Variable<T> &, std::vector<T> &,       \
                                        const Mode);                           \
                                                                               \
    extern template void Engine::Get<T>(const std::string &, std::vector<T> &, \
                                        const Mode);                           \
                                                                               \
    extern template Variable<T> &Engine::FindVariable(                         \
        const std::string &variableName, const std::string hint);              \
                                                                               \
    extern template std::map<size_t, std::vector<typename Variable<T>::Info>>  \
    Engine::AllStepsBlocksInfo(const Variable<T> &variable) const;             \
                                                                               \
    extern template std::vector<typename Variable<T>::Info>                    \
    Engine::BlocksInfo(const Variable<T> &variable, const size_t step) const;

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_ENGINE_H_ */
