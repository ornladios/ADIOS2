/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Tests that EncryptionOperator loads keys via environment variables with no
 * key parameters on the reader side.
 *
 * Env vars exercised:
 *   ADIOS2_ASYM_SECRET_KEY_FILE  — asymmetric secret key, file path
 *   ADIOS2_ASYM_SECRET_KEY       — asymmetric secret key, hex string
 *   ADIOS2_SECRET_KEY            — symmetric secret key, hex string
 */

#include <cstdlib>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include "adios2.h"

// RAII: set env vars for the duration of a scope, then unset them.
struct EnvGuard
{
    std::vector<std::string> Keys;
    EnvGuard(std::initializer_list<std::pair<std::string, std::string>> vars)
    {
        for (auto &kv : vars)
        {
            setenv(kv.first.c_str(), kv.second.c_str(), 1);
            Keys.push_back(kv.first);
        }
    }
    ~EnvGuard()
    {
        for (auto &k : Keys)
            unsetenv(k.c_str());
    }
};

template <typename Fn>
static bool Expect(const std::string &label, Fn fn)
{
    try
    {
        fn();
        std::cout << "PASS [" << label << "]\n";
        return true;
    }
    catch (std::exception &e)
    {
        std::cout << "FAIL [" << label << "]: " << e.what() << "\n";
        return false;
    }
}

static const std::vector<double> kData = []() {
    std::vector<double> v(20);
    std::iota(v.begin(), v.end(), 0.0);
    return v;
}();

static adios2::Params PluginParams(const adios2::Params &extra = {})
{
    adios2::Params p;
    p["PluginName"] = "Encryption";
    p["PluginLibrary"] = "EncryptionOperator";
    for (auto &kv : extra)
        p[kv.first] = kv.second;
    return p;
}

static void WriteWith(const std::string &bp, const adios2::Params &params)
{
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("w_" + bp);
    io.SetEngine("BPFile");
    auto var = io.DefineVariable<double>("data", {}, {}, {kData.size()}, adios2::ConstantDims);
    var.AddOperation("plugin", params);
    auto eng = io.Open(bp, adios2::Mode::Write);
    eng.BeginStep();
    eng.Put<double>(var, kData.data());
    eng.EndStep();
    eng.Close();
}

// Read without providing any key parameters — relies entirely on env vars.
static void ReadNoKeyParams(const std::string &bp)
{
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("r_" + bp);
    auto eng = io.Open(bp, adios2::Mode::Read);
    eng.BeginStep();
    auto var = io.InquireVariable<double>("data");
    std::vector<double> result;
    eng.Get<double>(var, result);
    eng.PerformGets();
    eng.EndStep();
    eng.Close();
    if (result != kData)
        throw std::runtime_error("data mismatch after decryption");
}

int main()
{
    int failures = 0;

    // -----------------------------------------------------------------------
    // Symmetric: write with SecretKeyFile param, read with ADIOS2_SECRET_KEY
    // -----------------------------------------------------------------------
    // Use a sodium hex key (64 zeros = all-zero 32-byte key) just for the test.
    // The writer uses the param to generate/store the key file; the env var
    // provides the same key to the reader as a hex string.
    const std::string kSymKeyHex(64, '0'); // 32 zero bytes in hex

    failures += !Expect("SymWrite", [&] {
        // Write with an explicit hex key via param.
        WriteWith("testSymEnvVar.bp", PluginParams({{"SecretKey", kSymKeyHex}}));
    });
    failures += !Expect("SymReadEnvHex", [&] {
        EnvGuard env{{"ADIOS2_SECRET_KEY", kSymKeyHex}};
        ReadNoKeyParams("testSymEnvVar.bp");
    });

    // -----------------------------------------------------------------------
    // Asymmetric: write with PublicKeyFile param, read with
    // ADIOS2_ASYM_SECRET_KEY_FILE (no public key param needed on reader side)
    // -----------------------------------------------------------------------
    failures += !Expect("AsymWrite", [&] {
        WriteWith("testAsymEnvFile.bp", PluginParams({{"PublicKeyFile", "test-public.key"}}));
    });
    failures += !Expect("AsymReadEnvFile", [&] {
        EnvGuard env{{"ADIOS2_ASYM_SECRET_KEY_FILE", "test-secret.key"}};
        ReadNoKeyParams("testAsymEnvFile.bp");
    });

    // -----------------------------------------------------------------------
    // Asymmetric: writer uses ADIOS2_PUBLIC_KEY_FILE env var (no param),
    // reader uses ADIOS2_ASYM_SECRET_KEY_FILE env var (no param)
    // -----------------------------------------------------------------------
    failures += !Expect("AsymWriteEnvPKFile", [&] {
        EnvGuard env{{"ADIOS2_PUBLIC_KEY_FILE", "test-public.key"}};
        WriteWith("testAsymWriteEnvPK.bp", PluginParams());
    });
    failures += !Expect("AsymReadEnvSKFile", [&] {
        EnvGuard env{{"ADIOS2_ASYM_SECRET_KEY_FILE", "test-secret.key"}};
        ReadNoKeyParams("testAsymWriteEnvPK.bp");
    });

    // -----------------------------------------------------------------------
    // Asymmetric: write and read using ADIOS2_ASYM_SECRET_KEY_FILE only —
    // the public key is derived from the secret key on both sides.
    // -----------------------------------------------------------------------
    failures += !Expect("AsymWriteDerivedPK", [&] {
        EnvGuard env{{"ADIOS2_ASYM_SECRET_KEY_FILE", "test-secret.key"}};
        WriteWith("testAsymDerived.bp", PluginParams());
    });
    failures += !Expect("AsymReadDerivedPK", [&] {
        EnvGuard env{{"ADIOS2_ASYM_SECRET_KEY_FILE", "test-secret.key"}};
        ReadNoKeyParams("testAsymDerived.bp");
    });

    if (failures == 0)
    {
        std::cout << "All env-var key tests passed.\n";
        return EXIT_SUCCESS;
    }
    std::cout << failures << " env-var key test(s) failed.\n";
    return EXIT_FAILURE;
}
