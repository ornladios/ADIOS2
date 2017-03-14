/*
 * MethodPy.h
 *
 *  Created on: Mar 14, 2017
 *      Author: wfg
 */

#ifndef METHODPY_H_
#define METHODPY_H_

#include <boost/python.hpp>

#include "core/Method.h"

namespace adios
{

using pyList = boost::python::list;

class MethodPy : public Method
{

public:

    MethodPy( const std::string type, const bool debugMode );

    ~MethodPy( );

    /**
     * static needed to support raw function
     * @param dictionary
     * @return
     */
    static boost::python::object SetParametersPy( boost::python::tuple args, boost::python::dict kwargs );

    static boost::python::object AddTransportPy( boost::python::tuple args, boost::python::dict kwargs );

    void PrintAll( ) const;

};







}


#endif /* METHODPY_H_ */
