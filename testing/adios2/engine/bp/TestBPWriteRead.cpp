#include <cstdint>

#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#include "../EngineWriteReadTestBase.h"

class BPWriteReadTest : public EngineWriteReadTestBase
{
public:
    BPWriteReadTest() : EngineWriteReadTestBase() {}

protected:
    virtual void OpenWrite(std::string fname)
    {
        auto &m_EngineSettings = m_adios.DeclareMethod("TestMethod");
        m_EngineSettings.SetEngine("BPFileWriter");
        m_EngineSettings.SetParameters("profile_units=mus");
        m_EngineSettings.AddTransport("File", "profile_units=mus",
                                      "have_metadata_file=no");

        m_Engine = m_adios.Open(fname, "w", m_EngineSettings);
        ASSERT_NE(m_Engine, nullptr);
    }
};

TEST_F(BPWriteReadTest, WriteRead_1D_8)
{
    EngineWriteReadTestBase::Declare1D_8();
    EngineWriteReadTestBase::OpenWriteClose("BPWriteReadTest_1D_8.bp");
}

TEST_F(BPWriteReadTest, WriteRead_2D_2x4)
{
    EngineWriteReadTestBase::Declare2D_2x4();
    EngineWriteReadTestBase::OpenWriteClose("BPWriteReadTest_2D_2x4.bp");
}

TEST_F(BPWriteReadTest, WriteRead_2D_4x2)
{
    EngineWriteReadTestBase::Declare2D_4x2();
    EngineWriteReadTestBase::OpenWriteClose("BPWriteReadTest_2D_4x2.bp");
}
