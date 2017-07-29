/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Transform.cpp
 *
 *  Created on: Dec 5, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "Transform.h"
#include "Transform.tcc"

namespace adios2
{

Transform::Transform(const std::string library, const bool debugMode)
: m_Library(library), m_DebugMode(debugMode)
{
}

size_t Transform::BufferMaxSize(const size_t sizeIn) const
{
    if (m_DebugMode)
    {
        throw std::invalid_argument(
            "ERROR: signature (const size_t) not supported "
            "by derived class implemented with " +
            m_Library + ", in call to BufferMaxSize\n");
    }
    return 0;
}

size_t Transform::Compress(const void * /*dataIn*/, const Dims & /*dimensions*/,
                           const size_t /*elementSize*/,
                           const std::string /*type*/, void * /*bufferOut*/,
                           const Params & /*params*/) const
{
    if (m_DebugMode)
    {
        throw std::invalid_argument("ERROR: signature (const void*, const "
                                    "Dims, const size_t, const std::string, "
                                    "void*, const Params&) not supported "
                                    "by derived class implemented with " +
                                    m_Library + ", in call to Compress\n");
    }
    return 0;
}

size_t Transform::Decompress(const void *bufferIn, const size_t sizeIn,
                             void *dataOut, const size_t sizeOut) const
{
    if (m_DebugMode)
    {
        throw std::invalid_argument(
            "ERROR: signature (const void*, const size_t, void) not supported "
            "by derived class implemented with " +
            m_Library + ", in call to Decompress\n");
    }

    return 0;
}

size_t Transform::Decompress(const void * /*bufferIn*/, const size_t /*sizeIn*/,
                             void * /*dataOut*/, const Dims & /*dimensions*/,
                             const std::string /*type*/,
                             const Params & /*parameters*/) const
{
    if (m_DebugMode)
    {
        throw std::invalid_argument("ERROR: signature (const void*, const "
                                    "size_t, void*, const Dims&, const "
                                    "std::string ) not supported "
                                    "by derived class implemented with " +
                                    m_Library + ", in call to Decompress\n");
    }

    return 0;
}

// PROTECTED
size_t Transform::DoBufferMaxSize(const void *dataIn, const Dims &dimensions,
                                  const std::string type,
                                  const Params &parameters) const
{
    if (m_DebugMode)
    {
        throw std::invalid_argument(
            "ERROR: signature (const void*, const Dims& "
            "std::string ) not supported "
            "by derived class implemented with " +
            m_Library + ", in call to BufferMaxSize\n");
    }

    return 0;
}

} // end namespace adios2
