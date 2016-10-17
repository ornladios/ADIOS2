/*
 * CZLIB.h
 *
 *  Created on: Oct 17, 2016
 *      Author: wfg
 */

#ifndef CZLIB_H_
#define CZLIB_H_


#include <zlib.h>

#include "CTransform.h"


namespace adios
{


class CZLIB : public CTransform
{

public:

    const std::string m_Method; ///< name of the transformation method
    const unsigned int m_CompressionLevel; ///< depends on library implementation
    CVariable& m_Variable; ///< variable to be transformed

    /**
     * Initialize parent method
     * @param variable
     */
    CZLIB( CVariable& variable );

    ~CZLIB( );

    void WriteTransform( );

    void ReadTransform( );

    void GetCompressedLength( ) const;

    void GetExpandedLength( ) const;

};


} //end namespace



#endif /* CZLIB_H_ */
