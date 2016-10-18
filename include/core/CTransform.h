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

#include "core/CVariable.h"


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
    CVariable& m_Variable; ///< variable to be transformed

    /**
     * Initialize parent method
     * @param method zlib, bzip2, szip
     * @param variable
     */
    CTransform( const std::string method, const unsigned int compressionLevel, CVariable& variable );

    virtual ~CTransform( );

    virtual void WriteTransform( ) = 0;

    virtual void ReadTransform( ) = 0;

    virtual void GetCompressedLength( ) const = 0;

    virtual void GetExpandedLength( ) const = 0;

};


} //end namespace
#endif /* CTRANSFORM_H_ */
