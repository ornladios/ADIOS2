/*
 * ADIOSPy.h
 *
 *  Created on: Mar 13, 2017
 *      Author: wfg
 */

#ifndef ADIOSPY_H_
#define ADIOSPY_H_

#include <string>
#include <memory> //std::shared_ptr

#ifdef HAVE_BOOSTPYTHON
  #include "boost/python.hpp"
#endif

#ifdef HAVE_PYBIND11
  #include "pybind11/pybind11.h"
#endif

#include "ADIOS.h"
#include "adiosPyFunctions.h" //ListToVector, VectorToList
#include "VariablePy.h"
#include "MethodPy.h"
#include "EnginePy.h"

namespace adios
{

#ifdef HAVE_BOOSTPYTHON
using pyList = boost::python::list;
using pyObject = boost::python::object;
#endif

#ifdef HAVE_PYBIND11
using pyList = pybind11::list;
using pyObject = pybind11::object;
#endif



class ADIOSPy : public ADIOS
{

public:

    ADIOSPy( MPI_Comm mpiComm, const bool debug );
    ~ADIOSPy( );

    void HelloMPI( ); ///< says hello from rank/size for testing


    template<class T> inline
    VariablePy<T>& DefineVariablePy( const std::string name,
                                     const pyList localDimensionsPy = pyList(),
                                     const pyList globalDimensionsPy = pyList(),
                                     const pyList globalOffsetsPy = pyList()   )
    {
        Variable<T>& var = DefineVariable<T>( name, ListToVector( localDimensionsPy ), ListToVector( globalDimensionsPy ), ListToVector( globalOffsetsPy ) );
        return *reinterpret_cast<VariablePy<T>*>( &var );
    }


    MethodPy& DeclareMethodPy( const std::string methodName, const std::string type = "" );

    EnginePy OpenPy( const std::string name, const std::string accessMode,
    		         const MethodPy&  method, pyObject py_comm = pyObject() );

};





} //end namespace


#endif /* ADIOSPY_H_ */
