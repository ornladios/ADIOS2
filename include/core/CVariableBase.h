/*
 * CVariable.h
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */

#ifndef CVARIABLEBASE_H_
#define CVARIABLEBASE_H_

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
class CVariableBase
{

public:

    CVariableBase( const std::string type, const std::string dimensionsCSV, const std::string transform );

    CVariableBase( const std::string type, const std::string dimensionsCSV, const std::string transform,
                   const std::string globalDimensionsCSV, const std::string globalOffsetsCSV );


    virtual ~CVariableBase( );

    template<class T> const T* Get( ) const;
    template<class T> void Set( const void* values );


protected:

    const std::string m_Type; ///< mandatory, double, float, unsigned integer, integer, etc.
    const std::string m_DimensionsCSV; ///< comma separated list for variables to search for local dimensions
    const std::string m_Transform; ///< data transform: zlib:1, bzip2:4, szip:9 which level of compression
    const std::string m_GlobalDimensionsCSV; ///< comma separated list for variables to search for global dimensions
    const std::string m_GlobalOffsetsCSV; ///< comma separated list for variables to search for global offsets
    const bool m_IsGlobal = false; ///< boolean flag that identifies is a variable is global or not

};


} //end namespace



#endif /* CVARIABLEBASE_H_ */
