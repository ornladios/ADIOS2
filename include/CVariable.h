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

class CVariable
{

public:

    CVariable( const bool isGlobal, const std::string type, const std::string dimensionsCSV = "1", const std::string transform = ""  ):
        m_IsGlobal( isGlobal ),
        m_Type( type ),
        m_Transform( transform )
    {
        if( dimensionsCSV == "1" )
        {
            m_Dimensions.push_back( "1" );
            return;
        }

        std::istringstream dimensionsCSVSS( dimensionsCSV );
        std::string dimension;
        while( std::getline( dimensionsCSVSS, dimension, ',' ) )
        {
            m_Dimensions.push_back( dimension );
        }
    }

    virtual ~CVariable()
    { }

    template<class T> const T& Get( ) const;
    template<class T, class U> void Set(const U& rhs);


//protected: turned off for testing
    bool m_IsGlobal = false;
    std::string m_Type = "NONE"; ///< mandatory, double, float, unsigned integer, integer, etc.
    std::vector<std::string> m_Dimensions = {"1"}; ///< if empty variable is a scalar, else N-dimensional variable
    std::string m_Transform; ///< data transform: zlib, bzip2, szip
    unsigned int m_CompressionLevel = 0; ///< taken from m_Transform, 1 to 9, if zero the default library value is taken
    //To do/understand gwrite, gread, read
};


template<class T>
class CVariableTemplate : public CVariable
{

public:

    /**
     * Template constructor class required for putting CVariable objects as value in a STL map container
     * @param isGlobal
     * @param type
     * @param dimensionsCSV
     * @param transform
     */
    CVariableTemplate( const bool isGlobal, const std::string type, const std::string dimensionsCSV = "1", const std::string transform = "" ):
        CVariable( isGlobal, type, dimensionsCSV, transform )
    { }

    ~CVariableTemplate( )
    { }

    T* m_Value; // pointer or no pointer?

    const T& Get() const { return m_Value; }


};


} //end namespace



#endif /* CVARIABLE_H_ */
