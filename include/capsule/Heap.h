/*
 * Heap.h
 *
 *  Created on: Dec 19, 2016
 *      Author: wfg
 */

#ifndef HEAP_H_
#define HEAP_H_


#include "core/Capsule.h"


namespace adios
{

/**
 * Buffer and Metadata are allocated in the Heap
 */
class Heap : public Capsule
{

public:

    std::vector<char> m_Buffer; ///< buffer allocated using the STL
    std::vector<char> m_Metadata; ///< metadata buffer allocated using the STL

    Heap( const std::string accessMode );

    ~Heap( );

};

} //end namespace






#endif /* HEAP_H_ */
