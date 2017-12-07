#include <cstdint>

#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#define str_helper(X) #X
#define str(X) str_helper(X)

class XMLConfigTest : public ::testing::Test
{
public:
    XMLConfigTest() : configDir(str(XML_CONFIG_DIR)) {}

    // protected:
    // virtual void SetUp() { }

    // virtual void TearDown() { }
    std::string configDir;
};

TEST_F(XMLConfigTest, TwoIOs)
{
    const std::string configFile(
        configDir + std::string(&adios2::PathSeparator, 1) + "config1.xml");

#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(configFile, MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(configFile, adios2::DebugON);
#endif

    // must be declared at least once
    EXPECT_EQ(adios.InquireIO("Test IO 1"), nullptr);

    EXPECT_NO_THROW({
        adios2::IO &io = adios.DeclareIO("Test IO 1");
        const adios2::Params &params = io.GetParameters();
        ASSERT_EQ(params.size(), 5);
        EXPECT_THROW(params.at("DoesNotExist"), std::out_of_range);
        EXPECT_EQ(params.at("Threads"), "1");
        EXPECT_EQ(params.at("ProfileUnits"), "Microseconds");
        EXPECT_EQ(params.at("MaxBufferSize"), "20Mb");
        EXPECT_EQ(params.at("InitialBufferSize"), "1Mb");
        EXPECT_EQ(params.at("BufferGrowthFactor"), "2");
        adios2::Engine &engine =
            io.Open("Test BP Writer 1", adios2::Mode::Write);
        engine.Close();

        EXPECT_NE(adios.InquireIO("Test IO 1"), nullptr);
    });

    EXPECT_EQ(adios.InquireIO("Test IO 2"), nullptr);
    EXPECT_NO_THROW({
        adios2::IO &io = adios.DeclareIO("Test IO 2");
        const adios2::Params &params = io.GetParameters();
        ASSERT_EQ(params.size(), 0);
        EXPECT_NE(adios.InquireIO("Test IO 2"), nullptr);
    });

    // double declaring
    EXPECT_THROW(adios.DeclareIO("Test IO 1"), std::invalid_argument);
    EXPECT_THROW(adios.DeclareIO("Test IO 2"), std::invalid_argument);
}

TEST_F(XMLConfigTest, TwoEnginesException)
{
    const std::string configFile(
        configDir + std::string(&adios2::PathSeparator, 1) + "config2.xml");

#ifdef ADIOS2_HAVE_MPI
    EXPECT_THROW(
        adios2::ADIOS adios(configFile, MPI_COMM_WORLD, adios2::DebugON),
        std::invalid_argument);
#else
    EXPECT_THROW(adios2::ADIOS adios(configFile, adios2::DebugON),
                 std::invalid_argument);
#endif
}

int main(int argc, char **argv)
{
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(nullptr, nullptr);
#endif

    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
