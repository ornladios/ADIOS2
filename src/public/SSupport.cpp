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


const std::map<std::string, std::set<std::string> > SSupport::Datatypes
{
    { "C++",
        {
            "char", "std::string", "string",
            "unsigned char",
            "short", "std::vector<short>", "vector<short>",
            "unsigned short", "std::vector<unsigned short>", "vector<unsigned short>",
            "int", "std::vector<int>", "vector<int>", "integer",
            "unsigned int", "std::vector<unsigned int>", "vector<unsigned int>",
            "long int", "long",
            "unsigned long int", "std::vector<unsigned long int>", "vector<unsigned long int>", "unsigned long", "std::vector<unsigned long>", "vector<unsigned long>",
            "long long int", "long long",
            "unsigned long long int", "std::vector<unsigned long long int>", "vector<unsigned long long int>", "unsigned long long", "std::vector<unsigned long long>", "vector<unsigned long long>",
            "float", "std::vector<float>", "vector<float>",
            "float complex", "std::complex<float>", "complex<float>",
            "double", "std::vector<double>", "vector<double>",
            "long double", "std::vector<long double>", "vector<long double>",
            "double complex", "std::complex<double>", "complex<double>"
        }
    },
    { "C",
        {
            "char",
            "unsigned char",
            "short",
            "unsigned short",
            "int",
            "unsigned int",
            "long int", "long",
            "unsigned long int", "unsigned long",
            "long long int", "long long"
            "unsigned long long int", "unsigned long long",
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
            "integer",
            "real",
            "complex",
            "double precision",
            "double complex"
        }
    }
};



} //end namespace

