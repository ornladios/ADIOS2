/*
 * SSupport.cpp
 *
 *  Created on: Oct 18, 2016
 *      Author: wfg
 */


#include "public/SSupport.h"


namespace adios
{

const std::string SSupport::Version{ "2.00" };

const std::set<std::string> SSupport::HostLanguages{
    { "C", "C++", "Fortran", "Python", "Java" }
};

const std::set<std::string> SSupport::Transports{
    { "NULL", "POSIX", "FStream", "MPI", "MPI_LUSTRE", "MPI_AGGREGATE", "DATASPACES", "DIMES", "FLEXPATH", "PHDF5", "NC4", "ICEE" }
};

const std::set<std::string> SSupport::Transforms{
    { "none", "identity", "bzip2", "isobar" }
};

const std::map<std::string, std::string> SSupport::Datatypes{
    { "unsigned integer", "unsigned int" }, { "integer", "int" },
    { "real" , "float" }, { "float" , "float" },
    { "real*8" , "double" }, { "double" , "double" }
};


} //end namespace

