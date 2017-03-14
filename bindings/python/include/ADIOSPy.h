/*
 * ADIOSPy.h
 *
 *  Created on: Mar 13, 2017
 *      Author: wfg
 */

#ifndef ADIOSPY_H_
#define ADIOSPY_H_

#include <string>

#include <boost/python.hpp>

#include "ADIOS.h"
#include "adiosPyFunctions.h" //ListToVector, VectorToList
#include "VariablePy.h"
#include "MethodPy.h"


namespace adios
{

using pyList = boost::python::list;


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




};





} //end namespace


#endif /* ADIOSPY_H_ */
