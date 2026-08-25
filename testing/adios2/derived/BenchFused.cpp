/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Three-way derived-expression benchmark: interpreter pipeline vs JIT-fused
// (TryFuse/dill vector loop) vs a natively compiled -O3 loop of the same
// expression (what a hand-written kernel would do; clang may contract FMAs
// and vectorize).  Run over several expression shapes.

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "adios2/helper/adiosType.h"
#include "adios2/toolkit/derived/ExprCodeStream.h"
#include "adios2/toolkit/derived/ExprNode.h"

using namespace adios2;
using namespace adios2::derived;
using namespace adios2::detail;

static const size_t N = 10 * 1000 * 1000;
static const int REPS = 5;

template <class F>
static double BestMs(F &&f)
{
    double best = 1e30;
    for (int r = 0; r < REPS; r++)
    {
        auto t0 = std::chrono::steady_clock::now();
        f();
        auto t1 = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        if (ms < best)
            best = ms;
    }
    return best;
}

template <class T>
struct Case
{
    const char *name;
    const char *expr;
    std::vector<std::string> vars;
    // native -O3 reference loop: out[i] = f(inputs, i)
    std::function<void(const std::vector<const T *> &, T *, size_t)> native;
};

template <class T>
static void RunCase(const Case<T> &c)
{
    DataType dt = helper::GetDataType<T>();

    // input data
    std::map<std::string, std::vector<T>> data;
    std::vector<const T *> rawIn;
    for (size_t v = 0; v < c.vars.size(); v++)
    {
        auto &vec = data[c.vars[v]];
        vec.resize(N);
        for (size_t i = 0; i < N; i++)
            vec[i] = (T)(0.25 + ((i * (v + 3)) % 97) * 0.03125);
    }
    for (auto &v : c.vars)
        rawIn.push_back(data[v].data());

    // build twice: interpreter-only and fused
    auto build = [&](bool fuse) {
        auto tree = ParseToExprNode(c.expr);
        std::map<std::string, DataType> types;
        for (auto &v : c.vars)
            types[v] = dt;
        ResolveTreeTypes(tree, types);
        auto cs = GenerateCode(tree);
        SemanticsPass(cs, types);
        PlanBuffers(cs);
        if (fuse)
            TryFuse(cs);
        return cs;
    };
    auto csI = build(false);
    auto csF = build(true);

    std::map<std::string, std::vector<DerivedData>> inputData;
    Dims start = {0}, count = {N};
    for (auto &v : c.vars)
        inputData[v] = {{(void *)data[v].data(), start, count, dt}};

    double msI = BestMs([&] {
        auto out = Execute(csI, 1, inputData);
        free(out[0].Data);
    });
    double msF = -1;
    if (csF.Fused)
        msF = BestMs([&] {
            auto out = Execute(csF, 1, inputData);
            free(out[0].Data);
        });
    double msN = BestMs([&] {
        T *out = (T *)malloc(N * sizeof(T));
        c.native(rawIn, out, N);
        free(out);
    });

    double bytes = (double)(c.vars.size() + 1) * N * sizeof(T);
    auto gbs = [&](double ms) { return bytes / (ms * 1e-3) / 1e9; };

    printf("%-34s %-6s", c.name, sizeof(T) == 4 ? "float" : "double");
    printf(" | interp %8.2f ms %6.1f GB/s", msI, gbs(msI));
    if (msF > 0)
        printf(" | fused %7.2f ms %6.1f GB/s (%4.2fx)", msF, gbs(msF), msI / msF);
    else
        printf(" | fused      NOT FUSED           ");
    printf(" | native %7.2f ms %6.1f GB/s", msN, gbs(msN));
    if (msF > 0)
        printf("  fused/native %.0f%%", 100.0 * msN / msF);
    printf("\n");
}

int main()
{
    printf("N = %zu elements, best of %d reps\n\n", N, REPS);

    RunCase<float>(
        {"a+b", "a+b", {"a", "b"}, [](const std::vector<const float *> &in, float *out, size_t n) {
             for (size_t i = 0; i < n; i++)
                 out[i] = in[0][i] + in[1][i];
         }});

    RunCase<float>({"a*2.5 + b*c - a/2.0",
                    "a*2.5 + b*c - a / 2.0",
                    {"a", "b", "c"},
                    [](const std::vector<const float *> &in, float *out, size_t n) {
                        for (size_t i = 0; i < n; i++)
                            out[i] = in[0][i] * 2.5f + in[1][i] * in[2][i] - in[0][i] / 2.0f;
                    }});

    RunCase<float>({"magnitude",
                    "sqrt(a*a + b*b + c*c)",
                    {"a", "b", "c"},
                    [](const std::vector<const float *> &in, float *out, size_t n) {
                        for (size_t i = 0; i < n; i++)
                            out[i] = std::sqrt(in[0][i] * in[0][i] + in[1][i] * in[1][i] +
                                               in[2][i] * in[2][i]);
                    }});

    RunCase<double>({"magnitude",
                     "sqrt(a*a + b*b + c*c)",
                     {"a", "b", "c"},
                     [](const std::vector<const double *> &in, double *out, size_t n) {
                         for (size_t i = 0; i < n; i++)
                             out[i] = std::sqrt(in[0][i] * in[0][i] + in[1][i] * in[1][i] +
                                                in[2][i] * in[2][i]);
                     }});

    // Ana's relativistic energy (per-element rest mass), constants inlined,
    // squares written as products so the element-wise fuser takes it.
    RunCase<double>({"energy (Ana)",
                     "sqrt((ux*ux + uy*uy + uz*uz)*8.98755178736818e16 + "
                     "(m0*2.99792458e8)*(m0*2.99792458e8))",
                     {"ux", "uy", "uz", "m0"},
                     [](const std::vector<const double *> &in, double *out, size_t n) {
                         const double c2 = 8.98755178736818e16, c = 2.99792458e8;
                         for (size_t i = 0; i < n; i++)
                         {
                             double u2 =
                                 in[0][i] * in[0][i] + in[1][i] * in[1][i] + in[2][i] * in[2][i];
                             double mc = in[3][i] * c;
                             out[i] = std::sqrt(u2 * c2 + mc * mc);
                         }
                     }});

    // built-in aggregated magnitude operator: a compiled single-pass C++
    // kernel (the thing the original scalar JIT lost to).  Never fuseable
    // (Reshape selection rule) so "fused" reports NOT FUSED; compare its
    // interp number against the sqrt-form fused number above.
    RunCase<float>({"magnitude() operator",
                    "magnitude(a,b,c)",
                    {"a", "b", "c"},
                    [](const std::vector<const float *> &in, float *out, size_t n) {
                        for (size_t i = 0; i < n; i++)
                            out[i] = std::sqrt(in[0][i] * in[0][i] + in[1][i] * in[1][i] +
                                               in[2][i] * in[2][i]);
                    }});

    // pow form: SemanticsPass strength-reduces ^2 to a multiply, so it fuses
    RunCase<float>({"magnitude (pow form)",
                    "sqrt(a^2 + b^2 + c^2)",
                    {"a", "b", "c"},
                    [](const std::vector<const float *> &in, float *out, size_t n) {
                        for (size_t i = 0; i < n; i++)
                            out[i] = std::sqrt(in[0][i] * in[0][i] + in[1][i] * in[1][i] +
                                               in[2][i] * in[2][i]);
                    }});

    return 0;
}
