/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Operator.inl
 *
 *  Created on: Oct 18, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_OPERATOR_INL_
#define ADIOS2_CORE_OPERATOR_INL_
#ifndef ADIOS2_CORE_OPERATOR_H_
#error "Inline file should only be included from it's header, never on it's own"
#endif

#include "adios2/core/Callback.h"

namespace adios2
{

template <class R, class... Args>
std::function<R(Args...)> Operator::GetCallback()
{
    if (m_DebugMode && m_Type != "Callback")
    {
        throw std::invalid_argument(
            "ERROR: operator of type " + m_Type +
            " doesn't support GetCallback, it's "
            " only allowed for Callback Operator types\n");
    }

    Callback<R, Args...> *callback = dynamic_cast<Callback<R, Args...> *>(this);
    return callback->m_Function;
}

} // end namespace adios2

#endif /* ADIOS2_CORE_OPERATOR_INL_ */
