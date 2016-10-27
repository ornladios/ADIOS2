/*
 * CTransform.h
 *
 *  Created on: Oct 17, 2016
 *      Author: wfg
 */

#ifndef CTRANSFORM_H_
#define CTRANSFORM_H_


/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
/// \endcond

#include "core/CVariableBase.h"


namespace adios
{

/**
 * Parent class that defines data variable transformations. Used as a member of CVariable
 */
class CTransform
{

public:

    const std::string m_Method; ///< name of the transformation method
    const unsigned int m_CompressionLevel; ///< depends on library implementation
    CVariableBase& m_Variable; ///< variable to be transformed

    /**
     * Initialize parent method
     * @param method zlib, bzip2, szip
     * @param variable
     */
    CTransform( const std::string method, const unsigned int compressionLevel, CVariableBase& variable ):
        m_Method( method ),
        m_CompressionLevel( compressionLevel ),
        m_Variable( variable )
    { }

    virtual ~CTransform( ){ };

    virtual void Compress( ) const = 0; ///< Compress m_Variable data m_Value

    virtual void Decompress( ) const = 0; ///< Ext m_Variable data m_Value

};


} //end namespace
#endif /* CTRANSFORM_H_ */
