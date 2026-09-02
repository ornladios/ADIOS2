/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdexcept>
#include <string>

#include <adios2.h>
#include <adios2/helper/adiosCommDummy.h>
#include <adios2/helper/adiosYAML.h>

#include <gtest/gtest.h>

#define str_helper(X) #X
#define str(X) str_helper(X)

namespace
{

std::string ConfigPath(const std::string &name)
{
    return str(YAML_CONFIG_DIR) + std::string(&adios2::PathSeparator, 1) + name;
}

TEST(YAMLHostOptions, HTTPS)
{
    adios2::helper::Comm comm = adios2::helper::CommDummy();
    adios2::HostOptions hosts;
    std::string homePath = "/home/campaign-user";

    EXPECT_NO_THROW(adios2::helper::ParseHostOptionsFile(comm, ConfigPath("hosts-https.yaml"),
                                                         hosts, homePath));

    ASSERT_EQ(hosts.size(), 1);
    const auto host = hosts.find("docker-https");
    ASSERT_NE(host, hosts.end());
    ASSERT_EQ(host->second.size(), 1);

    const adios2::HostConfig &config = host->second.front();
    EXPECT_EQ(config.name, "docker-https-loopback");
    EXPECT_EQ(config.protocol, adios2::HostAccessProtocol::HTTPS);
    EXPECT_EQ(config.endpoint, "https://127.0.0.1:8443");
    EXPECT_EQ(config.caFile, "/home/campaign-user/certs/hpc-campaign-test.crt");
    EXPECT_EQ(config.verbose, 2);
}

TEST(YAMLHostOptions, HTTPSEndpointIsRequired)
{
    adios2::helper::Comm comm = adios2::helper::CommDummy();
    adios2::HostOptions hosts;
    std::string homePath = "/home/campaign-user";

    EXPECT_THROW(adios2::helper::ParseHostOptionsFile(
                     comm, ConfigPath("hosts-https-missing-endpoint.yaml"), hosts, homePath),
                 std::invalid_argument);
}

} // end anonymous namespace

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
