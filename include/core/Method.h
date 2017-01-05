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


namespace adios
{

/**
 * Serves as metadata to define an engine
 */
struct Method
{
    std::string Name; ///< Method name
    std::vector< std::string > Capsules; ///< Capsule type
    std::map< std::string, std::vector<std::string> > Transports; ///< key: transports, value: arguments to Transport
};


} //end namespace


#endif /* METHOD_H_ */
