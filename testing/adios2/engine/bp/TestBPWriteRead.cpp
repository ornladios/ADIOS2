#include <cstdint>

#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#include "../EngineWriteReadTest.h"

class BPWriteRead1DTest : public EngineWriteRead1DTest
{
public:
    BPWriteRead1DTest()
    : EngineWriteRead1DTest("BPFileWriter", "BPWriteRead1DTest.bp")
    {
    }
};

TEST_F(BPWriteRead1DTest, WriteReadTest) { WriteRead(); }

//******************************************************************************
// 2D 2x4 test data
//******************************************************************************

class BPWriteRead2D2x4Test : public EngineWriteRead2D2x4Test
{
public:
    BPWriteRead2D2x4Test()
    : EngineWriteRead2D2x4Test("BPFileWriter", "BPWriteRead2D2x4Test.bp")
    {
    }
};

TEST_F(BPWriteRead2D2x4Test, WriteReadTest) { WriteRead(); }

//******************************************************************************
// 2D 4x2 test data
//******************************************************************************

class BPWriteRead2D4x2Test : public EngineWriteRead2D4x2Test
{
public:
    BPWriteRead2D4x2Test()
    : EngineWriteRead2D4x2Test("BPFileWriter", "BPWriteRead2D4x2Test.bp")
    {
    }
};

TEST_F(BPWriteRead2D4x2Test, WriteReadTest) { WriteRead(); }
