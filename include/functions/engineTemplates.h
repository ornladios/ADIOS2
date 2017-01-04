/*
 * GroupTemplates.h
 *
 *  Created on: Nov 7, 2016
 *      Author: wfg
 */

#ifndef ENGINETEMPLATES_H_
#define ENGINETEMPLATES_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
#include <stdexcept>
/// \endcond


#include "core/Group.h"
#include "core/Variable.h"
#include "core/Capsule.h"
#include "functions/adiosFunctions.h"



namespace adios
{


/**
 * Intermediate function that assigns values from user to a Variable.Values pointer reference
 * and dispatches it to different capsules.
 * A variable's numerical dimensions are obtained from previously set variables in group.
 * @param group variable owner, used to get all dimensions as numerical values
 * @param variable variable to be written
 * @param capsules container of capsules coming from an Engine
 * @param transports used only if buffer size is larger than a certain maximum
 */
template<class T>
void WriteToCapsules( const Group& group, Variable<T>& variable, const T* values,
                      std::vector< std::shared_ptr<Capsule> >& capsules,
                      std::vector< std::shared_ptr<Transport> >& transports )
{
    variable.Values = values;
    auto localDimensions = group.GetDimensions( variable.DimensionsCSV );

    std::vector< unsigned long long int > globalDimensions;
    std::vector< unsigned long long int > globalOffsets;

    if( variable.GlobalBoundsIndex > -1 ) //global variable
    {
        globalDimensions = group.GetDimensions( group.m_GlobalBounds[ variable.GlobalBoundsIndex ].first );
        globalOffsets = group.GetDimensions( group.m_GlobalBounds[ variable.GlobalBoundsIndex ].second );
    }

    for( auto& capsule : capsules )
    {
        capsule->Write( variable, localDimensions, globalDimensions, globalOffsets, transports );
    }
}


} //end namespace



#endif /* ENGINETEMPLATES_H_ */
