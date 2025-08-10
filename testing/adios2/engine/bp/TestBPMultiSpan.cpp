/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TestBPMultiSpan
 *
 */

#include <cstdint>
#include <cstring>

#include <iostream>
#include <numeric> //std::iota
#include <stdexcept>
#include <vector>

#include <adios2.h>

#include <gtest/gtest.h>

std::string engineName;       // comes from command line
std::string engineParameters; // comes from command line

class SpanTests : public ::testing::Test
{
public:
    SpanTests() = default;
};

TEST_F(SpanTests, MultiSpan)
{

    using namespace adios2;
    using type = size_t;

    const std::string fname("MultiSpan.bp");

    adios2::ADIOS adios;
    {
        IO IO = adios.DeclareIO("IO");
        if (!engineName.empty())
        {
            IO.SetEngine(engineName);
        }
        if (!engineParameters.empty())
        {
            IO.SetParameters(engineParameters);
        }

        Engine engine = IO.Open(fname, Mode::Write);

        Variable<type> spanvar = IO.DefineVariable<type>("span", {10, 10});

        engine.BeginStep();

        std::vector<Variable<type>::Span> spans;
        spans.reserve(10);

        for (size_t i = 0; i < 10; ++i)
        {
            spanvar.SetSelection({{i, 0}, {1, 10}});
            spans.push_back(engine.Put<type>(spanvar));
        }

        for (size_t i = 0; i < 10; ++i)
        {
            std::cout << "Span pointer: " << spans[i].data() << '\n';
            if (i > 1)
            {
                EXPECT_NE(spans[i].data(), spans[i - 1].data());
            }
            std::iota(spans[i].begin(), spans[i].end(), (type)i * 10);
        }

        engine.EndStep();
        engine.Close();
    }
    {
        adios2::IO io = adios.DeclareIO("IO2");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        if (!engineParameters.empty())
        {
            io.SetParameters(engineParameters);
        }

        adios2::Engine bpReader = io.Open(fname, adios2::Mode::ReadRandomAccess);
        adios2::Variable<type> var = io.InquireVariable<type>("span");

        std::vector<type> globalData(10 * 10);
        bpReader.Get(var, globalData);
        bpReader.PerformGets();

        type expected = 0;
        for (auto &item : globalData)
        {
            EXPECT_EQ(item, expected++);
            std::cout << " " << item;
            if ((expected % 10) == 0)
                std::cout << std::endl;
        }
        std::cout << std::endl;
        bpReader.Close();
    }
}

int main(int argc, char **argv)
{

    int result;
    ::testing::InitGoogleTest(&argc, argv);

    if (argc > 1)
    {
        engineName = std::string(argv[1]);
    }
    if (argc > 2)
    {
        engineParameters = std::string(argv[2]);
    }
    result = RUN_ALL_TESTS();

    return result;
}
