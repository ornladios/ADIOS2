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

#include "core/SVariable.h"


namespace adios
{

/**
 * Parent class that defines data variable transformations. Used as a member of CVariable
 */
class CTransform
{

public:


    const std::string m_Method;

    /**
     * Initialize parent method
     * @param method zlib, bzip2, szip
     * @param variable
     */
    CTransform( const std::string method ):
        m_Method{ method }
    { }

    virtual ~CTransform( ){ };

    virtual void Compress( const std::vector<char>& bufferIn, std::vector<char>& bufferOut ) = 0;

    virtual void Decompress( const std::vector<char>& bufferIn, std::vector<char>& bufferOut ) = 0;

};


} //end namespace
#endif /* CTRANSFORM_H_ */
