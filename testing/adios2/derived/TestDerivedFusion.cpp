/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cmath>
#include <cstring>
#include <map>
#include <numeric>
#include <vector>

#include <gtest/gtest.h>

#include "adios2/toolkit/derived/DerivedData.h"
#include "adios2/toolkit/derived/ExprCodeStream.h"
#include "adios2/toolkit/derived/ExprNode.h"

using namespace adios2;
using namespace adios2::derived;
using namespace adios2::detail;

// Reference values must not be FMA-contracted: the fused JIT loop emits
// exactly the multiplies and adds the expression wrote (bit-identical to the
// interpreter), while clang would fuse the C references below by default.
#pragma STDC FP_CONTRACT OFF

// Helper: build a code stream from an expression string with float variables
static ExprCodeStream BuildCS(const std::string &expr, const std::vector<std::string> &varNames)
{
    auto tree = ParseToExprNode(expr);
    std::map<std::string, DataType> varTypes;
    for (auto &v : varNames)
        varTypes[v] = DataType::Float;
    ResolveTreeTypes(tree, varTypes);
    auto cs = GenerateCode(tree);
    SemanticsPass(cs, varTypes);
    PlanBuffers(cs);
    TryFuse(cs);
    cs.ExprString = expr;
    return cs;
}

// Helper: execute a code stream on float arrays, return float result
static std::vector<float> RunFloat(const ExprCodeStream &cs,
                                   std::map<std::string, std::vector<float>> &vars, size_t N)
{
    std::map<std::string, std::vector<DerivedData>> inputData;
    Dims start = {0}, count = {N};
    for (auto &kv : vars)
    {
        inputData[kv.first] = {{(void *)kv.second.data(), start, count, DataType::Float}};
    }
    auto result = Execute(cs, 1, inputData);
    std::vector<float> out(N);
    memcpy(out.data(), result[0].Data, N * sizeof(float));
    free(result[0].Data);
    return out;
}

class DerivedFusionTest : public ::testing::Test
{
protected:
    static const size_t N = 1000;
    std::vector<float> A, B, C;
    std::map<std::string, std::vector<float>> vars;

    void SetUp() override
    {
        A.resize(N);
        B.resize(N);
        C.resize(N);
        for (size_t i = 0; i < N; i++)
        {
            A[i] = (float)(i + 1) * 0.1f;
            B[i] = (float)(i + 1) * 0.2f;
            C[i] = (float)(i + 1) * 0.3f;
        }
        vars["A"] = A;
        vars["B"] = B;
        vars["C"] = C;
    }
};

TEST_F(DerivedFusionTest, FusionEnabled)
{
    auto cs = BuildCS("A + B", {"A", "B"});
    EXPECT_NE(cs.Fused, nullptr) << "Fusion should succeed for A + B";
}

TEST_F(DerivedFusionTest, Add)
{
    auto cs = BuildCS("A + B", {"A", "B"});
    auto result = RunFloat(cs, vars, N);
    for (size_t i = 0; i < N; i++)
        EXPECT_NEAR(result[i], A[i] + B[i], 1e-5f);
}

TEST_F(DerivedFusionTest, Subtract)
{
    auto cs = BuildCS("A - B", {"A", "B"});
    auto result = RunFloat(cs, vars, N);
    for (size_t i = 0; i < N; i++)
        EXPECT_NEAR(result[i], A[i] - B[i], 1e-5f);
}

TEST_F(DerivedFusionTest, Multiply)
{
    auto cs = BuildCS("A * B", {"A", "B"});
    auto result = RunFloat(cs, vars, N);
    for (size_t i = 0; i < N; i++)
        EXPECT_NEAR(result[i], A[i] * B[i], 1e-5f);
}

TEST_F(DerivedFusionTest, Divide)
{
    auto cs = BuildCS("A / B", {"A", "B"});
    auto result = RunFloat(cs, vars, N);
    for (size_t i = 0; i < N; i++)
        EXPECT_NEAR(result[i], A[i] / B[i], 1e-5f);
}

TEST_F(DerivedFusionTest, Negate)
{
    auto cs = BuildCS("x = A\n -x", {"A"});
    auto result = RunFloat(cs, vars, N);
    for (size_t i = 0; i < N; i++)
        EXPECT_NEAR(result[i], -A[i], 1e-5f);
}

TEST_F(DerivedFusionTest, PowerSquare)
{
    auto cs = BuildCS("A ^ 2", {"A"});
    auto result = RunFloat(cs, vars, N);
    for (size_t i = 0; i < N; i++)
        EXPECT_NEAR(result[i], A[i] * A[i], 1e-4f);
}

TEST_F(DerivedFusionTest, PowerCube)
{
    auto cs = BuildCS("A ^ 3", {"A"});
    auto result = RunFloat(cs, vars, N);
    for (size_t i = 0; i < N; i++)
        EXPECT_NEAR(result[i], std::pow(A[i], 3.0f), 1e-3f);
}

TEST_F(DerivedFusionTest, Sqrt)
{
    auto cs = BuildCS("sqrt(A)", {"A"});
    auto result = RunFloat(cs, vars, N);
    for (size_t i = 0; i < N; i++)
        EXPECT_NEAR(result[i], std::sqrt(A[i]), 1e-5f);
}

TEST_F(DerivedFusionTest, Sin)
{
    auto cs = BuildCS("sin(A)", {"A"});
    auto result = RunFloat(cs, vars, N);
    for (size_t i = 0; i < N; i++)
        EXPECT_NEAR(result[i], std::sin(A[i]), 1e-5f);
}

TEST_F(DerivedFusionTest, Cos)
{
    auto cs = BuildCS("cos(A)", {"A"});
    auto result = RunFloat(cs, vars, N);
    for (size_t i = 0; i < N; i++)
        EXPECT_NEAR(result[i], std::cos(A[i]), 1e-5f);
}

TEST_F(DerivedFusionTest, Magnitude)
{
    auto cs = BuildCS("sqrt(A^2 + B^2 + C^2)", {"A", "B", "C"});
    auto result = RunFloat(cs, vars, N);
    for (size_t i = 0; i < N; i++)
    {
        float expected = std::sqrt(A[i] * A[i] + B[i] * B[i] + C[i] * C[i]);
        EXPECT_NEAR(result[i], expected, 1e-4f);
    }
}

// n-ary chains: GenerateCode flattens a-b-c and a/b/c into single 3-input
// instructions; the fused loop must left-fold ALL operands (a dropped third
// operand once passed every 2-operand test).
TEST_F(DerivedFusionTest, NArySubtract)
{
    auto cs = BuildCS("A - B - C", {"A", "B", "C"});
    auto result = RunFloat(cs, vars, N);
    for (size_t i = 0; i < N; i++)
    {
        float expected = A[i] - B[i] - C[i];
        EXPECT_NEAR(result[i], expected, 1e-4f);
    }
}

TEST_F(DerivedFusionTest, NAryDivide)
{
    auto cs = BuildCS("A / B / C", {"A", "B", "C"});
    auto result = RunFloat(cs, vars, N);
    for (size_t i = 0; i < N; i++)
    {
        float expected = A[i] / B[i] / C[i];
        EXPECT_NEAR(result[i], expected, 1e-4f);
    }
}

TEST_F(DerivedFusionTest, ComposedExpression)
{
    auto cs = BuildCS("(A + B) * C - A", {"A", "B", "C"});
    auto result = RunFloat(cs, vars, N);
    for (size_t i = 0; i < N; i++)
    {
        float expected = (A[i] + B[i]) * C[i] - A[i];
        EXPECT_NEAR(result[i], expected, 1e-4f);
    }
}

TEST_F(DerivedFusionTest, ConstantArithmetic)
{
    auto cs = BuildCS("A * 2.5 + 1.0", {"A"});
    auto result = RunFloat(cs, vars, N);
    for (size_t i = 0; i < N; i++)
        EXPECT_NEAR(result[i], A[i] * 2.5f + 1.0f, 1e-4f);
}

TEST_F(DerivedFusionTest, ConstantSubtractReverse)
{
    auto cs = BuildCS("3 - A", {"A"});
    auto result = RunFloat(cs, vars, N);
    for (size_t i = 0; i < N; i++)
        EXPECT_NEAR(result[i], 3.0f - A[i], 1e-5f);
}

TEST_F(DerivedFusionTest, ScientificNotation)
{
    auto cs = BuildCS("A * 1.5e2", {"A"});
    auto result = RunFloat(cs, vars, N);
    for (size_t i = 0; i < N; i++)
        EXPECT_NEAR(result[i], A[i] * 150.0f, 1e-2f);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
