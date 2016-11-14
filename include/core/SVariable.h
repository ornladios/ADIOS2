/*
 * CVariable.h
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */

#ifndef SVARIABLE_H_
#define SVARIABLE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
#include <vector>
#include <typeinfo> // for typeid
#include <sstream>
/// \endcond

namespace adios
{
/**
 * @param Base (parent) class for template derived (child) class CVariable. Required to put CVariable objects in STL containers.
 */
template< class T >
struct SVariable
{
    const std::string m_DimensionsCSV; ///< comma separated list for variables to search for local dimensions
    const T* m_Values;
    const int m_TransformIndex; ///< if global > 0, index corresponds to , if local then = -1
    const int m_GlobalBoundsIndex; ///< if global > 0, index corresponds to global-bounds in m_GlobalBounds in CGroup, if local then = -1
};


} //end namespace



#endif /* SVARIABLE_H_ */
