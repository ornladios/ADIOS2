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
#include <memory> //for std::unique_ptr
#include <vector>

#include "CVariable.h"
#include "SAttribute.h"
#include "CTransport.h"

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
     *     Key: std::string unique variable name
     *     Value: Children of SVariable struct defined in SVariable.h, using unique_ptr for polymorphism
     * </pre>
     */
    std::map< std::string, std::shared_ptr<CVariable> > Variables;

    std::vector< SAttribute > Attributes; ///< Contains all group attributes from SAttribute.h

    std::vector< unsigned long int > GlobalDimensions; ///< from global-bounds in XML File, data in global space

    std::vector< unsigned long int > GlobalOffsets; ///< from global-bounds in XML File, data in global space

    std::shared_ptr< CTransport > TransportMethod; ///< transport method defined in XML File, using shared pointer as SGroup can be uninitialized

};


} //end namespace


#endif /* SGROUP_H_ */
