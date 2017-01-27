/*
 * Support.cpp
 *
 *  Created on: Oct 18, 2016
 *      Author: wfg
 */


#include "core/Support.h"


namespace adios
{


const std::string Support::Version{ "2.00" };


const std::set<std::string> Support::HostLanguages{
    { "C++", "C", "Fortran" }
};


const std::set<std::string> Support::Transports{
    { "NULL", "POSIX", "FStream", "MdtmMan" } // "MPI", "MPI_LUSTRE", "MPI_AGGREGATE", "DATASPACES", "DIMES", "FLEXPATH", "PHDF5", "NC4", "ICEE" }
};


const std::set<std::string> Support::Transforms{
    { "none", "identity", "bzip2", "isobar", "szip" , "zlib" }
};


const std::map<std::string, std::set<std::string> > Support::Datatypes
{
    { "C++",
        {
            "char", "std::string", "string",
            "unsigned char",
            "short",
            "unsigned short",
            "int", "integer",
            "unsigned int",
            "long int", "long",
            "unsigned long int", "unsigned long",
            "long long int", "long long",
            "unsigned long long int", "unsigned long long",
            "float",
            "float complex",
            "double",
            "long double",
            "double complex", "std::complex<double>", "complex<double>"
        }
    },
    { "C",
        {
            "char",
            "unsigned char",
            "short",
            "unsigned short",
            "int", "integer"
            "unsigned int", "unsigned integer",
            "long int", "long", "long integer",
            "unsigned long int", "unsigned long", "unsigned long integer",
            "long long int", "long long", "long long integer",
            "unsigned long long int", "unsigned long long", "unsigned long long integer",
            "float",
            "float complex"
            "double",
            "long double",
            "double complex"
        }
    },
    { "Fortran",
        {
            "character",
            "integer*2",
            "integer", "real*2",
            "real", "real*4",
            "double precision", "real*8",
            "complex",
            "double complex"
        }
    }
};


const std::map<std::string, std::set<std::string> > Support::DatatypesAliases
{
    { "char",           { "char", "character" }  },
    { "unsigned char",  { "unsigned char", "unsigned character" }  },
    { "short",          { "short", "integer*2" } },
    { "unsigned short", { "unsigned short" }  },
    { "int",                    { "int", "integer" } },
    { "unsigned int",           { "unsigned int", "unsigned integer" } },
    { "long int",               { "long int", "long", "long integer" } },
    { "unsigned long int",      { "unsigned long int", "unsigned long", "unsigned long integer" } },
    { "long long int",          { "long long int", "long long", "long long integer" } },
    { "unsigned long long int", { "unsigned long long int", "unsigned long long", "unsigned long long integer" } },
    { "float", { "float", "real", "real*4" } },
    { "double", { "double", "double precision", "real*8" } },
    { "long double", { "long double", "long double precision", "real*16" } }
};


} //end namespace

