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

#include "core/Group.h"
#include "functions/adiosFunctions.h"

namespace adios
{

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
    Group* m_Group = nullptr; ///< Set default group

    /**
     * Constructor
     * @param type must be an engine type, default = SingleBP
     */
    Method( const std::string type, const bool debugMode = false );

    /**
     * Constructor that accepts a group reference for access to a default group access
     * @param type
     * @param group
     * @param debugMode
     */
    Method( const std::string type, Group& group, const bool debugMode = false );

    ~Method( );

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

    void SetDefaultGroup( Group& group );

private:

    void AddTransportParameters( const std::string type, const std::vector<std::string>& parameters );

};


} //end namespace


#endif /* METHOD_H_ */
