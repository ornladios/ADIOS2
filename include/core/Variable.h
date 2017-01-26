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
#include <memory>
/// \endcond

#include "core/Transform.h"


namespace adios
{
/**
 * @param Base (parent) class for template derived (child) class CVariable. Required to put CVariable objects in STL containers.
 */
template< class T >
struct Variable
{
    std::string DimensionsCSV; ///< comma separated list for variables to search for local dimensions
    const T* Values; ///< pointer to values passed from user in ADIOS Write
    int GlobalBoundsIndex; ///< if global > 0, index corresponds to global-bounds in m_GlobalBounds in CGroup, if local then = -1

    std::vector< Transform* > Transforms; ///< associated transforms, sequence determines application order, e.g. first Transforms[0] then Transforms[1]. Pointer used as reference (no memory management).
    std::vector< int > Parameters; ///< additional optional parameter understood by the corresponding Transform in Transforms vector
    //Group& Group; ///< reference to group
    //bool IsDimension = false; ///< true: is used as a dimension in another variable (typically scalars), false: none
};


} //end namespace



#endif /* VARIABLE_H_ */
