/*
 * CVariableType.h
 *
 *  Created on: Oct 18, 2016
 *      Author: wfg
 */

#ifndef CVARIABLETEMPLATE_H_
#define CVARIABLETEMPLATE_H_


#include "core/CVariable.h"


namespace adios
{


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

    const T* m_Value = nullptr; // pointer or no pointer?

    const T* Get() const { return m_Value; }

    void Set( const void* values ){ m_Value = static_cast<const T*>( values ); }
};


template<class T> const T* CVariable::Get( ) const
{
    return dynamic_cast< const CVariableTemplate<T>&  >(*this).Get( );
}

template<class T> void CVariable::Set( const void* values )
{
    return dynamic_cast< CVariableTemplate<T>& >( *this ).Set( values );
}


} //end namespace

#endif /* CVARIABLETEMPLATE_H_ */
