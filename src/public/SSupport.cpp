/*
 * SSupport.cpp
 *
 *  Created on: Oct 18, 2016
 *      Author: wfg
 */


#include "public/SSupport.h"


namespace adios
{

SSupport::SSupport( ):
    Version{ std::string
        ( "2.00" ) },

    HostLanguages{ std::set<std::string>
        { "C", "C++", "Fortran", "Python", "Java" } },

    Transports{ std::set<std::string>
        { "NULL", "POSIX", "MPI", "MPI_LUSTRE", "MPI_AGGREGATE", "DATASPACES", "DIMES", "FLEXPATH", "PHDF5", "NC4", "ICEE" } },

    Transforms{ std::set<std::string>
        { "none", "identity", "bzip2", "isobar" } },

    Datatypes{ std::map<std::string, std::string>
        {
            { "unsigned integer", "unsigned int" }, { "integer", "int" },
            { "real" , "float" }, { "float" , "float" },
            { "real*8" , "double" }, { "double" , "double" } }
        }
{ }



} //end namespace

