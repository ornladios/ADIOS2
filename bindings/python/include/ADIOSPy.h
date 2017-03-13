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


namespace adios
{


class ADIOSPy : public ADIOS
{

public:

    ADIOSPy( MPI_Comm mpiComm, const bool debug );
    ~ADIOSPy( );

    void HelloMPI( ); ///< says hello from rank/size for testing

    std::string DefineVariableDouble( const std::string name,
                                      const boost::python::list localDimensionsPy = boost::python::list(),
                                      const boost::python::list globalDimensionsPy = boost::python::list(),
                                      const boost::python::list globalOffsetsPy = boost::python::list() );

    std::string DefineVariableFloat( const std::string name,
                                     const boost::python::list localDimensionsPy = boost::python::list(),
                                     const boost::python::list globalDimensionsPy = boost::python::list(),
                                     const boost::python::list globalOffsetsPy = boost::python::list() );

    void SetVariableLocalDimensions( const std::string name, const boost::python::list list );

    boost::python::list GetVariableLocalDimensions( const std::string name );


private:
    template< class T >
    std::string DefineVariablePy( const std::string name, const boost::python::list& localDimensionsPy,
                                  const boost::python::list& globalDimensionsPy, const boost::python::list& globalOffsetsPy )
    {
        DefineVariable<T>( name, ListToVector( localDimensionsPy ), ListToVector( globalDimensionsPy ), ListToVector( globalOffsetsPy ) );
        return name;
    }


};



} //end namespace


#endif /* ADIOSPY_H_ */
