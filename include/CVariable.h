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

    const std::string Type = "NONE"; ///< mandatory, double, float, unsigned integer, integer, etc.

    CVariable( const std::string type, const std::vector<unsigned int> dimensions ):
        Type( type ),
        Dimensions( std::move( dimensions ) )
    { }

    virtual ~CVariable()
    { }

    template<class T> const T& Get( ) const;
    template<class T, class U> void Set(const U& rhs);


protected:

    const std::vector<unsigned int> Dimensions = {1}; ///< if empty variable is a scalar, else N-dimensional variable
    //To do/understand gwrite, gread, read
};


template<class T>
struct CVariableTemplate : public CVariable
{
    CVariableTemplate( const std::vector<unsigned int> dimensions ):
        CVariable( typeid(T).name(), dimensions )
    { }

    const T& Get() const { return Value; }
    T Value;

};


} //end namespace



#endif /* CVARIABLE_H_ */
