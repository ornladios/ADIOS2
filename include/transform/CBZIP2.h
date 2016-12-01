/*
 * CBZIP2.h
 *
 *  Created on: Oct 17, 2016
 *      Author: wfg
 */

#ifndef CBZIP2_H_
#define CBZIP2_H_


#include "core/CTransform.h"


namespace adios
{


class CBZIP2 : public CTransform
{

public:

    /**
     * Initialize parent method
     * @param compressionLevel
     * @param variable
     */
    CBZIP2( );

    ~CBZIP2( );

    void Compress( const std::vector<char>& bufferIn, std::vector<char>& bufferOut );

    void Decompress( const std::vector<char>& bufferIn, std::vector<char>& bufferOut );
};


} //end namespace



#endif /* CBZIP2_H_ */
