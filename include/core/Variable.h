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

namespace adios
{
/**
 * @param Base (parent) class for template derived (child) class CVariable. Required to put CVariable objects in STL containers.
 */
template< class T >
struct Variable
{
    const std::string DimensionsCSV; ///< comma separated list for variables to search for local dimensions
    const T* Values; ///< pointer to values passed from ADIOS Write
    const short GlobalBoundsIndex; ///< if global > 0, index corresponds to global-bounds in m_GlobalBounds in CGroup, if local then = -1
    short TransformIndex = -1; ///< if no transformation then = -1, otherwise index to m_Transforms container in ADIOS
    unsigned short CompressionLevel = 0; ///< if 0, then use default, values from 1 to 9 indicate compression level
};


} //end namespace



#endif /* VARIABLE_H_ */
