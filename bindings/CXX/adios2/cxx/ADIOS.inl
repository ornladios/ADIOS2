/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ADIOS2_BINDINGS_CXX_CXX_ADIOS_INL_
#define ADIOS2_BINDINGS_CXX_CXX_ADIOS_INL_
#ifndef ADIOS2_BINDINGS_CXX_CXX_ADIOS_H_
#error "Inline file should only be included from it's header, never on it's own"
#endif

#include <stdexcept>

namespace adios2
{

template <class R, class... Args>
Operator ADIOS::DefineOperator(const std::string name,
                               const std::function<R(Args...)> &function,
                               const Params &parameters)
{
    CheckPointer("for operator name " + name +
                 ", in call to ADIOS::DefineOperator");
    return Operator(DefineCallBack(name, function, parameters));
}

} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX_CXX_ADIOS_INL_ */
