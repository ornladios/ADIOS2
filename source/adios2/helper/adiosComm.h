/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosComm.h : Communicate in a multi-process environment.
 */

#ifndef ADIOS2_HELPER_ADIOSCOMM_H_
#define ADIOS2_HELPER_ADIOSCOMM_H_

namespace adios2
{
namespace helper
{

/** @brief Encapsulation for communication in a multi-process environment.  */
class Comm
{
public:
    ~Comm();
};

} // end namespace helper
} // end namespace adios2

#include "adiosComm.inl"

#endif // ADIOS2_HELPER_ADIOSCOMM_H_
