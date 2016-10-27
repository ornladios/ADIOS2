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

    CVariableBase( const bool isGlobal, const std::string type, const std::string dimensionsCSV = "1", const std::string transform = "" );

    virtual ~CVariableBase( );

    template<class T> const T* Get( ) const;
    template<class T> void Set( const void* values );

//protected: turned off for testing
    bool m_IsGlobal = false;
    std::string m_Type = "NONE"; ///< mandatory, double, float, unsigned integer, integer, etc.
    std::vector<std::string> m_Dimensions = {"1"}; ///< if empty variable is a scalar, else N-dimensional variable
    std::string m_Transform; ///< data transform: zlib, bzip2, szip
    unsigned int m_CompressionLevel = 0; ///< taken from m_Transform, 1 to 9, if zero the default library value is taken
    //To do/understand gwrite, gread, read
};


} //end namespace



#endif /* CVARIABLEBASE_H_ */
