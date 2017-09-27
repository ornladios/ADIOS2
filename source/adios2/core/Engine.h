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
     * @param io object that generates this Engine
     * @param name unique engine name within IO class object
     * @param openMode  open mode from ADIOSTypes.h OpenMode
     * @param mpiComm new communicator passed at Open or from ADIOS class
     */
    Engine(const std::string engineType, IO &io, const std::string &name,
           const Mode openMode, MPI_Comm mpiComm);

    virtual ~Engine() = default;

    void SetStep(const size_t step = 0);

    /**
     * Write function that adds static checking on the variable to be passed by
     * values
     * It then calls its corresponding derived class virtual function
     * This version uses m_Group to look for the variableName.
     * @param variable name of variable to the written
     * @param values pointer passed from the application
     */
    template <class T>
    void Write(Variable<T> &variable, const T *values);

    /**
     * Single value version
     * @param variable
     * @param values
     */
    template <class T>
    void Write(Variable<T> &variable, const T value);

    /**
     * String version
     * @param variableName
     * @param values
     */
    template <class T>
    void Write(const std::string &variableName, const T *values);

    /**
     * Single value version using string as variable handlers, allows
     * rvalues to
     * be passed
     * @param variableName
     * @param values
     */
    template <class T>
    void Write(const std::string &variableName, const T value);

    /**
     * Runtime version for either Variable<T> or VariableCompound
     * @param variable
     * @param values
     */
    void Write(VariableBase &variable, const void *values);

    /**
     * Runtime version
     * @param variableName
     * @param values
     */
    void Write(const std::string &variableName, const void *values);

    /**
     * Inquires if a variable of a certain type exists.
     * Returns a pointer reference, not need to manage memory.
     * @param variableName input to search
     * @return pointer reference to existing variable, nullptr if variable
     * doesn't exist
     */
    template <class T>
    Variable<T> *InquireVariable(const std::string &variableName);

    VariableBase *InquireVariableAny(const std::string &variableName);

    /**
     * Read function that adds static checking on the variable to be passed by
     * values
     * It then calls its corresponding derived class virtual function
     * This version uses m_Group to look for the variableName.
     * @param variable name of variable to the written
     * @param values pointer passed from the application, nullptr not allowed,
     * must use Read(variable) instead intentionally
     */
    template <class T>
    void Read(Variable<T> &variable, T *values);

    /**
     * String version
     * @param variableName
     * @param values
     */
    template <class T>
    void Read(const std::string &variableName, T *values);

    /**
     * Reader application indicates that no more data will be read from the
     * current stream before advancing.
     * This is necessary to allow writers to advance as soon as possible.
     */
    virtual void Release();

    /**
     * Indicates that a new step is going to be written as new variables come
     * in.
     */
    virtual void Advance(const float timeoutSeconds = 0.0);

    /**
     * Indicates that a new step is going to be written as new variables come
     * in.
     * @param mode Advance mode, there are different options for writers and
     * readers
     */
    virtual void Advance(const AdvanceMode mode,
                         const float timeoutSeconds = 0.0);

    /** @brief Advance asynchronously and get a callback when readers release
     * access to the buffered step.
     *
     * User variables that were allocated through AllocateVariable()
     * must not be modified until advance is completed.
     * @param mode Advance mode, there are different options for writers and
     * readers
     * @param callback Will be called when advance is completed.
     */
    //    virtual void AdvanceAsync(const AdvanceMode mode,
    //                              AdvanceAsyncCallback callback);
    std::future<void> AdvanceAsync(
        AdvanceMode mode,
        std::function<void(std::shared_ptr<adios2::Engine>)> /*callback*/);

    AdvanceStatus GetAdvanceStatus();

    /**
     * Needed for DataMan Engine
     * @param callback function passed from the user
     */
    virtual void
    SetCallBack(std::function<void(const void *, std::string, std::string,
                                   std::string, std::vector<size_t>)>
                    callback);

    /** Return the names of all variables present in a stream/file opened for
     * reading
     * @return a map of strings key: name, value : type
     */
    std::map<std::string, std::string> GetAvailableVariables() const;

    /**
     * Closes a particular transport, or all if -1
     * @param transportIndex order from IO AddTransport
     */
    virtual void Close(const int transportIndex = -1) = 0;

    virtual void Flush(const int transportIndex = -1);
    virtual void Get(const int transportIndex = -1);

    virtual std::future<void> FlushAsync(const int transportIndex = -1);
    virtual std::future<void> GetAsync(const int transportIndex = -1);

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

    /** Tracks written variables */
    std::set<std::string> m_WrittenVariables;

    AdvanceStatus m_AdvanceStatus = AdvanceStatus::OK;

    /** Called from constructors */
    virtual void Init();

    /** From IO SetParameters */
    virtual void InitParameters();

    /** From IO AddTransport */
    virtual void InitTransports();

// Known-type
#define declare_type(T)                                                        \
    virtual void DoWrite(Variable<T> &variable, const T *values);
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    virtual void DoWrite(VariableCompound &variable, const void *values);

    /**
     * Finds the variable and call the corresponding DoWrite by
     * type
     * @param variableName
     * @param values application values
     */
    void DoWrite(const std::string &variableName, const void *values);

// READ
#define declare(T, L)                                                          \
    virtual Variable<T> *DoInquireVariable##L(const std::string &variableName);

    ADIOS2_FOREACH_TYPE_2ARGS(declare)
#undef declare

#define declare_type(T) virtual void DoRead(Variable<T> &variable, T *values);

    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

private:
    /** Throw exception by Engine virtual functions not implemented by a derived
     * class */
    void ThrowUp(const std::string function) const;
}; // END OF CLASS

#define declare_template_instantiation(T)                                      \
    extern template void Engine::Write<T>(Variable<T> &, const T *);           \
    extern template void Engine::Write<T>(Variable<T> &, const T);             \
                                                                               \
    extern template void Engine::Write<T>(const std::string &, const T *);     \
    extern template void Engine::Write<T>(const std::string &, const T);       \
                                                                               \
    extern template void Engine::Read<T>(Variable<T> &, T *);                  \
    extern template void Engine::Read<T>(const std::string &, T *);

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace adios2

#include "Engine.inl"

#endif /* ADIOS2_CORE_ENGINE_H_ */
