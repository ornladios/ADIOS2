/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Randomized global-array round trip.  Each case writes a 1-4D array tiled by
 * irregular blocks, with writer and reader each in either array ordering and
 * optional writer memory selections, then reads a mix of selections per step:
 * whole array, exact blocks, slabs, blocks grown or shrunk by one, and random
 * boxes.  Every element is checked against a function of its global index and
 * guard regions around each destination buffer must stay untouched.
 */

#include <adios2.h>

#include <algorithm>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "../TestHelpers.h"

std::string engineName; // comes from command line

namespace
{

using Dims = adios2::Dims;
using T = int64_t;

const size_t NCases = 256;
const size_t NSteps = 2;
const size_t Guard = 16;
const T Sentinel = (T)0x5A5A5A5A5A5A5A5ALL;

size_t Product(const Dims &c)
{
    return std::accumulate(c.begin(), c.end(), (size_t)1, std::multiplies<size_t>());
}

// linear index of pos in a box of extent cnt; row-major has the last dim fastest
size_t Linear(const Dims &pos, const Dims &cnt, bool rowMajor)
{
    size_t off = 0;
    const size_t n = cnt.size();
    for (size_t k = 0; k < n; ++k)
    {
        const size_t d = rowMajor ? k : n - 1 - k;
        off = off * cnt[d] + pos[d];
    }
    return off;
}

template <class F>
void ForBox(const Dims &cnt, F f)
{
    if (Product(cnt) == 0)
    {
        return;
    }
    Dims p(cnt.size(), 0);
    while (true)
    {
        f(p);
        size_t d = cnt.size();
        while (true)
        {
            if (d == 0)
            {
                return;
            }
            --d;
            if (++p[d] < cnt[d])
            {
                break;
            }
            p[d] = 0;
        }
    }
}

// value of a global element, indexed in the writer's dimension order
T Value(const Dims &g, size_t step)
{
    uint64_t v = step * 1000003ULL + 7;
    for (auto x : g)
    {
        v = v * 1315423911ULL + x + 1;
    }
    return (T)v;
}

Dims Reversed(Dims d)
{
    std::reverse(d.begin(), d.end());
    return d;
}

struct Block
{
    Dims start, count;       // writer order
    Dims memStart, memCount; // empty unless a memory selection is used
};

struct Selection
{
    Dims start, count; // reader order
};

struct Case
{
    size_t id;
    bool writerRowMajor, readerRowMajor;
    Dims shape; // writer order
    std::vector<Block> blocks;
    std::vector<std::vector<Selection>> selections; // per step

    bool Reverse() const { return writerRowMajor != readerRowMajor && shape.size() > 1; }
    Dims ToReader(const Dims &d) const { return Reverse() ? Reversed(d) : d; }
    std::string Describe() const
    {
        std::ostringstream os;
        os << "case " << id << " ndim=" << shape.size() << " writer "
           << (writerRowMajor ? "row" : "col") << "-major, reader "
           << (readerRowMajor ? "row" : "col") << "-major";
        return os.str();
    }
};

Case MakeCase(std::mt19937_64 &rng, size_t id)
{
    auto rnd = [&](size_t lo, size_t hi) {
        return std::uniform_int_distribution<size_t>(lo, hi)(rng);
    };
    Case c;
    c.id = id;
    c.writerRowMajor = id & 1;
    c.readerRowMajor = (id >> 1) & 1;
    // BP5 writer memory selections ignore a C++ IO's ColumnMajor ordering
    const bool memSel = ((id >> 2) & 1) && c.writerRowMajor;
    const size_t n = rnd(1, 4);
    c.shape.resize(n);
    for (auto &s : c.shape)
    {
        s = rnd(1, n >= 3 ? 7 : 13);
    }

    // irregular tiling, including one-element slabs
    std::vector<std::vector<size_t>> cuts(n);
    Dims nblk(n);
    for (size_t d = 0; d < n; ++d)
    {
        cuts[d] = {0, c.shape[d]};
        for (size_t i = rnd(0, std::min<size_t>(c.shape[d] - 1, 3)); i > 0; --i)
        {
            cuts[d].push_back(rnd(1, c.shape[d] - 1));
        }
        std::sort(cuts[d].begin(), cuts[d].end());
        cuts[d].erase(std::unique(cuts[d].begin(), cuts[d].end()), cuts[d].end());
        nblk[d] = cuts[d].size() - 1;
    }
    ForBox(nblk, [&](const Dims &bi) {
        Block b{Dims(n), Dims(n), {}, {}};
        for (size_t d = 0; d < n; ++d)
        {
            b.start[d] = cuts[d][bi[d]];
            b.count[d] = cuts[d][bi[d] + 1] - cuts[d][bi[d]];
        }
        if (memSel)
        {
            b.memStart.resize(n);
            b.memCount.resize(n);
            for (size_t d = 0; d < n; ++d)
            {
                b.memStart[d] = rnd(0, 2);
                b.memCount[d] = b.memStart[d] + b.count[d] + rnd(0, 2);
            }
        }
        c.blocks.push_back(b);
    });
    std::shuffle(c.blocks.begin(), c.blocks.end(), rng);

    const Dims rshape = c.ToReader(c.shape);
    c.selections.resize(NSteps);
    for (auto &stepSels : c.selections)
    {
        for (size_t i = rnd(3, 8); i > 0; --i)
        {
            Selection s{Dims(n, 0), Dims(n, 0)};
            switch (rnd(0, 4))
            {
            case 0: // whole array
                s.count = rshape;
                break;
            case 1: // exactly one written block
            {
                const Block &b = c.blocks[rnd(0, c.blocks.size() - 1)];
                s.start = c.ToReader(b.start);
                s.count = c.ToReader(b.count);
                break;
            }
            case 2: // full in the faster dims, partial in one, 1 in the slower dims
            {
                const size_t k = rnd(0, n - 1);
                for (size_t j = 0; j < n; ++j)
                {
                    const size_t d = c.readerRowMajor ? n - 1 - j : j;
                    if (j < k)
                    {
                        s.count[d] = rshape[d];
                        continue;
                    }
                    s.start[d] = rnd(0, rshape[d] - 1);
                    s.count[d] = j == k ? rnd(1, rshape[d] - s.start[d]) : 1;
                }
                break;
            }
            case 3: // a block grown or shrunk by one element per side
            {
                const Block &b = c.blocks[rnd(0, c.blocks.size() - 1)];
                const Dims bs = c.ToReader(b.start), bc = c.ToReader(b.count);
                for (size_t d = 0; d < n; ++d)
                {
                    size_t lo = (bs[d] > 0 && rnd(0, 1)) ? bs[d] - 1 : bs[d];
                    size_t hi = bs[d] + bc[d];
                    if (hi < rshape[d] && rnd(0, 2) == 0)
                    {
                        ++hi;
                    }
                    else if (hi - lo > 1 && rnd(0, 2) == 0)
                    {
                        --hi;
                    }
                    s.start[d] = lo;
                    s.count[d] = hi - lo;
                }
                break;
            }
            default: // random box
                for (size_t d = 0; d < n; ++d)
                {
                    s.start[d] = rnd(0, rshape[d] - 1);
                    s.count[d] = rnd(1, rshape[d] - s.start[d]);
                }
            }
            stepSels.push_back(s);
        }
    }
    return c;
}

adios2::ArrayOrdering Ordering(bool rowMajor)
{
    return rowMajor ? adios2::ArrayOrdering::RowMajor : adios2::ArrayOrdering::ColumnMajor;
}

bool Write(const Case &c, const std::string &name)
{
    try
    {
        adios2::ADIOS adios;
        adios2::IO io = adios.DeclareIO("W", Ordering(c.writerRowMajor));
        io.SetEngine(engineName);
        const size_t n = c.shape.size();
        auto var = io.DefineVariable<T>("v", c.shape, Dims(n, 0), c.shape);
        adios2::Engine w = io.Open(name, adios2::Mode::Write);
        for (size_t step = 0; step < NSteps; ++step)
        {
            std::vector<std::vector<T>> buffers;
            buffers.reserve(c.blocks.size());
            w.BeginStep();
            for (const Block &b : c.blocks)
            {
                const bool memSel = !b.memCount.empty();
                const Dims memStart = memSel ? b.memStart : Dims(n, 0);
                const Dims memCount = memSel ? b.memCount : b.count;
                buffers.emplace_back(Product(memCount), (T)-1);
                std::vector<T> &buf = buffers.back();
                ForBox(b.count, [&](const Dims &p) {
                    Dims g(n), m(n);
                    for (size_t d = 0; d < n; ++d)
                    {
                        g[d] = b.start[d] + p[d];
                        m[d] = memStart[d] + p[d];
                    }
                    buf[Linear(m, memCount, c.writerRowMajor)] = Value(g, step);
                });
                var.SetSelection({b.start, b.count});
                if (memSel)
                {
                    var.SetMemorySelection({memStart, memCount});
                }
                else
                {
                    var.SetMemorySelection();
                }
                w.Put(var, buf.data());
            }
            w.EndStep();
        }
        w.Close();
    }
    catch (std::exception &e)
    {
        std::cout << c.Describe() << ": writer exception " << e.what() << std::endl;
        return false;
    }
    return true;
}

// returns the number of bad elements, or -1 on failure to read
long Read(const Case &c, const std::string &name)
{
    long bad = 0;
    auto report = [&](const std::string &what) {
        if (bad++ < 5)
        {
            std::cout << c.Describe() << ": " << what << std::endl;
        }
    };
    try
    {
        adios2::ADIOS adios;
        adios2::IO io = adios.DeclareIO("R", Ordering(c.readerRowMajor));
        io.SetEngine(engineName);
        const size_t n = c.shape.size();
        const Dims rshape = c.ToReader(c.shape);
        adios2::Engine r = io.Open(name, adios2::Mode::Read);
        for (size_t step = 0; step < NSteps; ++step)
        {
            if (r.BeginStep() != adios2::StepStatus::OK)
            {
                report("stream ended early");
                return -1;
            }
            auto var = io.InquireVariable<T>("v");
            if (!var || var.Shape() != rshape)
            {
                report("variable missing or shape mismatch");
                return -1;
            }
            const auto &sels = c.selections[step];
            std::vector<std::vector<T>> bufs;
            for (const Selection &s : sels)
            {
                bufs.emplace_back(Product(s.count) + 2 * Guard, Sentinel);
                var.SetSelection({s.start, s.count});
                r.Get(var, bufs.back().data() + Guard);
            }
            r.EndStep();
            for (size_t i = 0; i < sels.size(); ++i)
            {
                const Selection &s = sels[i];
                const std::vector<T> &buf = bufs[i];
                const size_t total = Product(s.count);
                for (size_t g = 0; g < Guard; ++g)
                {
                    if (buf[g] != Sentinel || buf[Guard + total + g] != Sentinel)
                    {
                        report("write outside the selection buffer");
                        break;
                    }
                }
                ForBox(s.count, [&](const Dims &p) {
                    Dims gr(n);
                    for (size_t d = 0; d < n; ++d)
                    {
                        gr[d] = s.start[d] + p[d];
                    }
                    if (buf[Guard + Linear(p, s.count, c.readerRowMajor)] !=
                        Value(c.ToReader(gr), step))
                    {
                        report("wrong value, step " + std::to_string(step));
                    }
                });
            }
        }
        r.Close();
    }
    catch (std::exception &e)
    {
        report(std::string("reader exception ") + e.what());
        return -1;
    }
    return bad;
}

} // namespace

class TestNdSelections : public ::testing::Test
{
};

TEST_F(TestNdSelections, RandomRoundTrip)
{
    std::mt19937_64 rng(20261009);
    for (size_t id = 0; id < NCases; ++id)
    {
        const Case c = MakeCase(rng, id);
        const std::string name = "NdSelections_" + engineName + "_" + std::to_string(id) + ".bp";
        EXPECT_TRUE(Write(c, name)) << c.Describe();
        EXPECT_EQ(Read(c, name), 0) << c.Describe();
        CleanupTestFiles(name);
    }
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    if (argc > 1)
    {
        engineName = std::string(argv[1]);
    }
    return RUN_ALL_TESTS();
}
