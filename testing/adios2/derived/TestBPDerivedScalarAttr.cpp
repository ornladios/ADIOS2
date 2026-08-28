/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Engine round-trip for the binding forms: a single-value (scalar) variable
// input broadcast per step, and an attribute folded to a constant.

#include <cmath>
#include <numeric>
#include <vector>

#include <adios2.h>
#include <gtest/gtest.h>

TEST(DerivedScalarAttr, WriterSide)
{
    const size_t N = 100;
    const size_t steps = 3;
    std::vector<float> a(N);
    std::iota(a.begin(), a.end(), 1.0f);

    adios2::ADIOS adios;
    {
        adios2::IO io = adios.DeclareIO("w");
        io.SetEngine("BP5");
        io.DefineAttribute<float>("offset", 100.0f, "", "/", true); // modifiable
        auto va = io.DefineVariable<float>("a", {N}, {0}, {N});
        auto vdt = io.DefineVariable<float>("dt"); // single value
        io.DefineDerivedVariable("scaled", "a * dt + @offset", adios2::DerivedVarType::StoreData);
        adios2::Engine w = io.Open("scalar_attr.bp", adios2::Mode::Write);
        for (size_t s = 0; s < steps; s++)
        {
            w.BeginStep();
            // attribute changes per step; derived must use the current value
            io.DefineAttribute<float>("offset", 100.0f * (float)(s + 1), "", "/", true);
            w.Put(va, a.data());
            w.Put(vdt, 0.5f * (float)(s + 1)); // varies per step
            w.EndStep();
        }
        w.Close();
    }
    {
        adios2::IO io = adios.DeclareIO("r");
        io.SetEngine("BP5");
        adios2::Engine r = io.Open("scalar_attr.bp", adios2::Mode::Read);
        for (size_t s = 0; s < steps; s++)
        {
            r.BeginStep();
            auto v = io.InquireVariable<float>("scaled");
            ASSERT_TRUE(v);
            std::vector<float> out(N);
            r.Get(v, out.data());
            r.EndStep();
            float dt = 0.5f * (float)(s + 1);
            float k = 100.0f * (float)(s + 1);
            for (size_t i = 0; i < N; i++)
                ASSERT_EQ(out[i], a[i] * dt + k) << "step " << s << " elem " << i;
        }
        r.Close();
    }
}

TEST(DerivedScalarAttr, ReaderDefined)
{
    const size_t N = 100;
    const size_t steps = 3;
    std::vector<float> a(N);
    std::iota(a.begin(), a.end(), 1.0f);

    adios2::ADIOS adios;
    {
        adios2::IO io = adios.DeclareIO("w2");
        io.SetEngine("BP5");
        io.DefineAttribute<float>("offset", 100.0f, "", "/", true); // modifiable
        auto va = io.DefineVariable<float>("a", {N}, {0}, {N});
        auto vdt = io.DefineVariable<float>("dt");
        adios2::Engine w = io.Open("scalar_attr_rd.bp", adios2::Mode::Write);
        for (size_t s = 0; s < steps; s++)
        {
            w.BeginStep();
            io.DefineAttribute<float>("offset", 100.0f * (float)(s + 1), "", "/", true);
            w.Put(va, a.data());
            w.Put(vdt, 0.5f * (float)(s + 1));
            w.EndStep();
        }
        w.Close();
    }
    {
        adios2::IO io = adios.DeclareIO("r2");
        io.SetEngine("BP5");
        io.DefineReaderDerivedVariable("scaled", "a * dt + @offset");
        adios2::Engine r = io.Open("scalar_attr_rd.bp", adios2::Mode::Read);
        for (size_t s = 0; s < steps; s++)
        {
            ASSERT_EQ(r.BeginStep(), adios2::StepStatus::OK);
            auto v = io.InquireVariable<float>("scaled");
            ASSERT_TRUE(v);
            std::vector<float> out(N);
            r.Get(v, out.data());
            r.EndStep();
            float dt = 0.5f * (float)(s + 1);
            float k = 100.0f * (float)(s + 1);
            for (size_t i = 0; i < N; i++)
                ASSERT_EQ(out[i], a[i] * dt + k) << "step " << s << " elem " << i;
        }
        r.Close();
    }
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
