/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 */

#include <gtest/gtest.h>
#include <ios>       //std::ios_base::failure
#include <iostream>  //std::cout
#include <stdexcept> //std::invalid_argument std::exception
#include <string>
#include <vector>

#include <adios2.h>

class AttributeReadTest : public ::testing::Test
{
public:
    AttributeReadTest() = default;
};

std::string fname;

// ADIOS2 Common read
TEST_F(AttributeReadTest, ADIOS2AttributeRead)
{
    adios2::ADIOS adios;

    /*** IO class object: settings and factory of Settings: Variables,
     * Parameters, Transports, and Execution: Engines */
    adios2::IO bpIO = adios.DeclareIO("BPFile_N2N");

    adios2::Engine bpReader = bpIO.Open(fname, adios2::Mode::ReadRandomAccess);
    EXPECT_TRUE(bpReader);

    std::vector<float> myFloats;
    adios2::Variable<float> bpFloats = bpIO.InquireVariable<float>("bpFloats");
    EXPECT_TRUE(bpFloats);

    bpReader.Get<float>(bpFloats, myFloats, adios2::Mode::Sync);

    EXPECT_GT(myFloats.size(), 9); // at least one MPI rank

    for (size_t i = 0; i < myFloats.size(); i++)
    {
        EXPECT_FLOAT_EQ(myFloats[i], (float)i);
    }
    const auto attributesInfo = bpIO.AvailableAttributes();

    EXPECT_EQ(attributesInfo.size(), 4);

    auto StrAtt = bpIO.InquireAttribute<std::string>("Single_String");
    EXPECT_TRUE(StrAtt);

    auto StrArray = StrAtt.Data();
    EXPECT_EQ(StrArray.size(), 1);
    EXPECT_STREQ(StrArray[0].c_str(), "File generated with ADIOS2");

    auto StrAtt2 = bpIO.InquireAttribute<std::string>("Array_of_Strings");
    EXPECT_TRUE(StrAtt2);

    auto StrArray2 = StrAtt2.Data();
    EXPECT_EQ(StrArray2.size(), 3);
    EXPECT_STREQ(StrArray2[0].c_str(), "one");
    EXPECT_STREQ(StrArray2[1].c_str(), "two");
    EXPECT_STREQ(StrArray2[2].c_str(), "three");

    auto DblAtt2 = bpIO.InquireAttribute<double>("Array_of_Doubles");
    EXPECT_TRUE(DblAtt2);

    auto DblArray2 = DblAtt2.Data();
    EXPECT_EQ(DblArray2.size(), 10);
    for (size_t i = 0; i < 10; i++)
    {
        EXPECT_DOUBLE_EQ(DblArray2[i], (double)i + 1);
    }

    auto DblAtt = bpIO.InquireAttribute<double>("Attr_Double");
    EXPECT_TRUE(DblAtt);

    auto DblArray = DblAtt.Data();
    EXPECT_EQ(DblArray.size(), 1);
    EXPECT_DOUBLE_EQ(DblArray[0], 0.0);

#ifdef VERBOSE_OUTPUT
    for (const auto &attributeInfoPair : attributesInfo)
    {
        std::cout << "Attribute: " << attributeInfoPair.first;
        for (const auto &attributePair : attributeInfoPair.second)
        {
            std::cout << "\tKey: " << attributePair.first << "\tValue: " << attributePair.second
                      << "\n";
        }
        std::cout << "\n";
    }
#endif
    bpReader.Close();
}

int main(int argc, char *argv[])
{
    int result;
    if (argc > 1)
    {
        fname = std::string(argv[1]);
    }
    ::testing::InitGoogleTest(&argc, argv);

    result = RUN_ALL_TESTS();

    return result;
}
