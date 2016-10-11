/*
 * SSupport.h
 *
 *  Created on: Oct 10, 2016
 *      Author: wfg
 */

#ifndef SSUPPORT_H_
#define SSUPPORT_H_

#include <set>
#include <string>
#include <map>

namespace adios
{

const std::string Version{"2.00"};

const std::set<std::string> HostLanguages {
    "C",
    "C++",
    "Fortran",
    "Python",
    "Java"
};

const std::set<std::string> TransportMethods {
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

const std::set<std::string> Tranformations {
    "none",
    "identity",
    "bzip2",
    "isobar"
};

const std::map<std::string, std::string> DataTypes {
    { "unsigned integer", "unsigned int" },
    { "integer", "int" },
    { "real" , "float" },
    { "real*8" , "double" },
    { "double" , "double" },
};












}


#endif /* SSUPPORT_H_ */
