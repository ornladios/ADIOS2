/*
 * Capsule.h
 *
 *  Created on: Dec 7, 2016
 *      Author: wfg
 */

#ifndef CAPSULE_H_
#define CAPSULE_H_

/**
 * Base class that owns an manages the raw data buffer and metadata.
 * Derived classes will allocate their own buffer in different memory spaces.
 * e.g. locally (stack) or in shared memory (virtual memory)
 */
class Capsule
{

public:

    const std::string m_Type; ///< buffer type
    const char m_AccessMode; ///< 'w': write, 'r': read, 'a': append

    /**
     * Base class constructor providing type from derived class and accessMode
     * @param type derived class type
     * @param accessMode 'w':write, 'r':read, 'a':append
     */
    Capsule( const std::string type, const char accessMode );

    virtual ~Capsule( );

    virtual void Write( const std::vector<char>& data, const std::size_t size );

};



#endif /* CAPSULE_H_ */
