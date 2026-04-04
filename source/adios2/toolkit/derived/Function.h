/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_DERIVED_Function_H_
#define ADIOS2_DERIVED_Function_H_

#include "DerivedData.h"

namespace adios2
{
namespace derived
{
DerivedData AddFunc(const ExprData &input);
DerivedData SubtractFunc(const ExprData &input);
DerivedData NegateFunc(const ExprData &input);
DerivedData SinFunc(const ExprData &input);
DerivedData CosFunc(const ExprData &input);
DerivedData TanFunc(const ExprData &input);
DerivedData AsinFunc(const ExprData &input);
DerivedData AcosFunc(const ExprData &input);
DerivedData AtanFunc(const ExprData &input);
DerivedData MultFunc(const ExprData &input);
DerivedData DivFunc(const ExprData &input);
DerivedData SqrtFunc(const ExprData &input);
DerivedData PowFunc(const ExprData &input);
DerivedData MagnitudeFunc(const ExprData &input);
DerivedData Cross3DFunc(const ExprData &input);
DerivedData Curl3DFunc(const ExprData &input);

std::tuple<Dims, Dims, Dims> SameDimsFunc(std::vector<std::tuple<Dims, Dims, Dims>> input);
std::tuple<Dims, Dims, Dims> SameDimsWithAgrFunc(std::vector<std::tuple<Dims, Dims, Dims>> input);
std::tuple<Dims, Dims, Dims> Cross3DDimsFunc(std::vector<std::tuple<Dims, Dims, Dims>> input);
std::tuple<Dims, Dims, Dims> CurlDimsFunc(std::vector<std::tuple<Dims, Dims, Dims>> input);

/** Promote (type-convert) an array element by element. */
void PromoteArray(DataType outType, DataType inType, void *out, void *in, size_t N);

}
}
#endif
