/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <adios2/core/ADIOS.h>
#include <adios2/core/Engine.h>
#include <adios2/core/IO.h>
#include <adios2/helper/adiosCommDummy.h>

#include <algorithm>
#include <exception>
#include <thread>
#include <type_traits>

#include <gtest/gtest.h>

using namespace adios2;

static_assert(!std::is_base_of<core::IO, core::DefinitionBatch>::value,
              "DefinitionBatch must be independent of IO");

TEST(CoreIODefinitions, ParallelBuildAndMerge)
{
    core::ADIOS adios("C++");
    auto &io = adios.DeclareIO("definitions");
    constexpr size_t workers = 4;
    constexpr size_t count = 128;
    std::vector<std::unique_ptr<core::DefinitionBatch>> batches;
    for (size_t i = 0; i < workers; ++i)
        batches.push_back(io.CreateDefinitionBatch(count));
    std::vector<core::Variable<int> *> pointers(workers);
    std::vector<std::exception_ptr> errors(workers);
    std::vector<std::thread> threads;
    for (size_t i = 0; i < workers; ++i)
    {
        threads.emplace_back([&, i] {
            try
            {
                auto &batch = *batches[i];
                for (size_t j = 0; j < count; ++j)
                {
                    const auto name = std::to_string(i) + "/" + std::to_string(j);
                    auto &variable = batch.DefineVariable<int>(name, {8}, {0}, {8});
                    batch.DefineAttribute<int>("units", static_cast<int>(j), name);
                    if (j == 0)
                        pointers[i] = &variable;
                }
            }
            catch (...)
            {
                errors[i] = std::current_exception();
            }
        });
    }
    for (auto &thread : threads)
        thread.join();
    EXPECT_TRUE(io.GetVariables().empty());
    for (size_t i = 0; i < workers; ++i)
    {
        if (errors[i])
            std::rethrow_exception(errors[i]);
        io.MergeDefinitions(*batches[i]);
        EXPECT_TRUE(batches[i]->Empty());
        EXPECT_EQ(batches[i]->InquireAttribute<int>(std::to_string(i) + "/0/units"), nullptr);
    }
    io.FinalizeDefinitions();
    // Destroy the batches, then verify ownership, addresses and associated values.
    batches.clear();
    EXPECT_EQ(io.GetVariables().size(), workers * count);
    for (size_t i = 0; i < workers; ++i)
    {
        EXPECT_EQ(io.InquireVariable<int>(std::to_string(i) + "/0"), pointers[i]);
        for (size_t j = 0; j < count; ++j)
        {
            const auto name = std::to_string(i) + "/" + std::to_string(j);
            auto *attribute = io.InquireAttribute<int>("units", name);
            ASSERT_NE(attribute, nullptr);
            EXPECT_EQ(attribute->m_DataSingleValue, static_cast<int>(j));
        }
    }
}

TEST(CoreIODefinitions, DuplicateVariableLeavesBatchIntact)
{
    core::ADIOS adios("C++");
    auto &io = adios.DeclareIO("definitions");
    auto &existing = io.DefineVariable<int>("duplicate");
    auto batch = io.CreateDefinitionBatch();
    batch->DefineVariable<int>("duplicate");
    batch->DefineVariable<int>("new");
    batch->DefineAttribute<int>("attribute", 1);
    EXPECT_THROW(io.MergeDefinitions(*batch), std::invalid_argument);
    EXPECT_EQ(io.InquireVariable<int>("duplicate"), &existing);
    EXPECT_EQ(io.InquireVariable<int>("new"), nullptr);
    EXPECT_EQ(io.InquireAttribute<int>("attribute"), nullptr);
    EXPECT_NE(batch->InquireVariable<int>("duplicate"), nullptr);
    EXPECT_NE(batch->InquireVariable<int>("new"), nullptr);
    ASSERT_NE(batch->InquireAttribute<int>("attribute"), nullptr);
}

TEST(CoreIODefinitions, DuplicateAttributeLeavesVariablesIntact)
{
    core::ADIOS adios("C++");
    auto &io = adios.DeclareIO("definitions");
    io.DefineAttribute<int>("duplicate", 1);
    auto batch = io.CreateDefinitionBatch();
    auto &variable = batch->DefineVariable<int>("new");
    batch->DefineAttribute<int>("duplicate", 1);
    EXPECT_THROW(io.MergeDefinitions(*batch), std::invalid_argument);
    EXPECT_EQ(io.InquireVariable<int>("new"), nullptr);
    EXPECT_EQ(batch->InquireVariable<int>("new"), &variable);
    EXPECT_EQ(io.InquireAttribute<int>("duplicate")->m_DataSingleValue, 1);
}

TEST(CoreIODefinitions, BatchOwnershipAndConfiguration)
{
    core::ADIOS adios("C++");
    auto &io = adios.DeclareIO("definitions", ArrayOrdering::ColumnMajor);
    auto &other = adios.DeclareIO("other");
    io.m_VarOpsPlaceholder["data"].push_back({"null", {}});
    auto batch = io.CreateDefinitionBatch();
    io.m_VarOpsPlaceholder.clear();
    auto &variable = batch->DefineVariable<double>("data");
    EXPECT_EQ(variable.m_Operations.size(), 1);
    EXPECT_THROW(other.MergeDefinitions(*batch), std::invalid_argument);
    io.MergeDefinitions(*batch);
    io.FinalizeDefinitions();
    EXPECT_EQ(io.InquireVariable<double>("data"), &variable);
    EXPECT_NO_THROW(io.MergeDefinitions(*batch)); // an empty batch is harmless
}

namespace
{
class AttributeRecorder : public core::Engine
{
public:
    AttributeRecorder(core::IO &io, const std::string &name, Mode mode, helper::Comm comm)
    : Engine("AttributeRecorder", io, name, mode, std::move(comm))
    {
        m_IsOpen = true;
    }

    size_t Notifications = 0;
    bool Installed = true;
    bool OwnedData = true;

    void NotifyEngineAttribute(std::string name, core::AttributeBase *attribute,
                               void *data) noexcept override
    {
        ++Notifications;
        if (attribute->m_Type == DataType::Int32)
        {
            auto *typed = m_IO.InquireAttribute<int>(name);
            Installed &= typed == attribute;
            OwnedData &= typed && data == &typed->m_DataSingleValue;
        }
        else
        {
            auto *typed = m_IO.InquireAttribute<std::string>(name);
            Installed &= typed == attribute;
            OwnedData &= typed && data == typed->m_DataArray.data();
        }
    }

private:
    void DoClose(int) override {}
};
}

TEST(CoreIODefinitions, NotifiesDestinationEngineUsingOwnedData)
{
    core::ADIOS adios("C++");
    auto &io = adios.DeclareIO("definitions");
    core::IO::RegisterEngine("attributerecorder", {core::IO::MakeEngine<AttributeRecorder>,
                                                   core::IO::MakeEngine<AttributeRecorder>});
    io.SetEngine("attributerecorder");
    auto &engine =
        static_cast<AttributeRecorder &>(io.Open("recorder", Mode::Write, helper::CommDummy()));
    auto batch = io.CreateDefinitionBatch();
    {
        const std::string values[] = {"one", "two"};
        batch->DefineAttribute<std::string>("array", values, 2);
        batch->DefineAttribute<int>("scalar", 42);
    }
    EXPECT_EQ(engine.Notifications, 0);
    io.MergeDefinitions(*batch);
    EXPECT_EQ(engine.Notifications, 0);
    io.FinalizeDefinitions();
    EXPECT_EQ(engine.Notifications, 2);
    EXPECT_TRUE(engine.Installed);
    EXPECT_TRUE(engine.OwnedData);
    io.FinalizeDefinitions();
    EXPECT_EQ(engine.Notifications, 2);
    ASSERT_NE(io.InquireAttribute<std::string>("array"), nullptr);
    EXPECT_EQ(io.InquireAttribute<std::string>("array")->m_DataArray[1], "two");
    engine.Close();
}

TEST(CoreIODefinitions, AttributeMayPrecedeInputInAnotherBatch)
{
    core::ADIOS adios("C++");
    auto &io = adios.DeclareIO("definitions");
    auto attributes = io.CreateDefinitionBatch();
    attributes->DefineAttribute<std::string>("units", "meters", "data");
    io.MergeDefinitions(*attributes);
    auto variables = io.CreateDefinitionBatch();
    auto &data = variables->DefineVariable<float>("data", {4}, {0}, {4});
    io.MergeDefinitions(*variables);
    EXPECT_TRUE(attributes->Empty());
    EXPECT_TRUE(variables->Empty());
    io.FinalizeDefinitions();
    EXPECT_EQ(io.InquireVariable<float>("data"), &data);
    ASSERT_NE(io.InquireAttribute<std::string>("units", "data"), nullptr);
    EXPECT_EQ(io.InquireAttribute<std::string>("units", "data")->m_DataSingleValue, "meters");
}

TEST(CoreIODefinitions, OpenRequiresFinalization)
{
    core::ADIOS adios("C++");
    auto &io = adios.DeclareIO("definitions");
    io.SetEngine("Null");
    auto batch = io.CreateDefinitionBatch();
    batch->DefineVariable<int>("data");
    io.MergeDefinitions(*batch);
    EXPECT_THROW(io.Open("unfinalized", Mode::Write, helper::CommDummy()), std::logic_error);
    io.FinalizeDefinitions();
    auto &engine = io.Open("finalized", Mode::Write, helper::CommDummy());
    engine.Close();
}

TEST(CoreIODefinitions, FailedFinalizationDelaysNotificationsUntilRetry)
{
    core::ADIOS adios("C++");
    auto &io = adios.DeclareIO("definitions");
    core::IO::RegisterEngine("attributerecorder", {core::IO::MakeEngine<AttributeRecorder>,
                                                   core::IO::MakeEngine<AttributeRecorder>});
    io.SetEngine("attributerecorder");
    auto &engine =
        static_cast<AttributeRecorder &>(io.Open("recorder", Mode::Write, helper::CommDummy()));
    auto batch = io.CreateDefinitionBatch();
    batch->DefineAttribute<int>("units", 1, "later");
    io.MergeDefinitions(*batch);
    EXPECT_THROW(io.FinalizeDefinitions(), std::invalid_argument);
    EXPECT_EQ(engine.Notifications, 0);
    batch->DefineVariable<int>("later");
    io.MergeDefinitions(*batch);
    io.FinalizeDefinitions();
    EXPECT_EQ(engine.Notifications, 1);
    EXPECT_TRUE(engine.Installed);
    EXPECT_TRUE(engine.OwnedData);
    engine.Close();
}

TEST(CoreIODefinitions, MissingAssociationCanBeSuppliedAfterFailedFinalization)
{
    core::ADIOS adios("C++");
    auto &io = adios.DeclareIO("definitions");
    auto batch = io.CreateDefinitionBatch();
    batch->DefineAttribute<int>("attribute", 7, "later");
    io.MergeDefinitions(*batch);
    EXPECT_THROW(io.FinalizeDefinitions(), std::invalid_argument);
    // Batches remain usable after transfer; retries do not need the original rows.
    batch->DefineVariable<int>("later");
    io.MergeDefinitions(*batch);
    EXPECT_NO_THROW(io.FinalizeDefinitions());
    EXPECT_EQ(io.InquireAttribute<int>("attribute", "later")->m_DataSingleValue, 7);
}

TEST(CoreIODefinitions, AttributeModificationMatchesIO)
{
    core::ADIOS adios("C++");
    auto &io = adios.DeclareIO("definitions");
    auto batch = io.CreateDefinitionBatch();
    batch->DefineAttribute<int>("fixed", 1);
    EXPECT_NO_THROW(batch->DefineAttribute<int>("fixed", 1));
    EXPECT_THROW(batch->DefineAttribute<int>("fixed", 2), std::invalid_argument);
    EXPECT_THROW(batch->DefineAttribute<double>("fixed", 1.0), std::invalid_argument);
    auto &attribute = batch->DefineAttribute<int>("modifiable", 1, "", "/", true);
    const int values[] = {2, 3};
    batch->DefineAttribute<int>("modifiable", values, 2);
    EXPECT_FALSE(attribute.m_IsSingleValue);
    EXPECT_EQ(attribute.m_DataArray, (std::vector<int>{2, 3}));
    batch->DefineAttribute<int>("modifiable", 4);
    EXPECT_TRUE(attribute.m_IsSingleValue);
    EXPECT_EQ(attribute.m_DataSingleValue, 4);
    io.MergeDefinitions(*batch);
    io.FinalizeDefinitions();
    EXPECT_EQ(io.InquireAttribute<int>("modifiable"), &attribute);
}

#ifdef ADIOS2_HAVE_DERIVED_VARIABLE
TEST(CoreIODefinitions, DerivedBeforeInputsAcrossParallelBatches)
{
    core::ADIOS adios("C++");
    auto &io = adios.DeclareIO("definitions");
    auto expressions = io.CreateDefinitionBatch();
    auto inputs = io.CreateDefinitionBatch();
    auto attributes = io.CreateDefinitionBatch();
    std::thread first([&] {
        expressions->DefineDerivedVariable("scaled", "data * factor + @offset",
                                           DerivedVarType::StoreData);
    });
    std::thread second([&] {
        inputs->DefineVariable<float>("data", {4}, {0}, {4});
        inputs->DefineVariable<float>("factor");
    });
    std::thread third([&] { attributes->DefineAttribute<float>("offset", 3.0f); });
    first.join();
    second.join();
    third.join();
    io.MergeDefinitions(*expressions);
    io.MergeDefinitions(*attributes);
    io.MergeDefinitions(*inputs);
    expressions.reset();
    inputs.reset();
    attributes.reset();
    io.FinalizeDefinitions();
    const auto &derived = io.GetDerivedVariables();
    ASSERT_EQ(derived.count("scaled"), 1);
    auto &variable = static_cast<core::VariableDerived &>(*derived.at("scaled"));
    EXPECT_EQ(variable.m_Shape, Dims{4});
    EXPECT_EQ(variable.m_Type, DataType::Float);
    EXPECT_EQ(variable.GetDerivedType(), DerivedVarType::StoreData);
    EXPECT_EQ(variable.m_NameToType.at("@offset"), DataType::Float);
    EXPECT_NO_THROW(io.FinalizeDefinitions());
}

TEST(CoreIODefinitions, MissingDerivedInputCanBeSuppliedLater)
{
    core::ADIOS adios("C++");
    auto &io = adios.DeclareIO("definitions");
    auto batch = io.CreateDefinitionBatch();
    batch->DefineDerivedVariable("result", "data + @offset");
    io.MergeDefinitions(*batch);
    EXPECT_THROW(io.FinalizeDefinitions(), std::invalid_argument);
    batch->DefineVariable<float>("data", {4}, {0}, {4});
    batch->DefineAttribute<float>("offset", 2.0f);
    io.MergeDefinitions(*batch);
    EXPECT_NO_THROW(io.FinalizeDefinitions());
    EXPECT_EQ(io.GetDerivedVariables().count("result"), 1);
}

TEST(CoreIODefinitions, RejectsCollisionsAcrossAllVariableNamespaces)
{
    core::ADIOS adios("C++");
    auto &io = adios.DeclareIO("definitions");
    auto pending = io.CreateDefinitionBatch();
    pending->DefineDerivedVariable("result", "data + 1");
    EXPECT_THROW(pending->DefineVariable<float>("result"), std::invalid_argument);
    EXPECT_THROW(pending->DefineReaderDerivedVariable("result", "data"), std::invalid_argument);
    io.MergeDefinitions(*pending);
    auto collision = io.CreateDefinitionBatch();
    collision->DefineVariable<float>("result");
    collision->DefineAttribute<int>("untouched", 1);
    EXPECT_THROW(io.MergeDefinitions(*collision), std::invalid_argument);
    EXPECT_FALSE(collision->Empty());
    EXPECT_EQ(io.InquireAttribute<int>("untouched"), nullptr);
    auto inputs = io.CreateDefinitionBatch();
    inputs->DefineVariable<float>("data", {4}, {0}, {4});
    inputs->DefineReaderDerivedVariable("reader", "data");
    io.MergeDefinitions(*inputs);
    io.FinalizeDefinitions();
    auto other = io.CreateDefinitionBatch();
    other->DefineVariable<float>("reader");
    EXPECT_THROW(io.MergeDefinitions(*other), std::invalid_argument);
    auto writerCollision = io.CreateDefinitionBatch();
    writerCollision->DefineDerivedVariable("data", "data + 1");
    EXPECT_THROW(io.MergeDefinitions(*writerCollision), std::invalid_argument);
}

TEST(CoreIODefinitions, ReaderRecipeRetainsLazyBinding)
{
    core::ADIOS adios("C++");
    auto &io = adios.DeclareIO("definitions");
    auto batch = io.CreateDefinitionBatch();
    batch->DefineReaderDerivedVariable("result", "data * @scale");
    io.MergeDefinitions(*batch);
    io.FinalizeDefinitions(); // inputs need not exist until the engine opens
    EXPECT_EQ(io.ResolveReaderDerivedVariable("result"), nullptr);
    batch->DefineVariable<float>("data", {4}, {0}, {4});
    batch->DefineAttribute<float>("scale", 2.0f);
    io.MergeDefinitions(*batch);
    io.FinalizeDefinitions();
    auto *result = io.ResolveReaderDerivedVariable("result");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->m_Shape, Dims{4});
}

TEST(CoreIODefinitions, InvalidExpressionsAndUnsupportedChainsFailAtFinalization)
{
    core::ADIOS adios("C++");
    auto &invalid = adios.DeclareIO("invalid");
    auto syntax = invalid.CreateDefinitionBatch();
    EXPECT_NO_THROW(syntax->DefineDerivedVariable("result", "data + )"));
    invalid.MergeDefinitions(*syntax);
    EXPECT_THROW(invalid.FinalizeDefinitions(), std::invalid_argument);
    EXPECT_TRUE(invalid.GetDerivedVariables().empty());

    auto &chains = adios.DeclareIO("chains");
    auto batch = chains.CreateDefinitionBatch();
    batch->DefineVariable<float>("data", {4}, {0}, {4});
    batch->DefineDerivedVariable("first", "data + 1");
    batch->DefineDerivedVariable("second", "first + 1");
    chains.MergeDefinitions(*batch);
    EXPECT_THROW(chains.FinalizeDefinitions(), std::invalid_argument);
    EXPECT_EQ(chains.GetDerivedVariables().count("second"), 0);
}

TEST(CoreIODefinitions, DerivedOperationSnapshotAndFailedConstruction)
{
    core::ADIOS adios("C++");
    auto &io = adios.DeclareIO("definitions");
    io.m_VarOpsPlaceholder["result"].push_back({"null", {}});
    auto batch = io.CreateDefinitionBatch();
    batch->DefineDerivedVariable("result", "data + 1", DerivedVarType::StoreData);
    batch->DefineVariable<float>("data", {4}, {0}, {4});
    io.m_VarOpsPlaceholder.clear();
    io.MergeDefinitions(*batch);
    io.FinalizeDefinitions();
    EXPECT_EQ(io.GetDerivedVariables().at("result")->m_Operations.size(), 1);

    auto &invalid = adios.DeclareIO("invalid");
    invalid.m_VarOpsPlaceholder["bad"].push_back({"null", {}});
    auto bad = invalid.CreateDefinitionBatch();
    bad->DefineVariable<float>("data", {4}, {0}, {4});
    bad->DefineDerivedVariable("bad", "data + 1", DerivedVarType::StatsOnly);
    invalid.MergeDefinitions(*bad);
    EXPECT_THROW(invalid.FinalizeDefinitions(), std::invalid_argument);
    EXPECT_EQ(invalid.GetDerivedVariables().count("bad"), 0);
    EXPECT_THROW(invalid.FinalizeDefinitions(), std::invalid_argument);
}

#ifdef ADIOS2_HAVE_BP5
TEST(CoreIODefinitions, DerivedBP5RoundTrip)
{
    core::ADIOS adios("C++");
    auto &io = adios.DeclareIO("writer");
    io.SetEngine("BP5");
    auto recipe = io.CreateDefinitionBatch();
    recipe->DefineDerivedVariable("result", "data * @scale", DerivedVarType::StoreData);
    io.MergeDefinitions(*recipe);
    auto inputs = io.CreateDefinitionBatch();
    auto &data = inputs->DefineVariable<float>("data", {4}, {0}, {4});
    inputs->DefineAttribute<float>("scale", 2.0f);
    io.MergeDefinitions(*inputs);
    io.FinalizeDefinitions();
    auto &writer = io.Open("batch-derived.bp", Mode::Write, helper::CommDummy());
    const float values[] = {1, 2, 3, 4};
    writer.BeginStep();
    writer.Put(data, values);
    writer.EndStep();
    writer.Close();

    auto &readIO = adios.DeclareIO("reader");
    readIO.SetEngine("BP5");
    auto &reader = readIO.Open("batch-derived.bp", Mode::ReadRandomAccess, helper::CommDummy());
    auto *result = readIO.InquireVariable<float>("result");
    ASSERT_NE(result, nullptr);
    std::vector<float> output(4);
    reader.Get(*result, output.data(), Mode::Sync);
    EXPECT_EQ(output, (std::vector<float>{2, 4, 6, 8}));
    reader.Close();
}
#endif
#endif

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
