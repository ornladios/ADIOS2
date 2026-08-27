/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Expression grammar rules: '/' is always division in formulas; slashed ADIOS
// names live in binding right-hand sides or inline backticks; a numeric
// binding RHS defines a named constant; slicing syntax is gone.

#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "adios2/toolkit/derived/ExprCodeStream.h"
#include "adios2/toolkit/derived/ExprNode.h"

using namespace adios2;
using namespace adios2::derived;
using namespace adios2::detail;

static ExprCodeStream Build(const std::string &expr, const std::map<std::string, DataType> &types)
{
    auto tree = ParseToExprNode(expr);
    std::map<std::string, DataType> t = types;
    ResolveTreeTypes(tree, t);
    auto cs = GenerateCode(tree);
    SemanticsPass(cs, t);
    PlanBuffers(cs);
    return cs;
}

static std::vector<float> Run1(const ExprCodeStream &cs, const std::string &name,
                               std::vector<float> &in)
{
    std::map<std::string, std::vector<DerivedData>> data;
    Dims start = {0}, count = {in.size()};
    data[name] = {{(void *)in.data(), start, count, DataType::Float}};
    auto out = Execute(const_cast<ExprCodeStream &>(cs), 1, data);
    std::vector<float> r((float *)out[0].Data, (float *)out[0].Data + in.size());
    free(out[0].Data);
    return r;
}

TEST(ExprGrammar, UnspacedSlashIsDivision)
{
    auto cs = Build("a/2.0", {{"a", DataType::Float}});
    ASSERT_EQ(cs.InputVarNames.size(), 1u);
    EXPECT_EQ(cs.InputVarNames[0], "a");
    std::vector<float> in = {8, 10, -3};
    auto r = Run1(cs, "a", in);
    EXPECT_EQ(r[0], 4.0f);
    EXPECT_EQ(r[1], 5.0f);
    EXPECT_EQ(r[2], -1.5f);
}

TEST(ExprGrammar, UnspacedVariableDivision)
{
    auto cs = Build("a/a", {{"a", DataType::Float}});
    ASSERT_EQ(cs.InputVarNames.size(), 1u);
    std::vector<float> in = {8, -2, 5};
    auto r = Run1(cs, "a", in);
    for (auto v : r)
        EXPECT_EQ(v, 1.0f);
}

TEST(ExprGrammar, PathInBindingRHS)
{
    auto cs = Build("x = sim/data.Ux \n x + x", {{"sim/data.Ux", DataType::Float}});
    ASSERT_EQ(cs.InputVarNames.size(), 1u);
    EXPECT_EQ(cs.InputVarNames[0], "sim/data.Ux");
}

TEST(ExprGrammar, LeadingSlashPathInBinding)
{
    auto cs = Build("j = /bbb/ng \n j * 2.0", {{"/bbb/ng", DataType::Float}});
    ASSERT_EQ(cs.InputVarNames.size(), 1u);
    EXPECT_EQ(cs.InputVarNames[0], "/bbb/ng");
}

TEST(ExprGrammar, BacktickInlineName)
{
    auto cs = Build("`sim/Ux` + `sim/Ux`", {{"sim/Ux", DataType::Float}});
    ASSERT_EQ(cs.InputVarNames.size(), 1u);
    EXPECT_EQ(cs.InputVarNames[0], "sim/Ux");
}

TEST(ExprGrammar, NamedConstant)
{
    auto cs = Build("c = 2.5 \n a * c", {{"a", DataType::Float}});
    // c is a constant, not an input
    ASSERT_EQ(cs.InputVarNames.size(), 1u);
    EXPECT_EQ(cs.InputVarNames[0], "a");
    std::vector<float> in = {2, -4};
    auto r = Run1(cs, "a", in);
    EXPECT_EQ(r[0], 5.0f);
    EXPECT_EQ(r[1], -10.0f);
}

TEST(ExprGrammar, NamedConstantPrecision)
{
    // std::to_string would zero this; %.17g must not
    auto cs = Build("k = 1.0e-10 \n a + k", {{"a", DataType::Float}});
    std::vector<float> in = {0};
    auto r = Run1(cs, "a", in);
    EXPECT_NEAR(r[0], 1.0e-10f, 1e-16f);
}

TEST(ExprGrammar, EnergyExpressionReadableForm)
{
    // Ana's expression, written the way physics reads
    auto cs = Build("c = 2.99792458e8 \n ux = ux \n"
                    " sqrt(ux*ux*c^2 + (ux*c)*(ux*c))",
                    {{"ux", DataType::Double}});
    ASSERT_EQ(cs.InputVarNames.size(), 1u);
}

TEST(ExprGrammar, InlinePathIsError)
{
    // pre-change this parsed as one variable named "sim.Ux"; now '.' outside
    // numbers and binding RHS is an error
    EXPECT_THROW(ParseToExprNode("sim.Ux + 1"), std::exception);
}

TEST(ExprGrammar, SlicingSyntaxGone)
{
    EXPECT_THROW(ParseToExprNode("x = var \n x[1:5]"), std::exception);
}

// NaN test on the bit pattern: exponent all-ones with nonzero mantissa.
// (std::isnan is tautologically false under fast-math and icx warns on it.)
static bool IsNanBits(int32_t i) { return (i & 0x7fffffff) > 0x7f800000; }

// ULP distance between two floats: 0 = bit-identical (also for +0.0f vs -0.0f
// and NaN vs NaN of any payload). Opposite-signed values are measured through
// zero so the metric stays monotone across it.
static size_t UlpDistance(float a, float b)
{
    int32_t ia, ib;
    std::memcpy(&ia, &a, sizeof(float));
    std::memcpy(&ib, &b, sizeof(float));
    if (ia == ib)
        return 0;
    if (IsNanBits(ia) || IsNanBits(ib))
        return IsNanBits(ia) && IsNanBits(ib) ? 0 : SIZE_MAX;
    // Map the sign-magnitude float ordering onto a monotone integer line
    ia = (ia < 0) ? INT32_MIN - ia : ia;
    ib = (ib < 0) ? INT32_MIN - ib : ib;
    return static_cast<size_t>(std::abs(static_cast<int64_t>(ia) - static_cast<int64_t>(ib)));
}

// --- run an expression through BOTH evaluation paths and require results
// within maxUlp of each other; returns the interpreter's result.
// maxUlp defaults to 0 = bit-identical, the current fusion contract (no FMA
// contraction). If contraction is ever enabled, callers raise the tolerance
// rather than restructure the comparison. -------------------------------
static std::vector<float> RunBothWays(const std::string &expr,
                                      const std::map<std::string, DataType> &types,
                                      std::map<std::string, std::vector<DerivedData>> &data,
                                      size_t n, const std::vector<std::string> &scalarInputs = {},
                                      size_t maxUlp = 0)
{
    auto tree = ParseToExprNode(expr);
    std::map<std::string, DataType> t = types;
    ResolveTreeTypes(tree, t);

    auto lower = [&](bool fuse) {
        auto tcopy = tree;
        auto cs = GenerateCode(tcopy);
        SemanticsPass(cs, t);
        PlanBuffers(cs);
        MarkScalarInputs(cs, scalarInputs);
        if (fuse)
            TryFuse(cs);
        return cs;
    };
    auto csI = lower(false);
    auto csF = lower(true);

    auto outI = Execute(csI, 1, data);
    std::vector<float> rI((float *)outI[0].Data, (float *)outI[0].Data + n);
    free(outI[0].Data);
    if (csF.Fused) // vector-capable target: fused must match within maxUlp
    {
        auto outF = Execute(csF, 1, data);
        std::vector<float> rF((float *)outF[0].Data, (float *)outF[0].Data + n);
        free(outF[0].Data);
        for (size_t i = 0; i < n; i++)
        {
            size_t ulp = UlpDistance(rI[i], rF[i]);
            if (ulp > maxUlp)
            {
                ADD_FAILURE() << "fused and interpreted results differ at element " << i << " ("
                              << rI[i] << " vs " << rF[i] << ", " << ulp << " ulp, max " << maxUlp
                              << ") for: " << expr;
                break; // one report is enough; don't flood on total mismatch
            }
        }
    }
    return rI;
}

TEST(ExprGrammar, AttributeBindingBecomesScalarInput)
{
    // "@attr" is a runtime scalar input named "@attr", not a folded constant
    auto tree = ParseToExprNode("s = @scale \n a * s");
    auto attrs = AttributeNameList(tree);
    ASSERT_EQ(attrs.size(), 1u);
    EXPECT_EQ(attrs[0], "scale");
    std::map<std::string, DataType> t = {{"a", DataType::Float}, {"@scale", DataType::Float}};
    ResolveTreeTypes(tree, t);
    auto cs = GenerateCode(tree);
    SemanticsPass(cs, t);
    PlanBuffers(cs);
    ASSERT_EQ(cs.InputVarNames.size(), 2u);
    for (const auto &buf : cs.Buffers)
    {
        if (buf.IsInput && buf.VarName == "@scale")
        {
            EXPECT_TRUE(buf.IsScalarInput); // auto-marked at generation
        }
    }
}

TEST(ExprGrammar, InlineAttributeSameAsBinding)
{
    // Ana's ask: "@attr + 2" directly in the formula, no binding line needed
    auto inl = ParseToExprNode("a * @scale");
    auto bnd = ParseToExprNode("s = @scale \n a * s");
    EXPECT_EQ(AttributeNameList(inl), AttributeNameList(bnd));
    EXPECT_EQ(VariableNameList(inl), VariableNameList(bnd));
    // pathy attribute names compose with backticks
    auto pathy = ParseToExprNode("a * @`group/scale`");
    ASSERT_EQ(AttributeNameList(pathy).size(), 1u);
    EXPECT_EQ(AttributeNameList(pathy)[0], "group/scale");
    // ... and the backticked form works as a binding RHS too
    auto bound = ParseToExprNode("k = @`has spaces` \n a * k");
    ASSERT_EQ(AttributeNameList(bound).size(), 1u);
    EXPECT_EQ(AttributeNameList(bound)[0], "has spaces");
}

TEST(ExprGrammar, AttributeInputBothWays)
{
    const size_t n = 1027; // force vector body + remainder in the fused path
    std::vector<float> a(n);
    for (size_t i = 0; i < n; i++)
        a[i] = (float)(i % 7) * 1.5f;
    float scale = 3.0f;

    std::map<std::string, std::vector<DerivedData>> data;
    Dims start = {0}, count = {n};
    data["a"] = {{(void *)a.data(), start, count, DataType::Float}};
    DerivedData sdd = {(void *)&scale, {}, {}, DataType::Float};
    sdd.IsScalar = true;
    data["@scale"] = {sdd};

    auto r =
        RunBothWays("a * @scale", {{"a", DataType::Float}, {"@scale", DataType::Float}}, data, n);
    for (size_t i = 0; i < n; i++)
        EXPECT_EQ(r[i], a[i] * scale);
}

TEST(ExprGrammar, BacktickSimpleName)
{
    // backticks around a name with no special characters are valid too
    auto tree = ParseToExprNode("`var` + 1");
    auto vars = VariableNameList(tree);
    ASSERT_EQ(vars.size(), 1u);
    EXPECT_EQ(vars[0], "var");
}

TEST(ExprGrammar, ScalarInputBroadcastBothWays)
{
    const size_t n = 1027; // force vector body + remainder in the fused path
    std::vector<float> a(n);
    for (size_t i = 0; i < n; i++)
        a[i] = (float)(i % 13) * 0.5f;
    float dt = 0.25f;

    std::map<std::string, std::vector<DerivedData>> data;
    Dims start = {0}, count = {n};
    data["a"] = {{(void *)a.data(), start, count, DataType::Float}};
    DerivedData sdd = {(void *)&dt, {}, {}, DataType::Float};
    sdd.IsScalar = true;
    data["dt"] = {sdd};

    auto r = RunBothWays("a * dt + dt", {{"a", DataType::Float}, {"dt", DataType::Float}}, data, n,
                         {"dt"});
    for (size_t i = 0; i < n; i++)
        EXPECT_EQ(r[i], a[i] * dt + dt);
}

TEST(ExprGrammar, ScalarMismatchFallsBack)
{
    // runtime-scalar data with an UNMARKED buffer must not reach fused code
    const size_t n = 8;
    std::vector<float> a(n, 2.0f);
    float s = 3.0f;
    std::map<std::string, std::vector<DerivedData>> data;
    Dims start = {0}, count = {n};
    data["a"] = {{(void *)a.data(), start, count, DataType::Float}};
    DerivedData sdd = {(void *)&s, {}, {}, DataType::Float};
    sdd.IsScalar = true;
    data["s"] = {sdd};
    // no scalarInputs marking: interpreter handles it, values still right
    auto r = RunBothWays("a * s", {{"a", DataType::Float}, {"s", DataType::Float}}, data, n);
    for (size_t i = 0; i < n; i++)
        EXPECT_EQ(r[i], 6.0f);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
