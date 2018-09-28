/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <cstdint>
#include <cstring>

#include <iostream>
#include <stdexcept>

#include <adios2.h>
#include <adios2/ADIOSTypes.h>
#include <adios2/helper/adiosString.h>

#include <gtest/gtest.h>

TEST(ADIOS2HelperString, ADIOS2HelperStringFNF)
{

    const std::string fname("nosuchfile.txt");
    const std::string hint("");

    ASSERT_THROW(adios2::helper::FileToString(fname, hint),
                 std::ios_base::failure);
}

TEST(ADIOS2HelperString, ADIOS2HelperStringParameterMap)
{

    const bool debugMode = true;
    const std::vector<std::string> badparam_in = {"badparam"};
    const std::vector<std::string> emptyparam_in = {"emptyparam="};
    const std::vector<std::string> dupparam_in = {"dupparam=1", "dupparam=2"};
    const std::vector<std::string> param_in = {"param1=1", "param2=2",
                                               "param3=3"};

    adios2::Params parameters =
        adios2::helper::BuildParametersMap(param_in, debugMode);

    ASSERT_THROW(adios2::helper::BuildParametersMap(badparam_in, debugMode),
                 std::invalid_argument);
    ASSERT_THROW(adios2::helper::BuildParametersMap(emptyparam_in, debugMode),
                 std::invalid_argument);
    ASSERT_THROW(adios2::helper::BuildParametersMap(dupparam_in, debugMode),
                 std::invalid_argument);

    ASSERT_EQ(parameters.find("param1")->second, "1");
    ASSERT_EQ(parameters.find("param2")->second, "2");
    ASSERT_EQ(parameters.find("param3")->second, "3");
}

TEST(ADIOS2HelperString, ADIOS2HelperStringAddExtension)
{

    const std::string abc("abc");
    const std::string abcbp("abc.bp");
    const std::string ext(".bp");

    ASSERT_EQ(adios2::helper::AddExtension(abc, ext), abcbp);
    ASSERT_EQ(adios2::helper::AddExtension(abcbp, ext), abcbp);
}

TEST(ADIOS2HelperString, ADIOS2HelperStringConversion)
{

    const bool debugMode = true;
    const std::string dbl("123.1230");
    const std::string uint("123");
    const std::string notnum("notnum");
    const std::string hint("");

    ASSERT_EQ(adios2::helper::StringToDouble(dbl, debugMode, hint), 123.123);
    ASSERT_THROW(adios2::helper::StringToDouble(notnum, debugMode, hint),
                 std::invalid_argument);
    ASSERT_EQ(adios2::helper::StringToUInt(uint, debugMode, hint), 123);
    ASSERT_THROW(adios2::helper::StringToUInt(notnum, debugMode, hint),
                 std::invalid_argument);
}

TEST(ADIOS2HelperString, ADIOS2HelperDimString)
{

    const adios2::Dims dimensions = {1, 2, 3};
    const std::string dimstr("Dims(3):[1, 2, 3]");

    ASSERT_EQ(adios2::helper::DimsToString(dimensions), dimstr);
}

TEST(ADIOS2HelperString, ADIOS2HelperGlobalName)
{

    const std::string localName("myfile.bp");
    const std::string prefix("mydir");
    const std::string separator("/");
    const std::string global("mydir/myfile.bp");

    ASSERT_EQ(adios2::helper::GlobalName(localName, prefix, separator), global);
}

int main(int argc, char **argv)
{

    int result;
    ::testing::InitGoogleTest(&argc, argv);
    result = RUN_ALL_TESTS();

    return result;
}
