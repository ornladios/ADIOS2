/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Engine.h
 *
 *  Created on: Nov 7, 2016
 *      Author: wfg
 */

#ifndef ENGINE_H_
#define ENGINE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <complex>    //std::complex
#include <functional> //std::function
#include <map>
#include <memory> //std::shared_ptr
#include <string>
#include <utility> //std::pair
#include <vector>
/// \endcond

#include "../ADIOS_MPI.h"

#include "../ADIOS.h"
#include "../ADIOSTypes.h"
#include "Capsule.h"
#include "Method.h"
#include "Transform.h"
#include "Transport.h"
#include "Variable.h"
#include "VariableCompound.h"

namespace adios
{

typedef enum { NONBLOCKINGREAD = 0, BLOCKINGREAD = 1 } PerformReadMode;

typedef enum {
    APPEND = 0,
    UPDATE = 1, // writer advance modes
    NEXT_AVAILABLE = 2,
    LATEST_AVAILABLE = 3, // reader advance modes
} AdvanceMode;

/**
 * Base class for Engine operations managing shared-memory, and buffer and
 * variables transform and transport operations
 */
class Engine
{

public:
    MPI_Comm m_MPIComm = MPI_COMM_SELF;

    const std::string m_EngineType; ///< from derived class
    const std::string m_Name;       ///< name used for this engine
    const std::string
        m_AccessMode;       ///< accessMode for buffers used by this engine
    const Method &m_Method; ///< associated method containing engine metadata

    int m_RankMPI = 0; ///< current MPI rank process
    int m_SizeMPI = 1; ///< current MPI processes size

    const std::string m_HostLanguage = "C++"; ///< default host language

    /**
     * Unique constructor
     * @param adios
     * @param engineType
     * @param name
     * @param accessMode
     * @param mpiComm
     * @param method
     * @param endMessage
     */
    Engine(ADIOS &adios, const std::string engineType, const std::string &name,
           const std::string accessMode, MPI_Comm mpiComm, const Method &method,
           const std::string endMessage);

    virtual ~Engine() = default;

    /** @brief Let ADIOS allocate memory for a variable, which can be used by
     * the user.
     * To decrease the cost of copying memory, a user may let ADIOS allocate the
     * memory for a user-variable,
     * according to the definition of an ADIOS-variable. The memory will be part
     * of the ADIOS buffer used
     * by the engine and it lives until the engine (file, stream) is closed.
     * A variable that has been allocated this way (cannot have its local
     * dimensions changed, and AdvanceAsync() should be
     * used instead of Advance() and the user-variable must not be modified by
     * the application until the notification arrives.
     * This is required so that any reader can access the written data before
     * the application overwrites it.
     * @param var Variable with defined local dimensions and offsets in global
     * space
     * @param fillValue Fill the allocated array with this value
     * @return A constant pointer to the non-constant allocated array. User
     * should
     * not deallocate this pointer.
     */
    template <class T>
    inline T *const AllocateVariable(Variable<T> &var, T fillValue = 0)
    {
        throw std::invalid_argument("ERROR: type not supported for variable " +
                                    var->name + " in call to GetVariable\n");
    }

    /**
     * Needed for DataMan Engine
     * @param callback function passed from the user
     */
    virtual void SetCallBack(std::function<void(const void *, std::string,
                                                std::string, std::string, Dims)>
                                 callback);

    /**
     * Write function that adds static checking on the variable to be passed by
     * values
     * It then calls its corresponding derived class virtual function
     * This version uses m_Group to look for the variableName.
     * @param variable name of variable to the written
     * @param values pointer passed from the application
     */
    template <class T>
    void Write(Variable<T> &variable, const T *values)
    {
        Write(variable, values);
    }

    /**
     * String version
     * @param variableName
     * @param values
     */
    template <class T>
    void Write(const std::string &variableName, const T *values)
    {
        Write(variableName, values);
    }

    /**
     * Single value version
     * @param variable
     * @param values
     */
    template <class T>
    void Write(Variable<T> &variable, const T values)
    {
        const T val = values;
        Write(variable, &val);
    }

    /**
     * Single value version using string as variable handlers, allows rvalues to
     * be passed
     * @param variableName
     * @param values
     */
    template <class T>
    void Write(const std::string &variableName, const T values)
    {
        const T val = values;
        Write(variableName, &val);
    }

    virtual void Write(Variable<char> &variable, const char *values);
    virtual void Write(Variable<unsigned char> &variable,
                       const unsigned char *values);
    virtual void Write(Variable<short> &variable, const short *values);
    virtual void Write(Variable<unsigned short> &variable,
                       const unsigned short *values);
    virtual void Write(Variable<int> &variable, const int *values);
    virtual void Write(Variable<unsigned int> &variable,
                       const unsigned int *values);
    virtual void Write(Variable<long int> &variable, const long int *values);
    virtual void Write(Variable<unsigned long int> &variable,
                       const unsigned long int *values);
    virtual void Write(Variable<long long int> &variable,
                       const long long int *values);
    virtual void Write(Variable<unsigned long long int> &variable,
                       const unsigned long long int *values);
    virtual void Write(Variable<float> &variable, const float *values);
    virtual void Write(Variable<double> &variable, const double *values);
    virtual void Write(Variable<long double> &variable,
                       const long double *values);
    virtual void Write(Variable<std::complex<float>> &variable,
                       const std::complex<float> *values);
    virtual void Write(Variable<std::complex<double>> &variable,
                       const std::complex<double> *values);
    virtual void Write(Variable<std::complex<long double>> &variable,
                       const std::complex<long double> *values);
    virtual void Write(VariableCompound &variable, const void *values);

    /**
     * @brief Write functions can be overridden by derived classes. Base class
     * behavior is to:
     * 1) Write to Variable values (m_Values) using the pointer to default group
     * *m_Group set with SetDefaultGroup function
     * 2) Transform the data
     * 3) Write to all capsules -> data and metadata
     * @param variableName
     * @param values coming from user app
     */
    virtual void Write(const std::string &variableName, const char *values);
    virtual void Write(const std::string &variableName,
                       const unsigned char *values);
    virtual void Write(const std::string &variableName, const short *values);
    virtual void Write(const std::string &variableName,
                       const unsigned short *values);
    virtual void Write(const std::string &variableName, const int *values);
    virtual void Write(const std::string &variableName,
                       const unsigned int *values);
    virtual void Write(const std::string &variableName, const long int *values);
    virtual void Write(const std::string &variableName,
                       const unsigned long int *values);
    virtual void Write(const std::string &variableName,
                       const long long int *values);
    virtual void Write(const std::string &variableName,
                       const unsigned long long int *values);
    virtual void Write(const std::string &variableName, const float *values);
    virtual void Write(const std::string &variableName, const double *values);
    virtual void Write(const std::string &variableName,
                       const long double *values);
    virtual void Write(const std::string &variableName,
                       const std::complex<float> *values);
    virtual void Write(const std::string &variableName,
                       const std::complex<double> *values);
    virtual void Write(const std::string &variableName,
                       const std::complex<long double> *values);
    virtual void Write(const std::string &variableName, const void *values);

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
    void Read(Variable<T> &variable, const T *values)
    {
        Read(variable, values);
    }

    /**
     * String version
     * @param variableName
     * @param values
     */
    template <class T>
    void Read(const std::string variableName, const T *values)
    {
        Read(variableName, values);
    }

    /**
     * Single value version
     * @param variable
     * @param values
     */
    template <class T>
    void Read(Variable<T> &variable, const T &values)
    {
        Read(variable, &values);
    }

    /**
     * Single value version using string as variable handlers
     * @param variableName
     * @param values
     */
    template <class T>
    void Read(const std::string variableName, const T &values)
    {
        Read(variableName, &values);
    }

    /**
     * Unallocated version, ADIOS will allocate space for incoming data
     * @param variable
     */
    template <class T>
    void Read(Variable<T> &variable)
    {
        Read(variable, nullptr);
    }

    /**
     * Unallocated version, ADIOS will allocate space for incoming data
     * @param variableName
     */
    template <class T>
    void Read(const std::string variableName)
    {
        Read(variableName, nullptr);
    }

    virtual void Read(Variable<double> &variable, const double *values);

    /**
     * Read function that adds static checking on the variable to be passed by
     * values
     * It then calls its corresponding derived class virtual function
     * This version uses m_Group to look for the variableName.
     * @param variable name of variable to the written
     * @param values pointer passed from the application
     */
    template <class T>
    void ScheduleRead(Variable<T> &variable, T *values)
    {
        ScheduleRead(variable, values);
    }

    /**
     * String version
     * @param variableName
     * @param values
     */
    template <class T>
    void ScheduleRead(const std::string variableName, T *values)
    {
        ScheduleRead(variableName, values);
    }

    /**
     * Single value version
     * @param variable
     * @param values
     */
    template <class T>
    void ScheduleRead(Variable<T> &variable, T &values)
    {
        ScheduleRead(variable, &values);
    }

    /**
     * Single value version using string as variable handlers
     * @param variableName
     * @param values
     */
    template <class T>
    void ScheduleRead(const std::string variableName, T &values)
    {
        ScheduleRead(variableName, &values);
    }

    /**
     * Unallocated version, ADIOS will allocate space for incoming data
     * @param variableName
     */
    void ScheduleRead(const std::string variableName)
    {
        ScheduleRead(variableName, nullptr);
    }

    /**
     * Unallocated unspecified version, ADIOS will receive any variable and will
     * allocate space for incoming data
     */
    void ScheduleRead() { ScheduleRead(nullptr, nullptr); }

    virtual void ScheduleRead(Variable<double> &variable, double *values);
    virtual void ScheduleRead(const std::string variableName, double *values);

    /**
     * Perform all scheduled reads, either blocking until all reads completed,
     * or
     * return immediately.
     * @param mode Blocking or non-blocking modes
     */
    virtual void PerformReads(PerformReadMode mode);

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
    virtual void Advance(const float timeout_sec = 0.0);

    /**
     * Indicates that a new step is going to be written as new variables come
     * in.
     * @param mode Advance mode, there are different options for writers and
     * readers
     */
    virtual void Advance(AdvanceMode mode, const float timeout_sec = 0.0);

    /** @brief Advance asynchronously and get a callback when readers release
     * access to the buffered step.
     *
     * User variables that were allocated through AllocateVariable()
     * must not be modified until advance is completed.
     * @param mode Advance mode, there are different options for writers and
     * readers
     * @param callback Will be called when advance is completed.
     */
    virtual void
    AdvanceAsync(AdvanceMode mode,
                 std::function<void(std::shared_ptr<adios::Engine>)> callback);

    // Read API
    /**
     * Inquires and (optionally) allocates and copies the contents of a variable
     * If success: it returns a pointer to the internal stored variable object
     * in
     * ADIOS class.
     * If failure: it returns nullptr
     * @param name variable name to look for
     * @param readIn if true: reads the full variable and payload, allocating
     * values in memory, if false: internal payload is nullptr
     * @return success: it returns a pointer to the internal stored variable
     * object in ADIOS class, failure: nullptr
     */
    virtual Variable<void> *InquireVariable(const std::string &variableName,
                                            const bool readIn = true);
    virtual Variable<char> *InquireVariableChar(const std::string &variableName,
                                                const bool readIn = true);
    virtual Variable<unsigned char> *
    InquireVariableUChar(const std::string &variableName,
                         const bool readIn = true);
    virtual Variable<short> *
    InquireVariableShort(const std::string &variableName,
                         const bool readIn = true);

    virtual Variable<unsigned short> *
    InquireVariableUShort(const std::string &variableName,
                          const bool readIn = true);

    virtual Variable<int> *InquireVariableInt(const std::string &variableName,
                                              const bool readIn = true);
    virtual Variable<unsigned int> *
    InquireVariableUInt(const std::string &variableName,
                        const bool readIn = true);

    virtual Variable<long int> *
    InquireVariableLInt(const std::string &variableName,
                        const bool readIn = true);

    virtual Variable<unsigned long int> *
    InquireVariableULInt(const std::string &variableName,
                         const bool readIn = true);

    virtual Variable<long long int> *
    InquireVariableLLInt(const std::string &variableName,
                         const bool readIn = true);

    virtual Variable<unsigned long long int> *
    InquireVariableULLInt(const std::string &variableName,
                          const bool readIn = true);

    virtual Variable<float> *
    InquireVariableFloat(const std::string &variableName,
                         const bool readIn = true);

    virtual Variable<double> *
    InquireVariableDouble(const std::string &variableName,
                          const bool readIn = true);

    virtual Variable<long double> *
    InquireVariableLDouble(const std::string &variableName,
                           const bool readIn = true);

    virtual Variable<std::complex<float>> *
    InquireVariableCFloat(const std::string &variableName,
                          const bool readIn = true);

    virtual Variable<std::complex<double>> *
    InquireVariableCDouble(const std::string &variableName,
                           const bool readIn = true);

    virtual Variable<std::complex<long double>> *
    InquireVariableCLDouble(const std::string &variableName,
                            const bool readIn = true);

    virtual VariableCompound *
    InquireVariableCompound(const std::string &variableName,
                            const bool readIn = true);

    /** Return the names of all variables present in a stream/file opened for
     * reading
     *
     * @return a vector of strings
     */
    std::vector<std::string> VariableNames();

    /**
     * Closes a particular transport, or all if -1
     * @param transportIndex order from Method AddTransport
     */
    virtual void Close(const int transportIndex = -1) = 0;

protected:
    ADIOS &m_ADIOS; ///< creates Engine at Open
    std::vector<std::shared_ptr<Transport>>
        m_Transports;               ///< transports managed
    const bool m_DebugMode = false; ///< true: additional exceptions checks
    unsigned int m_nThreads = 0;    ///< from Method nthreads
    const std::string
        m_EndMessage; ///< added to exceptions to improve debugging
    std::set<std::string> m_WrittenVariables;

    virtual void Init();           ///< Initialize m_Capsules and m_Transports
    virtual void InitParameters(); ///< Initialize parameters from Method
    virtual void InitTransports(); ///< Initialize transports from Method

    /**
     * Used to verify parameters in m_Method containers
     * @param itParameter iterator to a certain parameter
     * @param parameters map of parameters, from m_Method
     * @param parameterName used if exception is thrown to provide debugging
     * information
     * @param hint used if exception is thrown to provide debugging information
     */
    void CheckParameter(
        const std::map<std::string, std::string>::const_iterator itParameter,
        const std::map<std::string, std::string> &parameters,
        const std::string parameterName, const std::string hint) const;

    bool TransportNamesUniqueness() const; ///< checks if transport names are
                                           /// unique among the same types (file
                                           /// I/O)

    /**
     * Throws an exception in debug mode if transport index is out of range.
     * @param transportIndex must be in the range [ -1 , m_Transports.size()-1 ]
     */
    void CheckTransportIndex(const int transportIndex);
};

} // end namespace

#endif /* ENGINE_H_ */
