/*
 * SGroup.h
 *
 *  Created on: Oct 5, 2016
 *      Author: wfg
 */

#ifndef SGROUP_H_
#define SGROUP_H_

#include <map>
#include <string>

#include "SVariable.h"

namespace adios
{

/**
 * @brief Define group, used as map value in ADIOS class (map key is group name)
 */
struct SGroup
{
    /**
     * @brief Contains all group variables (from XML Config file).
     * <pre>
     * \t Key: std::string unique variable name
     * \t Value: SVariable struct defined in SVariable.h
     * </pre>
     */
    std::map< std::string, SVariable > Variables;

    std::vector<SAttribute> Attributes; ///< Contains all group attributes
    std::vector< unsigned long int > GlobalDimensions; ///< from global-bounds in XML File, data in global space
    std::vector< unsigned long int > GlobalOffsets; ///< from global-bounds in XML File, data in global space



};


} //end namespace


#endif /* SGROUP_H_ */
