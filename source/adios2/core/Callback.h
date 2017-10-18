/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CallBack.h
 *
 *  Created on: Oct 18, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_CALLBACK_H_
#define ADIOS2_CORE_CALLBACK_H_

#include <functional>

#include "adios2/core/Operator.h"

namespace adios2
{

template <class R, class... Args>
class Callback : public Operator
{

public:
    std::function<R(Args...)> m_Function;

    /**
     * Unique constructor
     * @param debugMode
     */
    Callback<R, Args...>(std::function<R(Args...)> function,
                         const Params &parameters, const bool debugMode);

    ~Callback<R, Args...>() = default;
};

} // end namespace adios2

#include "Callback.inl"

#endif /* ADIOS2_OPERATOR_CALLBACK_CALLBACK_H_ */
