/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TestAWSSDKTransport.cpp - Unit tests for AWS SDK S3 transport
 *
 * These tests require an S3-compatible endpoint (e.g., MinIO) running locally.
 * They are disabled by default and must be run manually:
 *
 *   # Start MinIO first:
 *   minio server /tmp/minio-data --console-address ":9001"
 *   mc alias set local http://localhost:9000 minioadmin minioadmin
 *   mc mb local/testbucket
 *
 *   # Run tests:
 *   ./bin/Test.Unit.AWSSDKTransport.Serial --gtest_also_run_disabled_tests
 *
 * Environment variables:
 *   ADIOS2_S3_ENDPOINT - S3 endpoint URL (default: http://localhost:9000)
 */

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

#include <adios2/common/ADIOSTypes.h>
#include <adios2/helper/adiosCommDummy.h>
#include <adios2/toolkit/transport/file/FileAWSSDK.h>

#include <aws/core/Aws.h>

#include <gtest/gtest.h>

namespace adios2
{
namespace unit
{

class AWSSDKTransportTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const char *endpoint = std::getenv("ADIOS2_S3_ENDPOINT");
        m_Endpoint = endpoint ? endpoint : "http://localhost:9000";
    }

    std::string m_Endpoint;
};

// Test basic write and read with S3 transport
TEST_F(AWSSDKTransportTest, DISABLED_WriteReadBasic)
{
    const std::string objectPath = "testbucket/test_basic.dat";
    constexpr size_t dataSize = 100;

    // Create test data with known pattern
    std::vector<uint8_t> dataWrite(dataSize);
    for (size_t i = 0; i < dataSize; ++i)
    {
        dataWrite[i] = static_cast<uint8_t>(i);
    }

    helper::Comm comm = helper::CommDummy();

    // Write
    {
        auto transport = std::make_unique<transport::FileAWSSDK>(comm);
        Params params;
        params["endpoint"] = m_Endpoint;
        transport->SetParameters(params);

        transport->Open(objectPath, Mode::Write);
        transport->Write(reinterpret_cast<char *>(dataWrite.data()), dataWrite.size());
        transport->Close();
    }

    // Read and verify
    std::vector<uint8_t> dataRead(dataSize);
    {
        auto transport = std::make_unique<transport::FileAWSSDK>(comm);
        Params params;
        params["endpoint"] = m_Endpoint;
        transport->SetParameters(params);

        transport->Open(objectPath, Mode::Read);

        size_t fileSize = transport->GetSize();
        ASSERT_EQ(fileSize, dataSize) << "File size mismatch";

        transport->Read(reinterpret_cast<char *>(dataRead.data()), dataRead.size());
        transport->Close();
    }

    // Verify all bytes
    for (size_t i = 0; i < dataSize; ++i)
    {
        EXPECT_EQ(dataWrite[i], dataRead[i]) << "Mismatch at byte " << i;
    }
}

// Test multipart upload with large data (>5MB minimum part size)
TEST_F(AWSSDKTransportTest, DISABLED_WriteReadLarge)
{
    const std::string objectPath = "testbucket/test_large.dat";
    constexpr size_t dataSize = 15 * 1024 * 1024; // 15 MB - triggers multipart upload

    // Create test data with known pattern
    std::vector<uint8_t> dataWrite(dataSize);
    for (size_t i = 0; i < dataSize; ++i)
    {
        dataWrite[i] = static_cast<uint8_t>((i * 7) % 256);
    }

    helper::Comm comm = helper::CommDummy();

    // Write
    {
        auto transport = std::make_unique<transport::FileAWSSDK>(comm);
        Params params;
        params["endpoint"] = m_Endpoint;
        params["min_part_size"] = "5mb";
        transport->SetParameters(params);

        transport->Open(objectPath, Mode::Write);
        transport->Write(reinterpret_cast<char *>(dataWrite.data()), dataWrite.size());
        transport->Close();
    }

    // Read and verify
    std::vector<uint8_t> dataRead(dataSize);
    {
        auto transport = std::make_unique<transport::FileAWSSDK>(comm);
        Params params;
        params["endpoint"] = m_Endpoint;
        transport->SetParameters(params);

        transport->Open(objectPath, Mode::Read);

        size_t fileSize = transport->GetSize();
        ASSERT_EQ(fileSize, dataSize) << "File size mismatch";

        transport->Read(reinterpret_cast<char *>(dataRead.data()), dataRead.size());
        transport->Close();
    }

    // Verify all bytes
    for (size_t i = 0; i < dataSize; ++i)
    {
        EXPECT_EQ(dataWrite[i], dataRead[i]) << "Mismatch at byte " << i;
    }
}

// Test many small writes that accumulate in buffer before upload
// This exercises the buffer accumulation logic
TEST_F(AWSSDKTransportTest, DISABLED_ManySmallWrites)
{
    const std::string objectPath = "testbucket/test_manysmall.dat";
    constexpr size_t writeSize = 100 * 1024; // 100 KB per write
    constexpr size_t numWrites = 120;        // 12 MB total (triggers 2 part uploads)
    constexpr size_t totalSize = writeSize * numWrites;

    // Create test data with unique pattern
    std::vector<uint8_t> dataWrite(totalSize);
    for (size_t i = 0; i < totalSize; ++i)
    {
        dataWrite[i] = static_cast<uint8_t>((i * 13 + i / 1000) % 256);
    }

    helper::Comm comm = helper::CommDummy();

    // Write in many small chunks - buffer should accumulate to 5MB then upload
    {
        auto transport = std::make_unique<transport::FileAWSSDK>(comm);
        Params params;
        params["endpoint"] = m_Endpoint;
        params["min_part_size"] = "5mb";
        transport->SetParameters(params);

        transport->Open(objectPath, Mode::Write);

        for (size_t i = 0; i < numWrites; ++i)
        {
            transport->Write(reinterpret_cast<char *>(dataWrite.data() + i * writeSize), writeSize);
        }

        transport->Close();
    }

    // Read and verify
    std::vector<uint8_t> dataRead(totalSize);
    {
        auto transport = std::make_unique<transport::FileAWSSDK>(comm);
        Params params;
        params["endpoint"] = m_Endpoint;
        transport->SetParameters(params);

        transport->Open(objectPath, Mode::Read);

        size_t fileSize = transport->GetSize();
        ASSERT_EQ(fileSize, totalSize) << "File size mismatch";

        transport->Read(reinterpret_cast<char *>(dataRead.data()), dataRead.size());
        transport->Close();
    }

    // Verify all bytes
    for (size_t i = 0; i < totalSize; ++i)
    {
        EXPECT_EQ(dataWrite[i], dataRead[i]) << "Mismatch at byte " << i;
    }
}

// Test mixed size writes - alternating small and large
// This exercises buffer filling + direct upload paths
TEST_F(AWSSDKTransportTest, DISABLED_MixedSizeWrites)
{
    const std::string objectPath = "testbucket/test_mixed.dat";

    // Pattern: small, large, small, large, small
    // 1MB + 8MB + 500KB + 6MB + 2MB = 17.5 MB
    const std::vector<size_t> writeSizes = {
        1 * 1024 * 1024, // 1 MB - goes to buffer
        8 * 1024 * 1024, // 8 MB - fills buffer, uploads part, remainder to buffer
        500 * 1024,      // 500 KB - accumulates in buffer
        6 * 1024 * 1024, // 6 MB - triggers upload
        2 * 1024 * 1024  // 2 MB - remainder in buffer, flushed on close
    };

    size_t totalSize = 0;
    for (auto s : writeSizes)
    {
        totalSize += s;
    }

    // Create test data
    std::vector<uint8_t> dataWrite(totalSize);
    for (size_t i = 0; i < totalSize; ++i)
    {
        dataWrite[i] = static_cast<uint8_t>((i * 7 + i / 10000) % 256);
    }

    helper::Comm comm = helper::CommDummy();

    // Write with mixed sizes
    {
        auto transport = std::make_unique<transport::FileAWSSDK>(comm);
        Params params;
        params["endpoint"] = m_Endpoint;
        params["min_part_size"] = "5mb";
        transport->SetParameters(params);

        transport->Open(objectPath, Mode::Write);

        size_t offset = 0;
        for (size_t writeSize : writeSizes)
        {
            transport->Write(reinterpret_cast<char *>(dataWrite.data() + offset), writeSize);
            offset += writeSize;
        }

        transport->Close();
    }

    // Read and verify
    std::vector<uint8_t> dataRead(totalSize);
    {
        auto transport = std::make_unique<transport::FileAWSSDK>(comm);
        Params params;
        params["endpoint"] = m_Endpoint;
        transport->SetParameters(params);

        transport->Open(objectPath, Mode::Read);

        size_t fileSize = transport->GetSize();
        ASSERT_EQ(fileSize, totalSize) << "File size mismatch";

        transport->Read(reinterpret_cast<char *>(dataRead.data()), dataRead.size());
        transport->Close();
    }

    // Verify all bytes
    for (size_t i = 0; i < totalSize; ++i)
    {
        EXPECT_EQ(dataWrite[i], dataRead[i]) << "Mismatch at byte " << i;
    }
}

// Test writes at exact part size boundaries
TEST_F(AWSSDKTransportTest, DISABLED_BoundaryWrites)
{
    const std::string objectPath = "testbucket/test_boundary.dat";
    constexpr size_t partSize = 5 * 1024 * 1024; // 5 MB part size

    // Write exactly 3 parts worth of data in various boundary-hitting patterns
    constexpr size_t totalSize = partSize * 3; // 15 MB

    std::vector<uint8_t> dataWrite(totalSize);
    for (size_t i = 0; i < totalSize; ++i)
    {
        dataWrite[i] = static_cast<uint8_t>((i * 11) % 256);
    }

    helper::Comm comm = helper::CommDummy();

    // Write in chunks that hit boundaries:
    // - Exactly partSize (fills buffer exactly, triggers upload)
    // - partSize - 1 byte (just under, stays in buffer)
    // - 1 byte (completes the part)
    // - partSize + 1 (overflows by 1 byte)
    // - remaining
    {
        auto transport = std::make_unique<transport::FileAWSSDK>(comm);
        Params params;
        params["endpoint"] = m_Endpoint;
        params["min_part_size"] = "5mb";
        transport->SetParameters(params);

        transport->Open(objectPath, Mode::Write);

        size_t offset = 0;

        // Write exactly one part
        transport->Write(reinterpret_cast<char *>(dataWrite.data() + offset), partSize);
        offset += partSize;

        // Write part - 1 byte (stays in buffer)
        transport->Write(reinterpret_cast<char *>(dataWrite.data() + offset), partSize - 1);
        offset += partSize - 1;

        // Write 1 byte (completes the buffered part)
        transport->Write(reinterpret_cast<char *>(dataWrite.data() + offset), 1);
        offset += 1;

        // Write remaining
        transport->Write(reinterpret_cast<char *>(dataWrite.data() + offset), totalSize - offset);

        transport->Close();
    }

    // Read and verify
    std::vector<uint8_t> dataRead(totalSize);
    {
        auto transport = std::make_unique<transport::FileAWSSDK>(comm);
        Params params;
        params["endpoint"] = m_Endpoint;
        transport->SetParameters(params);

        transport->Open(objectPath, Mode::Read);

        size_t fileSize = transport->GetSize();
        ASSERT_EQ(fileSize, totalSize) << "File size mismatch";

        transport->Read(reinterpret_cast<char *>(dataRead.data()), dataRead.size());
        transport->Close();
    }

    for (size_t i = 0; i < totalSize; ++i)
    {
        EXPECT_EQ(dataWrite[i], dataRead[i]) << "Mismatch at byte " << i;
    }
}

// Test very small writes (1 KB each) - stress test buffer accumulation
TEST_F(AWSSDKTransportTest, DISABLED_VerySmallWrites)
{
    const std::string objectPath = "testbucket/test_verysmall.dat";
    constexpr size_t writeSize = 1024; // 1 KB per write
    constexpr size_t numWrites = 6000; // 6 MB total
    constexpr size_t totalSize = writeSize * numWrites;

    std::vector<uint8_t> dataWrite(totalSize);
    for (size_t i = 0; i < totalSize; ++i)
    {
        dataWrite[i] = static_cast<uint8_t>((i * 17 + i / 100) % 256);
    }

    helper::Comm comm = helper::CommDummy();

    {
        auto transport = std::make_unique<transport::FileAWSSDK>(comm);
        Params params;
        params["endpoint"] = m_Endpoint;
        params["min_part_size"] = "5mb";
        transport->SetParameters(params);

        transport->Open(objectPath, Mode::Write);

        for (size_t i = 0; i < numWrites; ++i)
        {
            transport->Write(reinterpret_cast<char *>(dataWrite.data() + i * writeSize), writeSize);
        }

        transport->Close();
    }

    std::vector<uint8_t> dataRead(totalSize);
    {
        auto transport = std::make_unique<transport::FileAWSSDK>(comm);
        Params params;
        params["endpoint"] = m_Endpoint;
        transport->SetParameters(params);

        transport->Open(objectPath, Mode::Read);

        size_t fileSize = transport->GetSize();
        ASSERT_EQ(fileSize, totalSize) << "File size mismatch";

        transport->Read(reinterpret_cast<char *>(dataRead.data()), dataRead.size());
        transport->Close();
    }

    for (size_t i = 0; i < totalSize; ++i)
    {
        EXPECT_EQ(dataWrite[i], dataRead[i]) << "Mismatch at byte " << i;
    }
}

// Test configurable part sizes
TEST_F(AWSSDKTransportTest, DISABLED_PartSizeParameter)
{
    const std::string objectPath = "testbucket/test_partsize.dat";
    constexpr size_t dataSize = 20 * 1024 * 1024; // 20 MB

    // Create test data
    std::vector<uint8_t> dataWrite(dataSize);
    for (size_t i = 0; i < dataSize; ++i)
    {
        dataWrite[i] = static_cast<uint8_t>((i * 17) % 256);
    }

    helper::Comm comm = helper::CommDummy();

    // Write with 6MB part size (should create multiple parts)
    {
        auto transport = std::make_unique<transport::FileAWSSDK>(comm);
        Params params;
        params["endpoint"] = m_Endpoint;
        params["min_part_size"] = "6mb";
        transport->SetParameters(params);

        transport->Open(objectPath, Mode::Write);
        transport->Write(reinterpret_cast<char *>(dataWrite.data()), dataWrite.size());
        transport->Close();
    }

    // Read and verify
    std::vector<uint8_t> dataRead(dataSize);
    {
        auto transport = std::make_unique<transport::FileAWSSDK>(comm);
        Params params;
        params["endpoint"] = m_Endpoint;
        transport->SetParameters(params);

        transport->Open(objectPath, Mode::Read);

        size_t fileSize = transport->GetSize();
        ASSERT_EQ(fileSize, dataSize) << "File size mismatch";

        transport->Read(reinterpret_cast<char *>(dataRead.data()), dataRead.size());
        transport->Close();
    }

    // Verify all bytes
    for (size_t i = 0; i < dataSize; ++i)
    {
        EXPECT_EQ(dataWrite[i], dataRead[i]) << "Mismatch at byte " << i;
    }
}

} // namespace unit
} // namespace adios2

int main(int argc, char **argv)
{
    // Initialize AWS SDK before running tests
    Aws::SDKOptions options;
    Aws::InitAPI(options);

    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();

    // Shutdown AWS SDK after tests complete
    Aws::ShutdownAPI(options);

    return result;
}
