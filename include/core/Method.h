/*
 * Method.h
 *
 *  Created on: Dec 16, 2016
 *      Author: wfg
 */

#ifndef METHOD_H_
#define METHOD_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <vector>
#include <string>
#include <map>
/// \endcond

#include "functions/adiosFunctions.h"

namespace adios
{

typedef enum {
    GLOBAL_READERS = 2, ROUNDROBIN_READERS = 3, FIFO_READERS = 4,
    OPEN_ALL_STEPS = 5
} ReadMultiplexPattern;

typedef enum { NOWAITFORSTREAM = 0, WAITFORSTREAM = 1 } StreamOpenMode; // default: wait for stream

/**
 * Serves as metadata to define an engine
 */
class Method
{

public:

    const std::string m_Type; ///< Method type
    const bool m_DebugMode = false; ///< true: on, throws exceptions and do additional checks, false: off, faster, but unsafe
    std::map<std::string, std::string> m_Parameters; ///< method parameters
    std::vector< std::map<std::string, std::string> > m_TransportParameters; ///< each is a separate Transport containing their own parameters

    /**
     * Constructor
     * @param name is a label that can be used in the config file to set up the method at runtime
     */
    Method( const std::string name, const bool debugMode = false );

    ~Method( );

    /** Check if the method was defined by the user in the config file.
     * @return true if the method was user-defined, false otherwise when method is set with default parameters
     */
    bool isUserDefined();


    /**
     * Define the engine type
     * @param type must be a valid engine type
     */
    void SetEngine( const std::string type );


    /**
     * Tell how many cores the engine can use for its operations.
     * If only 1 core is specified, no extra threads will be created during the ADIOS calls, except
     * that staging always creates an extra thread for communication.
     * @param number of cores, minimum 1 is required
     */
    void SetAvailableCores( const int nCores );

    /**
     * Sets parameters for the method in "parameter=value" format
     * @param args list of parameters with format "parameter1=value1", ..., "parameterN=valueN"
     */
    template< class ...Args>
    void SetParameters( Args... args )
    {
        std::vector<std::string> parameters = { args... };
        m_Parameters = BuildParametersMap( parameters, m_DebugMode );
    }

    /**
     * Adds a transport and its parameters for the method
     * @param type must be a supported transport type under /include/transport
     * @param args list of parameters for a transport with format "parameter1=value1", ..., "parameterN=valueN"
     */
    template< class ...Args>
    void AddTransport( const std::string type, Args... args )
    {
        std::vector<std::string> parameters = { args... };
        AddTransportParameters( type, parameters );
    }

    void SetReadMultiplexPattern( const ReadMultiplexPattern pattern );  // How to split stream content among readers
    void SetStreamOpenMode( const StreamOpenMode mode);  // In Read mode, should Open() wait for the first step appear  (default)


private:

    void AddTransportParameters( const std::string type, const std::vector<std::string>& parameters );

};


} //end namespace


#endif /* METHOD_H_ */
