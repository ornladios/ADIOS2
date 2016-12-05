/*
 * Transform.h
 *
 *  Created on: Oct 17, 2016
 *      Author: wfg
 */

#ifndef TRANSFORM_H_
#define TRANSFORM_H_


/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
/// \endcond

#include "core/Variable.h"


namespace adios
{

/**
 * Parent class that defines data variable transformations. Used as a member of CVariable
 */
class Transform
{

public:


    const std::string m_Method;

    /**
     * Initialize parent method
     * @param method zlib, bzip2, szip
     * @param variable
     */
    Transform( const std::string method );

    virtual ~Transform( );

    virtual void Compress( const std::vector<char>& bufferIn, std::vector<char>& bufferOut ) = 0;

    virtual void Decompress( const std::vector<char>& bufferIn, std::vector<char>& bufferOut ) = 0;

};


} //end namespace
#endif /* TRANSFORM_H_ */
