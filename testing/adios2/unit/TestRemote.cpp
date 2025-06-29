#include <adios2/helper/adiosType.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>

#include "adios2/helper/adiosLog.h"
#include "adios2/helper/adiosString.h"
#include <adios2.h>
#include <adios2/common/ADIOSTypes.h>
#include <adios2/core/IO.h>
#include <adios2/helper/adiosCommDummy.h>
#include <adios2/toolkit/remote/EVPathRemote.h>
#include <adios2/toolkit/transportman/TransportMan.h>
#include <gtest/gtest.h>
namespace adios2
{
namespace helper
{

// Assuming CoreDims and DimsArray definitions are included here
using namespace adios2::helper;

TEST(Remote, OpenRead)
{

#define FILE_STRING "Writing this to a file."
#define FNAME "/tmp/TestFile.txt"
    std::ofstream ofile;
    ofile.open(FNAME);
    ofile << FILE_STRING;
    ofile.close();

    adios2::HostOptions hostOptions;
    int localPort = 26200;
    std::vector<char> contents;
    {
        std::unique_ptr<Remote> remote = nullptr;

        remote = std::unique_ptr<EVPathRemote>(new EVPathRemote(hostOptions));
        remote->OpenReadSimpleFile("localhost", localPort, FNAME, contents);
        std::cout << "Contents size was " << contents.size() << std::endl;
        ASSERT_EQ(contents.size(), strlen(FILE_STRING));
        ASSERT_EQ(0, memcmp(FILE_STRING, contents.data(), contents.size()));

        // OpenReadSimple doesn't leave the file open
        EXPECT_THROW(remote->Read(0, 1, contents.data()), std::invalid_argument);
    }

    {
        std::unique_ptr<Remote> remote = nullptr;
        remote = std::unique_ptr<EVPathRemote>(new EVPathRemote(hostOptions));
        remote->OpenSimpleFile("localhost", localPort, FNAME);
        std::cout << "Contents size is " << remote->m_Size << std::endl;
        contents.resize(remote->m_Size); // should be unnecessary
        remote->Read(0, remote->m_Size, contents.data());
        ASSERT_EQ(0, memcmp(FILE_STRING, contents.data(), contents.size()));
        remote->Close();

        // Can't read a closed file
        EXPECT_THROW(remote->Read(0, 1, contents.data()), std::invalid_argument);
    }
}
}
}

int main(int argc, char **argv)
{

    int result;
    ::testing::InitGoogleTest(&argc, argv);
    result = RUN_ALL_TESTS();

    return result;
}
