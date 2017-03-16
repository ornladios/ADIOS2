/*
 * EnginePy.h
 *
 *  Created on: Mar 15, 2017
 *      Author: wgodoy
 */

#ifndef ENGINEPY_H_
#define ENGINEPY_H_

#ifdef HAVE_BOOSTPYTHON
#include "boost/python.hpp"
#include "boost/python/numpy.hpp"
#endif

#ifdef HAVE_PYBIND11
#include "pybind11/pybind11.h"
#include "pybind11/numpy.h"
#endif

#include "core/Engine.h"
#include "VariablePy.h"
#include "adiosPyFunctions.h"

namespace adios
{

#ifdef HAVE_BOOSTPYTHON
using pyArray = boost::python::numpy::ndarray;
#endif

#ifdef HAVE_PYBIND11
using pyArray = pybind11::array;
#endif


class EnginePy
{

public:

	std::shared_ptr<Engine> m_Engine;

	void WritePy( VariablePy<char>& variable, const pyArray& array );
	void WritePy( VariablePy<unsigned char>& variable, const pyArray& array );
	void WritePy( VariablePy<short>& variable, const pyArray& array );
	void WritePy( VariablePy<unsigned short>& variable, const pyArray& array );
	void WritePy( VariablePy<int>& variable, const pyArray& array );
	void WritePy( VariablePy<unsigned int>& variable, const pyArray& array );
	void WritePy( VariablePy<long int>& variable, const pyArray& array );
	void WritePy( VariablePy<unsigned long int>& variable, const pyArray& array );
	void WritePy( VariablePy<long long int>& variable, const pyArray& array );
	void WritePy( VariablePy<unsigned long long int>& variable, const pyArray& array );
	void WritePy( VariablePy<float>& variable, const pyArray& array );
	void WritePy( VariablePy<double>& variable, const pyArray& array );
	void WritePy( VariablePy<long double>& variable, const pyArray& array );
	void WritePy( VariablePy<std::complex<float>>& variable, const pyArray& array );
	void WritePy( VariablePy<std::complex<double>>& variable, const pyArray& array );
	void WritePy( VariablePy<std::complex<long double>>& variable, const pyArray& array );

	void Close( );

	void GetType( ) const;

};


} //end namespace




#endif /* ENGINEPY_H_ */
