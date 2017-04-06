/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Method.h
 *
 *  Created on: Dec 16, 2016
 *      Author: wfg
 */

#ifndef METHOD_H_
#define METHOD_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <map>
#include <string>
#include <vector>
/// \endcond

#include "ADIOSTypes.h"
#include "functions/adiosFunctions.h"

namespace adios
{

typedef enum {
    GLOBAL_READERS = 2,
    ROUNDROBIN_READERS = 3,
    FIFO_READERS = 4,
    OPEN_ALL_STEPS = 5
} ReadMultiplexPattern;

typedef enum {
    NOWAITFORSTREAM = 0,
    WAITFORSTREAM = 1
} StreamOpenMode; // default: wait for stream

/**
 * Serves as metadata to define an engine
 */
class Method
{

public:
    const std::string m_Name;       ///< Method name (as defined in XML)
    const bool m_DebugMode = false; ///< true: on, throws exceptions and do
                                    /// additional checks, false: off, faster
    /// unsafe
    std::string m_Type; ///< Method's engine type
    unsigned int m_nThreads = 1;
    adios::IOMode m_IOMode = adios::IOMode::INDEPENDENT;

    std::map<std::string, std::string> m_Parameters; ///< method parameters
    std::vector<std::map<std::string, std::string>>
        m_TransportParameters; ///< each is a separate Transport containing
                               /// their
                               /// own parameters

    /**
     * Constructor
     * @param name is a label that can be used in the config file to set up the
     * method at runtime
     */
    Method(const std::string name, const bool debugMode = false);

    ~Method();

    /** Check if the method was defined by the user in the config file.
     * @return true if the method was user-defined, false otherwise when method
     * is
     * set with default parameters
     */
    bool IsUserDefined();

    /**
     * Define the engine type
     * @param type must be a valid engine type
     */
    void SetEngine(const std::string type);

    /**
     * Set how many threads the engine can use for its operations (e.g. file io,
     * compression, staging).
     * If 1 is allowed, no extra threads will be created during the ADIOS calls
     * for asynchronous operations.
     * Note that some transports may require and use extra thread(s). See their
     * documentation for their
     * requirements. E.g. some staging transports always create an extra thread
     * for communication.
     * Set this parameter like you set it for OpenMP, i.e. count one thread for
     * the main process that calls
     * ADIOS functions.
     * @param nThreads, minimum 1 is required
     */
    void AllowThreads(const unsigned int nThreads);

    /**
     * Sets parameters for the method in "parameter=value" format
     * @param args list of parameters with format "parameter1=value1", ...,
     * "parameterN=valueN"
     */
    template <class... Args>
    void SetParameters(Args... args)
    {
        std::vector<std::string> parameters = {args...};
        m_Parameters = BuildParametersMap(parameters, m_DebugMode);
    }

    /**
     * Adds a transport and its parameters for the method
     * @param type must be a supported transport type under /include/transport
     * @param args list of parameters for a transport with format
     * "parameter1=value1", ..., "parameterN=valueN"
     */
    template <class... Args>
    void AddTransport(const std::string type, Args... args)
    {
        std::vector<std::string> parameters = {args...};
        AddTransportParameters(type, parameters);
    }

    void SetReadMultiplexPattern(
        const ReadMultiplexPattern
            pattern); // How to split stream content among readers
    void SetStreamOpenMode(const StreamOpenMode mode); // In Read mode, should
                                                       // Open() wait for the
                                                       // first step appear
                                                       // (default)

    void SetVerbose(const Verbose verbose = Verbose::WARN)
    {
        m_Verbose = verbose;
    };
    Verbose GetVerbose() { return m_Verbose; };

private:
    Verbose m_Verbose = Verbose::WARN;

    void AddTransportParameters(const std::string type,
                                const std::vector<std::string> &parameters);
};

} // end namespace adios

#endif /* METHOD_H_ */
