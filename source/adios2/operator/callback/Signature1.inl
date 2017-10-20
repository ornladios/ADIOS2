/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Callback1.inl
 *
 *  Created on: Oct 19, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_OPERATOR_CALLBACK_SIGNATURE1_INL_
#define ADIOS2_OPERATOR_CALLBACK_SIGNATURE1_INL_
#ifndef ADIOS2_OPERATOR_CALLBACK_SIGNATURE1_H_
#error "Inline file should only be included from it's header, never on it's own"
#endif

namespace adios2
{
namespace callback
{

template <class T>
Signature1<T>::Signature1(
    const std::function<void(const T *, const std::string &,
                             const std::string &, const std::string &,
                             const Dims &)> &function,
    const Params &parameters, const bool debugMode)
: Operator("Signature1", parameters, debugMode), m_Function(function)
{
}

template <class T>
void Signature1<T>::RunCallback1(const T *arg1, const std::string &arg2,
                                 const std::string &arg3,
                                 const std::string &arg4, const Dims &arg5)
{
    if (m_Function)
    {
        m_Function(arg1, arg2, arg3, arg4, arg5);
    }
    else
    {
        throw std::runtime_error(
            "ERROR: callback function of Signature1<T> type failed\n");
    }
}

} // end namespace callback
} // end namespace adios2

#endif /* ADIOS2_OPERATOR_CALLBACK_CALLBACK1_INL_ */
