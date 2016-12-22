/*
 * Capsule.h
 *
 *  Created on: Dec 7, 2016
 *      Author: wfgtemplates and pointers
 */

#ifndef CAPSULE_H_
#define CAPSULE_H_

#include "core/Group.h"
#include "core/Variable.h"


namespace adios
{

/**
 * Base class that owns an manages the raw data buffer and metadata.
 * Derived classes will allocate their own buffer in different memory spaces.
 * e.g. locally (stack) or in shared memory (virtual memory)
 */
class Capsule
{

public:

    const std::string m_Type; ///< buffer type
    const std::string m_AccessMode; ///< 'w': write, 'r': read, 'a': append
    const int m_RankMPI;

    /**
     * Base class constructor providing type from derived class and accessMode
     * @param type derived class type
     * @param accessMode 'w':write, 'r':read, 'a':append
     */
    Capsule( const std::string type, const std::string accessMode, const int rankMPI );

    virtual ~Capsule( );

    virtual void Write( const Variable<char>& variable, const std::vector<unsigned long long int>& localDimensions,
                        const std::vector<unsigned long long int>& globalDimensions,
                        const std::vector<unsigned long long int>& globalOffsets ) = 0;

    virtual void Write( const Variable<unsigned char>& variable, const std::vector<unsigned long long int>& localDimensions,
                        const std::vector<unsigned long long int>& globalDimensions,
                        const std::vector<unsigned long long int>& globalOffsets ) = 0;

    virtual void Write( const Variable<short>& variable, const std::vector<unsigned long long int>& localDimensions,
                        const std::vector<unsigned long long int>& globalDimensions,
                        const std::vector<unsigned long long int>& globalOffsets ) = 0;

    virtual void Write( const Variable<unsigned short>& variable, const std::vector<unsigned long long int>& localDimensions,
                        const std::vector<unsigned long long int>& globalDimensions,
                        const std::vector<unsigned long long int>& globalOffsets ) = 0;

    virtual void Write( const Variable<int>& variable, const std::vector<unsigned long long int>& localDimensions,
                        const std::vector<unsigned long long int>& globalDimensions,
                        const std::vector<unsigned long long int>& globalOffsets ) = 0;

    virtual void Write( const Variable<unsigned int>& variable, const std::vector<unsigned long long int>& localDimensions,
                        const std::vector<unsigned long long int>& globalDimensions,
                        const std::vector<unsigned long long int>& globalOffsets ) = 0;

    virtual void Write( const Variable<long int>& variable, const std::vector<unsigned long long int>& localDimensions,
                        const std::vector<unsigned long long int>& globalDimensions,
                        const std::vector<unsigned long long int>& globalOffsets ) = 0;

    virtual void Write( const Variable<unsigned long int>& variable, const std::vector<unsigned long long int>& localDimensions,
                        const std::vector<unsigned long long int>& globalDimensions,
                        const std::vector<unsigned long long int>& globalOffsets ) = 0;

    virtual void Write( const Variable<long long int>& variable, const std::vector<unsigned long long int>& localDimensions,
                        const std::vector<unsigned long long int>& globalDimensions,
                        const std::vector<unsigned long long int>& globalOffsets ) = 0;

    virtual void Write( const Variable<unsigned long long int>& variable, const std::vector<unsigned long long int>& localDimensions,
                        const std::vector<unsigned long long int>& globalDimensions,
                        const std::vector<unsigned long long int>& globalOffsets ) = 0;

    virtual void Write( const Variable<float>& variable, const std::vector<unsigned long long int>& localDimensions,
                        const std::vector<unsigned long long int>& globalDimensions,
                        const std::vector<unsigned long long int>& globalOffsets ) = 0;

    virtual void Write( const Variable<double>& variable, const std::vector<unsigned long long int>& localDimensions,
                        const std::vector<unsigned long long int>& globalDimensions,
                        const std::vector<unsigned long long int>& globalOffsets ) = 0;

    virtual void Write( const Variable<long double>& variable, const std::vector<unsigned long long int>& localDimensions,
                        const std::vector<unsigned long long int>& globalDimensions,
                        const std::vector<unsigned long long int>& globalOffsets ) = 0;
};


} //end namespace

#endif /* CAPSULE_H_ */
