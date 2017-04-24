#include "EngineWriteReadTestBase.h"

#include <cstdint>

#include <array>
#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

EngineWriteReadTestBase::EngineWriteReadTestBase()
: m_adios(adios::Verbose::WARN, true)
{
}

void EngineWriteReadTestBase::Declare1D_8()
{
    auto &var_i8 = m_adios.DefineVariable<char>("i8", adios::Dims{8});
    auto &var_i16 = m_adios.DefineVariable<short>("i16", adios::Dims{8});
    auto &var_i32 = m_adios.DefineVariable<int>("i32", adios::Dims{8});
    auto &var_i64 = m_adios.DefineVariable<long>("i64", adios::Dims{8});
    auto &var_u8 = m_adios.DefineVariable<unsigned char>("u8", adios::Dims{8});
    auto &var_u16 =
        m_adios.DefineVariable<unsigned short>("u16", adios::Dims{8});
    auto &var_u32 = m_adios.DefineVariable<unsigned int>("u32", adios::Dims{8});
    auto &var_u64 =
        m_adios.DefineVariable<unsigned long>("u64", adios::Dims{8});
    auto &var_r32 = m_adios.DefineVariable<float>("r32", adios::Dims{8});
    auto &var_r64 = m_adios.DefineVariable<double>("r64", adios::Dims{8});
}

void EngineWriteReadTestBase::Declare2D_2x4()
{
    auto &var_i8 = m_adios.DefineVariable<char>("i8", adios::Dims{2, 4});
    auto &var_i16 = m_adios.DefineVariable<short>("i16", adios::Dims{2, 4});
    auto &var_i32 = m_adios.DefineVariable<int>("i32", adios::Dims{2, 4});
    auto &var_i64 = m_adios.DefineVariable<long>("i64", adios::Dims{2, 4});
    auto &var_u8 =
        m_adios.DefineVariable<unsigned char>("u8", adios::Dims{2, 4});
    auto &var_u16 =
        m_adios.DefineVariable<unsigned short>("u16", adios::Dims{2, 4});
    auto &var_u32 =
        m_adios.DefineVariable<unsigned int>("u32", adios::Dims{2, 4});
    auto &var_u64 =
        m_adios.DefineVariable<unsigned long>("u64", adios::Dims{2, 4});
    auto &var_r32 = m_adios.DefineVariable<float>("r32", adios::Dims{2, 4});
    auto &var_r64 = m_adios.DefineVariable<double>("r64", adios::Dims{2, 4});
}

void EngineWriteReadTestBase::Declare2D_4x2()
{
    auto &var_i8 = m_adios.DefineVariable<char>("i8", adios::Dims{4, 2});
    auto &var_i16 = m_adios.DefineVariable<short>("i16", adios::Dims{4, 2});
    auto &var_i32 = m_adios.DefineVariable<int>("i32", adios::Dims{4, 2});
    auto &var_i64 = m_adios.DefineVariable<long>("i64", adios::Dims{4, 2});
    auto &var_u8 =
        m_adios.DefineVariable<unsigned char>("u8", adios::Dims{4, 2});
    auto &var_u16 =
        m_adios.DefineVariable<unsigned short>("u16", adios::Dims{4, 2});
    auto &var_u32 =
        m_adios.DefineVariable<unsigned int>("u32", adios::Dims{4, 2});
    auto &var_u64 =
        m_adios.DefineVariable<unsigned long>("u64", adios::Dims{4, 2});
    auto &var_r32 = m_adios.DefineVariable<float>("r32", adios::Dims{4, 2});
    auto &var_r64 = m_adios.DefineVariable<double>("r64", adios::Dims{4, 2});
}

void EngineWriteReadTestBase::WriteStep(size_t step)
{
    auto &var_i8 = m_adios.GetVariable<char>("i8");
    auto &var_i16 = m_adios.GetVariable<short>("i16");
    auto &var_i32 = m_adios.GetVariable<int>("i32");
    auto &var_i64 = m_adios.GetVariable<long>("i64");
    auto &var_u8 = m_adios.GetVariable<unsigned char>("u8");
    auto &var_u16 = m_adios.GetVariable<unsigned short>("u16");
    auto &var_u32 = m_adios.GetVariable<unsigned int>("u32");
    auto &var_u64 = m_adios.GetVariable<unsigned long>("u64");
    auto &var_r32 = m_adios.GetVariable<float>("r32");
    auto &var_r64 = m_adios.GetVariable<double>("r64");

    m_Engine->Write(var_i8, m_DataI8.cbegin() + step);
    m_Engine->Write(var_i16, m_DataI16.cbegin() + step);
    m_Engine->Write(var_i32, m_DataI32.cbegin() + step);
    m_Engine->Write(var_i64, m_DataI64.cbegin() + step);
    m_Engine->Write(var_u8, m_DataU8.cbegin() + step);
    m_Engine->Write(var_u16, m_DataU16.cbegin() + step);
    m_Engine->Write(var_u32, m_DataU32.cbegin() + step);
    m_Engine->Write(var_u64, m_DataU64.cbegin() + step);
    m_Engine->Write(var_r32, m_DataR32.cbegin() + step);
    m_Engine->Write(var_r64, m_DataR64.cbegin() + step);
}

void EngineWriteReadTestBase::OpenWriteClose(std::string fname)
{
    OpenWrite(fname);

    for (size_t t = 0; t < 3; ++t)
    {
        EngineWriteReadTestBase::WriteStep(t);
        m_Engine->Advance();
    }

    m_Engine->Close();
}
