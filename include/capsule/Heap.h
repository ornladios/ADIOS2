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

    void Write( const Variable<char>& variable, const std::vector<unsigned long long int>& localDimensions,
                const std::vector<unsigned long long int>& globalDimensions,
                const std::vector<unsigned long long int>& globalOffsets );

    void Write( const Variable<unsigned char>& variable, const std::vector<unsigned long long int>& localDimensions,
                const std::vector<unsigned long long int>& globalDimensions,
                const std::vector<unsigned long long int>& globalOffsets );

    void Write( const Variable<short>& variable, const std::vector<unsigned long long int>& localDimensions,
                const std::vector<unsigned long long int>& globalDimensions,
                const std::vector<unsigned long long int>& globalOffsets );

    void Write( const Variable<unsigned short>& variable, const std::vector<unsigned long long int>& localDimensions,
                const std::vector<unsigned long long int>& globalDimensions,
                const std::vector<unsigned long long int>& globalOffsets );

    void Write( const Variable<int>& variable, const std::vector<unsigned long long int>& localDimensions,
                const std::vector<unsigned long long int>& globalDimensions,
                const std::vector<unsigned long long int>& globalOffsets );

    void Write( const Variable<unsigned int>& variable, const std::vector<unsigned long long int>& localDimensions,
                const std::vector<unsigned long long int>& globalDimensions,
                const std::vector<unsigned long long int>& globalOffsets );

    void Write( const Variable<long int>& variable, const std::vector<unsigned long long int>& localDimensions,
                const std::vector<unsigned long long int>& globalDimensions,
                const std::vector<unsigned long long int>& globalOffsets );

    void Write( const Variable<unsigned long int>& variable, const std::vector<unsigned long long int>& localDimensions,
                const std::vector<unsigned long long int>& globalDimensions,
                const std::vector<unsigned long long int>& globalOffsets );

    void Write( const Variable<long long int>& variable, const std::vector<unsigned long long int>& localDimensions,
                const std::vector<unsigned long long int>& globalDimensions,
                const std::vector<unsigned long long int>& globalOffsets );

    void Write( const Variable<unsigned long long int>& variable, const std::vector<unsigned long long int>& localDimensions,
                const std::vector<unsigned long long int>& globalDimensions,
                const std::vector<unsigned long long int>& globalOffsets );

    void Write( const Variable<float>& variable, const std::vector<unsigned long long int>& localDimensions,
                const std::vector<unsigned long long int>& globalDimensions,
                const std::vector<unsigned long long int>& globalOffsets );

    void Write( const Variable<double>& variable, const std::vector<unsigned long long int>& localDimensions,
                const std::vector<unsigned long long int>& globalDimensions,
                const std::vector<unsigned long long int>& globalOffsets );

    void Write( const Variable<long double>& variable, const std::vector<unsigned long long int>& localDimensions,
                const std::vector<unsigned long long int>& globalDimensions,
                const std::vector<unsigned long long int>& globalOffsets );


};

} //end namespace






#endif /* HEAP_H_ */
