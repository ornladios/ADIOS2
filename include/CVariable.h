/*
 * CVariable.h
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */

#ifndef CVARIABLE_H_
#define CVARIABLE_H_

#include <string>
#include <vector>
#include <typeinfo> // for typeid


namespace adios
{

class CVariable
{

public:

    CVariable( const std::string type, const std::string dimensionsCSV = "1" ):
        Type( type ),
        DimensionsCSV( dimensionsCSV )
    { }

    virtual ~CVariable()
    { }

    template<class T> const T& Get( ) const;
    template<class T, class U> void Set(const U& rhs);


//protected:

    std::string Type = "NONE"; ///< mandatory, double, float, unsigned integer, integer, etc.
    std::string DimensionsCSV = "1"; ///< single string containing comma separated value (CSV) for the variable dimensions, from XML config file
    std::vector<unsigned int> Dimensions = {1}; ///< if empty variable is a scalar, else N-dimensional variable
    //To do/understand gwrite, gread, read
};


template<class T>
class CVariableTemplate : public CVariable
{

public:

    CVariableTemplate( const std::string dimensionsCSV = "1" ):
        CVariable( typeid(T).name(), dimensionsCSV )
    { }

    ~CVariableTemplate( )
    { }

    const T& Get() const { return Value; }
    T Value;

};


} //end namespace



#endif /* CVARIABLE_H_ */
