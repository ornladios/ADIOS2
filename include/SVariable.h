/*
 * SVariable.h
 *
 *  Created on: Oct 5, 2016
 *      Author: wfg
 */

#ifndef SVARIABLE_H_
#define SVARIABLE_H_

#include <string>
#include <vector>


namespace adios
{

/**
 * @brief Define variable attributes, used as map value in SGroup (map key is variable name)
 */
struct SVariable
{
    const std::string Type; ///< mandatory, double, float, unsigned integer, integer, etc.
    const std::vector<unsigned int> Dimensions; ///< if empty variable is a scalar, else N-dimensional variable

    //To do/understand gwrite, gread, read
};



} //end namespace



#endif /* SVARIABLE_H_ */
