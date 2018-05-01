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
#include <future>
#include <map>
#include <memory> //std::shared_ptr
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

/** Base Abstract class for IO operations:  Read/Write, Schedule, Advance and
 * Close */
class Engine
{
public:
    using AdvanceAsyncCallback =
        std::function<void(std::shared_ptr<adios2::Engine>)>;

public:
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
     * Puts variable with pre-defined pointer at DefineVariable into adios2
     * buffer.
     * Launch policy:
     * <pre>
     *     Synch: Variable data memory is reusable after this call
     *     Deferred: function returns immediately so Variable data memory is
     *               not reusable until PerformPuts
     * </pre>
     * @param variable input object with metadata and pointer data contents
     */
    template <class T>
    void PutSync(Variable<T> &variable);

    void PutSync(const std::string &variableName);

    template <class T>
    void PutDeferred(Variable<T> &variable);

    void PutDeferred(const std::string &variableName);

    /**
     * Puts variable data passing new data pointer from application.
     * Launch policy:
     * <pre>
     *     Synch: Variable data memory is reusable after this call
     *     Deferred: function returns immediately so Variable data memory is
     *               not reusable until PerformPuts
     * </pre>
     * @param variable input object with metadata
     * @param values data pointer for this variable, can be reused
     */
    template <class T>
    void PutSync(Variable<T> &variable, const T *values);

    template <class T>
    void PutSync(const std::string &variableName, const T *values);

    template <class T>
    void PutDeferred(Variable<T> &variable, const T *values);

    template <class T>
    void PutDeferred(const std::string &variable, const T *values);

    /**
     * Puts variable data passing a single value from application.
     * Launch policy:
     * <pre>
     *     Synch: Variable data memory is reusable after this call
     *     Deferred: function returns immediately so Variable data memory is
     *               not reusable until PerformPuts
     * </pre>
     * @param variable input object with metadata
     * @param value single value passed by value, allows rvalues
     */
    template <class T>
    void PutSync(Variable<T> &variable, const T &value);

    template <class T>
    void PutSync(const std::string &variableName, const T &value);

    template <class T>
    void PutDeferred(Variable<T> &variable, const T &value);

    template <class T>
    void PutDeferred(const std::string &variable, const T &value);

    template <class T>
    void GetSync(Variable<T> &variable);

    template <class T>
    void GetSync(const std::string &variable);

    template <class T>
    void GetDeferred(Variable<T> &variable);

    template <class T>
    void GetDeferred(const std::string &variableName);

    template <class T>
    void GetSync(Variable<T> &variable, T *values);

    template <class T>
    void GetSync(const std::string &variableName, T *values);

    template <class T>
    void GetDeferred(Variable<T> &variable, T *values);

    template <class T>
    void GetDeferred(const std::string &variable, T *values);

    template <class T>
    void GetSync(Variable<T> &variable, T &values);

    template <class T>
    void GetSync(const std::string &variableName, T &values);

    template <class T>
    void GetDeferred(Variable<T> &variable, T &values);

    template <class T>
    void GetDeferred(const std::string &variableName, T &values);

    /**
     * Reader application indicates that no more data will be read from the
     * current stream before advancing.
     * This is necessary to allow writers to advance as soon as possible.
     */
    virtual void EndStep();

    /** Execute all Put<Deferred,T> starting from a previous PerformPuts */
    virtual void PerformPuts();

    /** Execute all Get<Deferred,T> starting from a previous PerformGets */
    virtual void PerformGets();

    /**
     * Convenience function to write all variables defined with constant
     * dimensions that are non-nullptr (with Variable.SetData(nullptr)) at once
     * in IO. This functions is only used in very straight-forward cases when
     * all variable dimensions are constant. Resizing (reallocation) generated
     * dangling pointers.
     * @exception if a variable in IO doesn't have constant dimensions it throws
     * an invalid_argument */
    void WriteStep();

    /**
     * Closes a particular transport, or all if -1 (default).
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
     * will occur.
     * Useful information if called before the first EndStep() of an output
     * Engine, as
     * it will know that the definitions are complete and constant for the
     * entire lifetime of the output and may optimize metadata handling.
     */
    void FixedSchedule() noexcept;

protected:
    /** from derived class */
    const std::string m_EngineType;

    /** IO class object that creates this Engine at Open */
    IO &m_IO;

    /** Unique name for this Engine within m_IO */
    const std::string m_Name;

    /** open mode from ADIOSTypes.h OpenMode */
    const Mode m_OpenMode;

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
    virtual void DoPutDeferred(Variable<T> &, const T *);                      \
    virtual void DoPutDeferred(Variable<T> &, const T &);
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

// Get
#define declare_type(T)                                                        \
    virtual void DoGetSync(Variable<T> &, T *);                                \
    virtual void DoGetDeferred(Variable<T> &, T *);                            \
    virtual void DoGetDeferred(Variable<T> &, T &);
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    virtual void DoClose(const int transportIndex) = 0;

private:
    /** Throw exception by Engine virtual functions not implemented/supported by
     *  a derived  class */
    void ThrowUp(const std::string function) const;

    /**
     * Called by string Put/Get versions
     * @param variableName
     * @return Variable<T> reference if found, else throws an exception in debug
     * mode
     */
    template <class T>
    Variable<T> &FindVariable(const std::string &variableName);

    /**
     * Checks if Engine was opened in Write mode so Put functions can be called
     * @param hint
     */
    void CheckWriteMode(const std::string hint) const;

    /**
     * Checks if Engine was opened in Read mode so Get functions can be called
     * @param hint
     */
    void CheckReadMode(const std::string hint) const;
};

#define declare_template_instantiation(T)                                      \
    extern template void Engine::PutSync<T>(Variable<T> &);                    \
    extern template void Engine::PutDeferred<T>(Variable<T> &);                \
                                                                               \
    extern template void Engine::PutSync<T>(Variable<T> &, const T *);         \
    extern template void Engine::PutDeferred<T>(Variable<T> &, const T *);     \
    extern template void Engine::PutSync<T>(const std::string &, const T *);   \
    extern template void Engine::PutDeferred<T>(const std::string &,           \
                                                const T *);                    \
                                                                               \
    extern template void Engine::PutSync<T>(Variable<T> &, const T &);         \
    extern template void Engine::PutDeferred<T>(Variable<T> &, const T &);     \
    extern template void Engine::PutSync<T>(const std::string &, const T &);   \
    extern template void Engine::PutDeferred<T>(const std::string &,           \
                                                const T &);                    \
                                                                               \
    extern template void Engine::GetSync<T>(Variable<T> &);                    \
    extern template void Engine::GetDeferred<T>(Variable<T> &);                \
    extern template void Engine::GetSync<T>(const std::string &);              \
    extern template void Engine::GetDeferred<T>(const std::string &);          \
                                                                               \
    extern template void Engine::GetSync<T>(Variable<T> &, T *);               \
    extern template void Engine::GetDeferred<T>(Variable<T> &, T *);           \
    extern template void Engine::GetSync<T>(const std::string &, T *);         \
    extern template void Engine::GetDeferred<T>(const std::string &, T *);     \
                                                                               \
    extern template void Engine::GetSync<T>(Variable<T> &, T &);               \
    extern template void Engine::GetDeferred<T>(Variable<T> &, T &);           \
    extern template void Engine::GetSync<T>(const std::string &, T &);         \
    extern template void Engine::GetDeferred<T>(const std::string &, T &);

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace adios2

#endif /* ADIOS2_CORE_ENGINE_H_ */
