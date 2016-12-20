/*
 * Variable.h
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */

#ifndef VARIABLE_H_
#define VARIABLE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
#include <vector>
#include <typeinfo> // for typeid
#include <sstream>
/// \endcond

#include "Transform.h"

namespace adios
{
/**
 * @param Base (parent) class for template derived (child) class CVariable. Required to put CVariable objects in STL containers.
 */
template< class T >
class Variable
{
    const std::string DimensionsCSV; ///< comma separated list for variables to search for local dimensions
    const T* Values; ///< pointer to values passed from ADIOS Write
    const unsigned short GlobalBoundsIndex; ///< if global > 0, index corresponds to global-bounds in m_GlobalBounds in CGroup, if local then = -1
    Transform* Transform = nullptr; ///< if no transformation then nullptr, otherwise pointer reference to a Transport object
    short Parameter = -1; ///< additional optional parameter understood by a Transform
};


} //end namespace



#endif /* VARIABLE_H_ */
