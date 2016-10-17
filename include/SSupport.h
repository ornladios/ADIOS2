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
/// \endcond

namespace adios
{

const std::string c_Version{"2.00"}; ///< current version

/**
 * Supported host languages
 */
const std::set<std::string> c_HostLanguages {
    "C",
    "C++",
    "Fortran",
    "Python",
    "Java"
};

/**
 * Available transport methods
 */
const std::set<std::string> c_Transports {
    "NULL",
    "POSIX",
    "MPI",
    "MPI_LUSTRE",
    "MPI_AGGREGATE",
    "DATASPACES",
    "DIMES",
    "FLEXPATH",
    "PHDF5",
    "NC4",
    "ICEE"
};

/**
 * Available transformations
 */
const std::set<std::string> c_Transformations {
    "none",
    "identity",
    "bzip2",
    "isobar"
};

/**
 * Supported data types
 */
const std::map<std::string, std::string> c_DataTypes {
    { "unsigned integer", "unsigned int" },
    { "integer", "int" },
    { "real" , "float" },
    { "float" , "float" },
    { "real*8" , "double" },
    { "double" , "double" },
};


} //end namespace


#endif /* SSUPPORT_H_ */
