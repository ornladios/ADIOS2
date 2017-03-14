/*
 * adiosPyFunctions.h
 *
 *  Created on: Mar 13, 2017
 *      Author: wfg
 */

#ifndef ADIOSPYFUNCTIONS_H_
#define ADIOSPYFUNCTIONS_H_

#include <vector>
#include <map>
#include <string>
#include <boost/python.hpp>


namespace adios
{

using Dims = std::vector<std::size_t>;
/**
 * Transforms a boost python list to a Dims (std::vector<std::size_t>) object
 * @param list input boost python list from python program
 * @return Dims (std::vector<std::size_t>) object than can be passed to python
 */
Dims ListToVector( const boost::python::list& list );

std::map<std::string, std::string> DictToMap( const boost::python::dict& dictionary );

}



#endif /* ADIOSPYFUNCTIONS_H_ */
