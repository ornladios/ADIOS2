/*
 * CZLIB.h
 *
 *  Created on: Oct 17, 2016
 *      Author: wfg
 */

#ifndef CZLIB_H_
#define CZLIB_H_


#include <zlib.h>

#include "../core/CTransform.h"


namespace adios
{


class CZLIB : public CTransform
{

public:

    /**
     * Initialize parent method
     * @param variable
     */
    CZLIB( const unsigned int compressionLevel, CVariable& variable );

    ~CZLIB( );

    void WriteTransform( );

    void ReadTransform( );

    void GetCompressedLength( ) const;

    void GetExpandedLength( ) const;

};


} //end namespace



#endif /* CZLIB_H_ */
