/*
 * Method.h
 *
 *  Created on: Dec 16, 2016
 *      Author: wfg
 */

#ifndef METHOD_H_
#define METHOD_H_

#include <vector>
#include <string>
#include <map>


namespace adios
{

/**
 * Serves as metadata to define an engine
 */
class Method
{

public:

    const std::string m_Type = "SingleBP"; ///< Method type
    const bool m_DebugMode = false; ///< true: on, throws exceptions and do additional checks, false: off, faster, but unsafe
    std::vector< std::map<std::string, std::string> > m_CapsuleParameters; ///< each is a separate Transport containing their own parameters
    std::vector< std::map<std::string, std::string> > m_TransportParameters; ///< each is a separate Transport containing their own parameters

    /**
     * Unique constructor, must have a type
     * @param type must be an engine type, default = SingleBP
     */
    Method( const std::string type = "", const bool debugMode = false );

    ~Method( );

    template< class ...Args>
    void AddCapsule( Args... args )
    {
        std::vector<std::string> parameters = { args... };
        AddCapsuleParameters( parameters );
    }

    template< class ...Args>
    void AddTransport( Args... args )
    {
        std::vector<std::string> parameters = { args... };
        AddTransportParameters( parameters );
    }

private:

    void AddCapsuleParameters( const std::vector<std::string>& parameters );
    void AddTransportParameters( const std::vector<std::string>& parameters );

};


} //end namespace


#endif /* METHOD_H_ */
