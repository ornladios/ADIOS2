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
    const std::string Type; ///< string or numeric type
    const std::string Value; ///< information about the attribute
    const int GlobalIndex; ///< if -1, local, else corresponding index to m_GlobalBounds in CGroup
};


} //end namespace



#endif /* SATTRIBUTE_H_ */
