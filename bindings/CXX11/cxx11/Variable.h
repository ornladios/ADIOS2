/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Variable.h :
 *
 *  Created on: Jun 4, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_CXX11_CXX11_VARIABLE_H_
#define ADIOS2_BINDINGS_CXX11_CXX11_VARIABLE_H_

#include "Operator.h"

#include "adios2/ADIOSTypes.h"

namespace adios2
{

// forward declare
class IO;     // friend
class Engine; // friend

namespace core
{

template <class T>
class Variable; // private implementation
}

template <class T>
class Variable
{

    friend class IO;
    friend class Engine;

public:
    Variable<T>() = default;
    ~Variable<T>() = default;

    explicit operator bool() const noexcept;

    void SetSelection(const Box<Dims> &selection);
    void SetStepSelection(const Box<size_t> &stepSelection);
    size_t SelectionSize() const;
    const std::string &Name() const;
    std::string Type() const;
    size_t Sizeof() const;
    adios2::ShapeID ShapeID() const;
    const Dims &Shape() const;
    const Dims &Start() const;
    const Dims &Count() const;
    size_t Steps() const;
    size_t StepsStart() const;

    unsigned int AddOperator(Operator &op, const Params &parameters = Params());

    struct OperatorInfo
    {
        Operator ADIOSOperator;
        Params Parameters;
        Params Info;
    };

    std::vector<OperatorInfo> OperatorsInfo() const noexcept;

private:
    Variable<T>(core::Variable<T> *variable);
    core::Variable<T> *m_Variable = nullptr;
};

} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX11_CXX11_VARIABLE_H_ */
