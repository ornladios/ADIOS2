/*
 * SAttribute.h
 *
 *  Created on: Oct 5, 2016
 *      Author: wfg
 */

#ifndef SATTRIBUTE_H_
#define SATTRIBUTE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
/// \endcond

namespace adios
{

/**
 * Plain-old data struct that defines an attribute in an ADIOS group in CGroup.h
 */
struct SAttribute
{
    bool IsGlobal; ///< true: static, defined in XML Config file, false: dynamic, defined in non-XML API
    std::string Type; ///< string or numeric type
    std::string Value; ///< information about the attribute
};


} //end namespace



#endif /* SATTRIBUTE_H_ */
