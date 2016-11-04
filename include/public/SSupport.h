/*
 * SSupport.h
 *
 *  Created on: Oct 10, 2016
 *      Author: wfg
 */

#ifndef SSUPPORT_H_
#define SSUPPORT_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <set>
#include <string>
#include <map>
#include <array>
/// \endcond

namespace adios
{

struct SSupport
{
    static const std::string Version; ///< current ADIOS version
    static const std::set<std::string> HostLanguages; ///< supported languages: C, C++, Fortran, Python, Java
    static const std::set<std::string> Transports; ///< supported transport methods
    static const std::set<std::string> Transforms; ///< supported data transform methods
    static const std::map<std::string, std::set<std::string> > Datatypes; ///< supported data types

};


} //end namespace


#endif /* SSUPPORT_H_ */
