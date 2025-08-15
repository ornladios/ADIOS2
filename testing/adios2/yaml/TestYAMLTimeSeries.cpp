#include <cstdint>

#include <iostream>
#include <stdexcept>

#include <adios2.h>
#include <adios2/helper/adiosCommDummy.h>
#include <adios2/helper/adiosYAML.h>

#include <gtest/gtest.h>

#define str_helper(X) #X
#define str(X) str_helper(X)

class YAMLTimeSeriesTest : public ::testing::Test
{
public:
    YAMLTimeSeriesTest() : configDir(str(YAML_CONFIG_DIR)) {}

    std::string configDir;
};

TEST_F(YAMLTimeSeriesTest, Local)
{
    const std::string atsFile(configDir + std::string(&adios2::PathSeparator, 1) + "local.ats");

    adios2::helper::Comm comm = adios2::helper::CommDummy();

    adios2::helper::TimeSeriesList tsl;
    EXPECT_NO_THROW(adios2::helper::ParseTimeSeriesFile(comm, atsFile, tsl));

    EXPECT_TRUE(tsl.ended);
    EXPECT_EQ(tsl.entries.size(), 4);
    EXPECT_EQ(tsl.entries[0].localpath, "/tmp/data/local_1.h5");
    EXPECT_EQ(tsl.entries[1].localpath, "/tmp/data/local_2.h5");
    EXPECT_EQ(tsl.entries[2].localpath, "/tmp/data/local_3.h5");
    EXPECT_EQ(tsl.entries[3].localpath, "/tmp/data/local_4.h5");
}

TEST_F(YAMLTimeSeriesTest, Remote)
{
    const std::string atsFile(configDir + std::string(&adios2::PathSeparator, 1) + "remote.ats");

    adios2::helper::Comm comm = adios2::helper::CommDummy();

    adios2::helper::TimeSeriesList tsl;
    EXPECT_NO_THROW(adios2::helper::ParseTimeSeriesFile(comm, atsFile, tsl));

    EXPECT_FALSE(tsl.ended);
    EXPECT_EQ(tsl.entries.size(), 2);
    EXPECT_EQ(tsl.entries[0].localpath, "/home/adios/data/remote_local_1");
    EXPECT_EQ(tsl.entries[0].remotepath, "/remote/path1");
    EXPECT_EQ(tsl.entries[0].remotehost, "Host1");
    EXPECT_EQ(tsl.entries[0].uuid, "0123456789abcdef0123456879abcdef");
    EXPECT_EQ(tsl.entries[1].localpath, "/home/adios/data/remote_local_2");
    EXPECT_EQ(tsl.entries[1].remotepath, "/remote/path2");
    EXPECT_EQ(tsl.entries[1].remotehost, "Host2");
    EXPECT_EQ(tsl.entries[1].uuid, "abcdef0123456879abcdef0123456789");
}

TEST_F(YAMLTimeSeriesTest, Mixed)
{
    const std::string atsFile(configDir + std::string(&adios2::PathSeparator, 1) + "mixed.ats");

    adios2::helper::Comm comm = adios2::helper::CommDummy();

    adios2::helper::TimeSeriesList tsl;
    EXPECT_NO_THROW(adios2::helper::ParseTimeSeriesFile(comm, atsFile, tsl));

    EXPECT_TRUE(tsl.ended);
    EXPECT_EQ(tsl.entries.size(), 4);
    EXPECT_EQ(tsl.entries[0].localpath, "/tmp/data/local_1.h5");
    EXPECT_EQ(tsl.entries[1].localpath, "/home/adios/data/remote_local_1");
    EXPECT_EQ(tsl.entries[2].localpath, "/tmp/data/local_2.h5");
    EXPECT_EQ(tsl.entries[3].localpath, "/home/adios/data/remote_local_2");

    EXPECT_TRUE(tsl.entries[0].remotepath.empty());
    EXPECT_EQ(tsl.entries[1].remotepath, "/remote/path1");
    EXPECT_TRUE(tsl.entries[2].remotepath.empty());
    EXPECT_EQ(tsl.entries[3].remotepath, "/remote/path2");

    EXPECT_TRUE(tsl.entries[0].remotehost.empty());
    EXPECT_EQ(tsl.entries[1].remotehost, "Host1");
    EXPECT_TRUE(tsl.entries[2].remotehost.empty());
    EXPECT_EQ(tsl.entries[3].remotehost, "Host2");

    EXPECT_TRUE(tsl.entries[0].uuid.empty());
    EXPECT_EQ(tsl.entries[1].uuid, "0123456789abcdef0123456879abcdef");
    EXPECT_TRUE(tsl.entries[2].uuid.empty());
    EXPECT_EQ(tsl.entries[3].uuid, "abcdef0123456879abcdef0123456789");
}

int main(int argc, char **argv)
{
    int result;
    ::testing::InitGoogleTest(&argc, argv);
    result = RUN_ALL_TESTS();
    return result;
}
