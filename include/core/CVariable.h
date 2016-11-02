/*
 * CVariableType.h
 *
 *  Created on: Oct 18, 2016
 *      Author: wfg
 */

#ifndef CVARIABLE_H_
#define CVARIABLE_H_


#include "core/CVariableBase.h"


namespace adios
{

/**
 * Derived template class of CVariableBase. Will hold a pointer of the right type to the void* passed from user app in ADIOS.
 */
template<class T>
class CVariable : public CVariableBase
{

public:

    /**
     * Template constructor class required for putting CVariable objects as value in a STL map container
     * @param isGlobal
     * @param type
     * @param dimensionsCSV
     * @param transform
     */
    CVariable( const std::string type, const std::string dimensionsCSV, const std::string transform ):
        CVariableBase( type, dimensionsCSV, transform )
    { }

    CVariable( const std::string type, const std::string dimensionsCSV, const std::string transform,
               const std::string globalDimensionsCSV, const std::string globalOffsetsCSV ):
        CVariableBase( type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV )
    { }

    ~CVariable( )
    { }

    const T* m_Value = nullptr; // pointer or no pointer?

    const T* Get() const { return m_Value; }

    void Set( const void* values ){ m_Value = static_cast<const T*>( values ); }
};


template<class T> const T* CVariableBase::Get( ) const
{
    return dynamic_cast< const CVariable<T>&  >(*this).Get( );
}

template<class T> void CVariableBase::Set( const void* values )
{
    return dynamic_cast< CVariable<T>& >( *this ).Set( values );
}


} //end namespace

#endif /* CVARIABLE_H_ */
