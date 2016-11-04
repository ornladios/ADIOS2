/*
 * CVariable.h
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */

#ifndef CVARIABLE_H_
#define CVARIABLE_H_

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
class CVariable
{

public:

    /**
     * Unique constructor for local and global variables
     * @param type variable type, must be in SSupport::Datatypes[hostLanguage] in public/SSupport.h
     * @param dimensionsCSV comma separated variable local dimensions (e.g. "Nx,Ny,Nz")
     * @param transformIndex
     * @param globalIndex
     */
    CVariable( const std::string type, const std::string dimensionsCSV,
               const int globalIndex, const int transformIndex );

    virtual ~CVariable( );

    void* m_Values;


protected:

    const std::string m_Type; ///< mandatory, double, float, unsigned integer, integer, etc.
    const std::string m_DimensionsCSV; ///< comma separated list for variables to search for local dimensions
};


} //end namespace



#endif /* CVARIABLE_H_ */
