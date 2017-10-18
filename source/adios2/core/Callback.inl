/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Callback.inl
 *
 *  Created on: Oct 18, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_CALLBACK_INL_
#define ADIOS2_CORE_CALLBACK_INL_
#ifndef ADIOS2_CORE_CALLBACK_H_
#error "Inline file should only be included from it's header, never on it's own"
#endif

namespace adios2
{

template <class R, class... Args>
Callback<R, Args...>::Callback(std::function<R(Args...)> function,
                               const Params &parameters, const bool debugMode)
: Operator("Callback", parameters, debugMode), m_Function(function)
{
}

} // end namespace adios2

#endif /* ADIOS2_CORE_CALLBACK_INL_ */
