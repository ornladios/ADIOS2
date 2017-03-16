/*
 * EnginePy.h
 *
 *  Created on: Mar 15, 2017
 *      Author: wgodoy
 */

#ifndef ENGINEPY_H_
#define ENGINEPY_H_

#include "boost/python.hpp"
#include "boost/python/numpy.hpp"

#include "core/Engine.h"
#include "VariablePy.h"

namespace adios
{

namespace np = boost::python::numpy;

class EnginePy
{

public:

	std::shared_ptr<Engine> m_Engine;

	void WritePy( VariablePy<double>& variable, const np::ndarray& array );

	void WritePy( VariablePy<float>& variable, const np::ndarray& array );

	void Close( );

	void GetType( ) const;

};


} //end namespace




#endif /* BINDINGS_PYTHON_INCLUDE_ENGINEPY_H_ */
