/*
 * adiosPyFunctions.h
 *
 *  Created on: Mar 13, 2017
 *      Author: wfg
 */

#ifndef ADIOSPYFUNCTIONS_H_
#define ADIOSPYFUNCTIONS_H_

#include <vector>
#include <string>
#include <boost/python.hpp>


namespace adios
{

std::vector<std::size_t> ListToVector( const boost::python::list& list );

boost::python::list VectorToList( const std::vector<std::size_t>& list );

}



#endif /* ADIOSPYFUNCTIONS_H_ */
