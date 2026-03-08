/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include "adios2.h"

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

int main()
{
    int failures = 0;
    const std::string kValidBP = "testSealedWrongUsage.bp";
    const std::string kZeros64(64, '0');

    try
    {
        WriteEncrypted(kValidBP, AsymParams("test-public.key"));
    }
    catch (std::exception &e)
    {
        std::cout << "FATAL: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    // Read-side decryption errors (exceptions from InverseOperate, not constructor)
    failures += !ExpectThrow("ReadWithNoSecretKey", [&] { ReadEncrypted(kValidBP, {}); });

    failures += !ExpectThrow("ReadWithWrongSecretKey", [&] {
        ReadEncrypted(kValidBP, AsymParams("test-public.key", "", "", kZeros64));
    });

    failures += !ExpectThrow("ReadWithWrongPublicKey", [&] {
        ReadEncrypted(kValidBP, AsymParams("", kZeros64, "test-secret.key"));
    });

    if (failures == 0)
    {
        std::cout << "All wrong-usage tests passed.\n";
        return EXIT_SUCCESS;
    }
    std::cout << failures << " wrong-usage test(s) failed.\n";
    return EXIT_FAILURE;
}
