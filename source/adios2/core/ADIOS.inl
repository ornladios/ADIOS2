/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS.inl
 *
 *  Created on: Oct 18, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_ADIOS_INL_
#define ADIOS2_CORE_ADIOS_INL_
#ifndef ADIOS2_CORE_ADIOS_H_
#error "Inline file should only be included from it's header, never on it's own"
#endif

#include <stdexcept>

namespace adios2
{

template <class R, class... Args>
Operator &ADIOS::DefineOperator(const std::string name,
                                const std::function<R(Args...)> &function,
                                const Params &parameters)
{
    if (m_DebugMode && m_Operators.count(name) == 1)
    {
        throw std::invalid_argument("ERROR: operator with name " + name +
                                    " previously defined, must be unique, in "
                                    "call to DefineOperatorCallback\n");
    }

    std::shared_ptr<Operator> callback = std::make_shared<Callback<R, Args...>>(
        function, parameters, m_DebugMode);

    auto itPair = m_Operators.emplace(name, std::move(callback));
    return *itPair.first->second;
}

} // end namespace adios2

#endif /* ADIOS2_CORE_ADIOS_INL_ */
