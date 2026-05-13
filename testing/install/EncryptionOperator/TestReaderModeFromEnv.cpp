/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Verifies that the reader selects asymmetric or symmetric mode purely from
 * env vars when no operator parameters are given on the read side.
 *
 * Setting ADIOS2_ASYM_* triggers asymmetric mode.
 * Setting only ADIOS2_SECRET_KEY with no asym indicators triggers symmetric.
 * Using the wrong mode's env vars causes a decryption failure.
 */

#include <cstdlib>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include "adios2.h"

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
static bool Expect(const std::string &label, bool shouldPass, Fn fn)
{
    try
    {
        fn();
        if (shouldPass)
        {
            std::cout << "PASS [" << label << "]\n";
            return true;
        }
        std::cout << "FAIL [" << label << "]: expected exception but none thrown\n";
        return false;
    }
    catch (std::exception &e)
    {
        if (!shouldPass)
        {
            std::cout << "PASS [" << label << "]: " << e.what() << "\n";
            return true;
        }
        std::cout << "FAIL [" << label << "]: unexpected exception: " << e.what() << "\n";
        return false;
    }
}

static const std::vector<double> kData = []() {
    std::vector<double> v(20);
    std::iota(v.begin(), v.end(), 0.0);
    return v;
}();

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

// Read with no operator params — relies on stored metadata + env vars.
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
    const std::string kSymKeyHex(64, 'a'); // 32 x 0xaa bytes

    // Setup: write a sym file and an asym file.
    adios2::Params symWriteParams;
    symWriteParams["PluginName"] = "Encryption";
    symWriteParams["PluginLibrary"] = "EncryptionOperator";
    symWriteParams["SecretKey"] = kSymKeyHex;

    adios2::Params asymWriteParams;
    asymWriteParams["PluginName"] = "Encryption";
    asymWriteParams["PluginLibrary"] = "EncryptionOperator";
    asymWriteParams["PublicKeyFile"] = "test-public.key";

    try
    {
        WriteWith("testModeSelectSym.bp", symWriteParams);
        WriteWith("testModeSelectAsym.bp", asymWriteParams);
    }
    catch (std::exception &e)
    {
        std::cout << "FATAL (write): " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    // PASS: sym data, sym env var → sym mode selected, decryption succeeds.
    failures += !Expect("SymDataEnvSym", true, [&] {
        EnvGuard env{{"ADIOS2_SECRET_KEY", kSymKeyHex}};
        ReadNoKeyParams("testModeSelectSym.bp");
    });

    // PASS: asym data, asym env var → asym mode selected, decryption succeeds.
    failures += !Expect("AsymDataEnvAsym", true, [&] {
        EnvGuard env{{"ADIOS2_ASYM_SECRET_KEY_FILE", "test-secret.key"}};
        ReadNoKeyParams("testModeSelectAsym.bp");
    });

    // FAIL: sym data, asym env var → asym mode selected, sym data format mismatch.
    failures += !Expect("SymDataEnvAsym", false, [&] {
        EnvGuard env{{"ADIOS2_ASYM_SECRET_KEY_FILE", "test-secret.key"}};
        ReadNoKeyParams("testModeSelectSym.bp");
    });

    // FAIL: asym data, sym env var → asym mode from stored metadata (PublicKeyFile),
    //       sym env var not used for asym secret key → no secret key loaded.
    failures += !Expect("AsymDataEnvSym", false, [&] {
        EnvGuard env{{"ADIOS2_SECRET_KEY", kSymKeyHex}};
        ReadNoKeyParams("testModeSelectAsym.bp");
    });

    if (failures == 0)
    {
        std::cout << "All reader-mode-from-env tests passed.\n";
        return EXIT_SUCCESS;
    }
    std::cout << failures << " reader-mode-from-env test(s) failed.\n";
    return EXIT_FAILURE;
}
