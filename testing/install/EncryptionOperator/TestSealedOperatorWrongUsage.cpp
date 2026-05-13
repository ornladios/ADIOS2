/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstdio>
#include <cstdlib>
#include <fstream>
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
static bool ExpectThrow(const std::string &label, Fn fn)
{
    try
    {
        fn();
        std::cout << "FAIL [" << label << "]: no exception thrown\n";
        return false;
    }
    catch (std::exception &e)
    {
        std::cout << "PASS [" << label << "]: " << e.what() << "\n";
        return true;
    }
}

static adios2::Params AsymParams(const std::string &pk_file = "", const std::string &pk_hex = "",
                                 const std::string &sk_file = "", const std::string &sk_hex = "")
{
    adios2::Params p;
    p["PluginName"] = "Encryption";
    p["PluginLibrary"] = "EncryptionOperator";
    if (!pk_file.empty())
        p["PublicKeyFile"] = pk_file;
    if (!pk_hex.empty())
        p["PublicKey"] = pk_hex;
    if (!sk_file.empty())
        p["SecretKeyFile"] = sk_file;
    if (!sk_hex.empty())
        p["SecretKey"] = sk_hex;
    return p;
}

static adios2::Params SymParams(const std::string &sk_hex = "")
{
    adios2::Params p;
    p["PluginName"] = "Encryption";
    p["PluginLibrary"] = "EncryptionOperator";
    if (!sk_hex.empty())
        p["SecretKey"] = sk_hex;
    return p;
}

static void WriteEncrypted(const std::string &file, const adios2::Params &params)
{
    std::vector<double> data(20);
    std::iota(data.begin(), data.end(), 0.0);
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("w_" + file);
    io.SetEngine("BPFile");
    auto var = io.DefineVariable<double>("data", {}, {}, {data.size()}, adios2::ConstantDims);
    var.AddOperation("plugin", params);
    auto writer = io.Open(file, adios2::Mode::Write);
    try
    {
        writer.BeginStep();
        writer.Put<double>(var, data.data());
        writer.EndStep();
    }
    catch (...)
    {
        try
        {
            writer.Close();
        }
        catch (...)
        {
        }
        throw;
    }
    writer.Close();
}

static void ReadEncrypted(const std::string &file, const adios2::Params &params)
{
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("r_" + file);
    auto reader = io.Open(file, adios2::Mode::Read);
    try
    {
        reader.BeginStep();
        auto var = io.InquireVariable<double>("data");
        if (!params.empty())
            var.AddOperation("plugin", params);
        std::vector<double> result;
        reader.Get<double>(var, result);
        reader.PerformGets();
        reader.EndStep();
    }
    catch (...)
    {
        try
        {
            reader.Close();
        }
        catch (...)
        {
        }
        throw;
    }
    reader.Close();
}

// Read a 32-byte binary key file and return its hex representation (64 chars).
static std::string KeyFileToHex(const std::string &path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f)
        throw std::runtime_error("KeyFileToHex: cannot open " + path);
    std::vector<unsigned char> b(32);
    f.read(reinterpret_cast<char *>(b.data()), 32);
    std::string hex;
    char buf[3];
    for (auto byte : b)
    {
        snprintf(buf, sizeof(buf), "%02x", byte);
        hex += buf;
    }
    return hex;
}

int main()
{
    int failures = 0;
    const std::string kValidBP = "testSealedWrongUsage.bp";
    const std::string kValidSymBP = "testSymWrongUsage.bp";
    const std::string kZeros64(64, '0');
    const std::string kSymKeyHex(64, 'a'); // 32 x 0xaa bytes

    // Setup: write an asym-encrypted file and a sym-encrypted file.
    try
    {
        WriteEncrypted(kValidBP, AsymParams("test-public.key"));
    }
    catch (std::exception &e)
    {
        std::cout << "FATAL (asym write): " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    try
    {
        WriteEncrypted(kValidSymBP, SymParams(kSymKeyHex));
    }
    catch (std::exception &e)
    {
        std::cout << "FATAL (sym write): " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    // Read the test keys as hex for priority ordering tests.
    // Priority tests work by supplying a WRONG value at a higher-priority source
    // and a CORRECT value at a lower-priority source; decryption must fail,
    // proving the higher-priority source was used.
    std::string correctSKHex, correctPKHex;
    try
    {
        correctSKHex = KeyFileToHex("test-secret.key");
        correctPKHex = KeyFileToHex("test-public.key");
    }
    catch (std::exception &e)
    {
        std::cout << "FATAL (key hex): " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    // --- Existing wrong-usage tests ---

    failures += !ExpectThrow("ReadWithNoSecretKey", [&] { ReadEncrypted(kValidBP, {}); });

    failures += !ExpectThrow("ReadWithWrongSecretKey", [&] {
        ReadEncrypted(kValidBP, AsymParams("test-public.key", "", "", kZeros64));
    });

    failures += !ExpectThrow("ReadWithWrongPublicKey", [&] {
        ReadEncrypted(kValidBP, AsymParams("", kZeros64, "test-secret.key"));
    });

    // --- Priority order: env file > env hex > param ---
    //
    // For wrong-file tests we use an existing but wrong-content file to avoid
    // constructor throws (which do not propagate cleanly through ADIOS2):
    //   wrong SK file = "test-public.key"  (32 bytes, wrong value as secret key)
    //   wrong PK file = "test-secret.key"  (32 bytes, wrong value as public key)

    // Asym SK: env file beats env hex
    failures += !ExpectThrow("PriorityAsymSKEnvFileOverEnvHex", [&] {
        // Wrong SK file at env, correct SK at env hex → env file wins → decrypt fails.
        EnvGuard env{{"ADIOS2_ASYM_SECRET_KEY_FILE", "test-public.key"},
                     {"ADIOS2_ASYM_SECRET_KEY", correctSKHex}};
        ReadEncrypted(kValidBP, AsymParams("test-public.key"));
    });

    // Asym SK: env file beats param
    failures += !ExpectThrow("PriorityAsymSKEnvFileOverParam", [&] {
        EnvGuard env{{"ADIOS2_ASYM_SECRET_KEY_FILE", "test-public.key"}};
        ReadEncrypted(kValidBP, AsymParams("test-public.key", "", "test-secret.key"));
    });

    // Asym SK: env hex beats param
    failures += !ExpectThrow("PriorityAsymSKEnvHexOverParam", [&] {
        EnvGuard env{{"ADIOS2_ASYM_SECRET_KEY", kZeros64}};
        ReadEncrypted(kValidBP, AsymParams("test-public.key", "", "test-secret.key"));
    });

    // Asym PK: env file beats env hex
    failures += !ExpectThrow("PriorityAsymPKEnvFileOverEnvHex", [&] {
        // Wrong PK file at env, correct PK at env hex → env file wins → decrypt fails.
        EnvGuard env{{"ADIOS2_PUBLIC_KEY_FILE", "test-secret.key"},
                     {"ADIOS2_PUBLIC_KEY", correctPKHex},
                     {"ADIOS2_ASYM_SECRET_KEY_FILE", "test-secret.key"}};
        ReadEncrypted(kValidBP, {});
    });

    // Asym PK: env file beats param
    failures += !ExpectThrow("PriorityAsymPKEnvFileOverParam", [&] {
        EnvGuard env{{"ADIOS2_PUBLIC_KEY_FILE", "test-secret.key"},
                     {"ADIOS2_ASYM_SECRET_KEY_FILE", "test-secret.key"}};
        ReadEncrypted(kValidBP, AsymParams("test-public.key"));
    });

    // Asym PK: env hex beats param
    failures += !ExpectThrow("PriorityAsymPKEnvHexOverParam", [&] {
        EnvGuard env{{"ADIOS2_PUBLIC_KEY", kZeros64},
                     {"ADIOS2_ASYM_SECRET_KEY_FILE", "test-secret.key"}};
        ReadEncrypted(kValidBP, AsymParams("test-public.key"));
    });

    // Sym key: env hex beats param
    failures += !ExpectThrow("PrioritySymEnvHexOverParam", [&] {
        EnvGuard env{{"ADIOS2_SECRET_KEY", kZeros64}};
        ReadEncrypted(kValidSymBP, SymParams(kSymKeyHex));
    });

    if (failures == 0)
    {
        std::cout << "All wrong-usage tests passed.\n";
        return EXIT_SUCCESS;
    }
    std::cout << failures << " wrong-usage test(s) failed.\n";
    return EXIT_FAILURE;
}
