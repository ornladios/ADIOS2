/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Operator.tcc :
 *
 *  Created on: Jun 7, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_CXX11_CXX11_OPERATOR_TCC_
#define ADIOS2_BINDINGS_CXX11_CXX11_OPERATOR_TCC_

#include "Operator.h"

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{

template <class T>
void Operator::RunCallback1(const T *arg0, const std::string &arg1,
                            const std::string &arg2, const std::string &arg3,
                            const Dims &arg4)
{
    helper::CheckForNullptr(m_Operator, "in call to Operator::RunCallback1");
    m_Operator->RunCallback1(arg0, arg1, arg2, arg3, arg4);
}

template <class T>
size_t Operator::BufferMaxSize(const T *dataIn, const Dims &dimensions,
                               const Params &parameters) const
{
    helper::CheckForNullptr(m_Operator, "in call to Operator::BufferMaxSize");
    return m_Operator->BufferMaxSize(dataIn, dimensions, parameters);
}

template <class T>
size_t Operator::Compress(const T *dataIn, const Dims &dimensions,
                          const size_t elementSize, const std::string type,
                          char *bufferOut, const Params &parameters) const
{
    helper::CheckForNullptr(m_Operator, "in call to Operator::Compress");
    return m_Operator->Compress(dataIn, dimensions, elementSize, type,
                                bufferOut, parameters);
}

template <class T>
size_t Operator::Decompress(const char *bufferIn, const size_t sizeIn,
                            T *dataOut, const size_t sizeOut) const
{
    helper::CheckForNullptr(m_Operator, "in call to Operator::Decompress(const "
                                        "char*, const size_t, T*, const "
                                        "size_t)");
    return m_Operator->Decompress(bufferIn, sizeIn, dataOut, sizeOut);
}

template <class T>
size_t Operator::Decompress(const char *bufferIn, const size_t sizeIn,
                            T *dataOut, const Dims &dimensions,
                            const std::string type,
                            const Params &parameters) const
{
    helper::CheckForNullptr(m_Operator, "in call to Operator::Decompress(const "
                                        "char*, const size_t, T*, const Dims, "
                                        "const std::string, const Params&)");
    return m_Operator->Decompress(bufferIn, sizeIn, dataOut, dimensions, type,
                                  parameters);
}

} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX11_CXX11_OPERATOR_TCC_ */
