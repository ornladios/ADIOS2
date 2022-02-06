#include "NullCoreWriter.h"
#include "NullCoreWriter.tcc"

#include "adios2/helper/adiosLog.h"

namespace adios2
{
namespace core
{
namespace engine
{

struct NullCoreWriter::NullCoreWriterImpl
{
    size_t CurrentStep = 0;
    bool IsInStep = false;
    bool IsOpen = true;
};

NullCoreWriter::NullCoreWriter(IO &io, const std::string &name, const Mode mode,
                               helper::Comm comm)
: Engine("NullCoreWriter", io, name, mode, std::move(comm)),
  Impl(new NullCoreWriter::NullCoreWriterImpl)
{
}

NullCoreWriter::~NullCoreWriter() = default;

StepStatus NullCoreWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    if (!Impl->IsOpen)
    {
        helper::Throw<std::runtime_error>(
            "Engine", "NullCoreWriter", "BeginStep",
            "NullCoreWriter::BeginStep: Engine already closed");
    }

    if (Impl->IsInStep)
    {
        helper::Throw<std::runtime_error>(
            "Engine", "NullCoreWriter", "BeginStep",
            "NullCoreWriter::BeginStep: Step already active");
    }

    Impl->IsInStep = true;
    ++Impl->CurrentStep;
    return StepStatus::OK;
}

size_t NullCoreWriter::CurrentStep() const
{
    if (!Impl->IsOpen)
    {
        helper::Throw<std::runtime_error>(
            "Engine", "NullCoreWriter", "CurrentStep",
            "NullCoreWriter::CurrentStep: Engine already closed");
    }

    return Impl->CurrentStep;
}

void NullCoreWriter::EndStep()
{
    if (!Impl->IsOpen)
    {
        helper::Throw<std::runtime_error>(
            "Engine", "NullCoreWriter", "EndStep",
            "NullCoreWriter::EndStep: Engine already closed");
    }

    if (!Impl->IsInStep)
    {
        helper::Throw<std::runtime_error>(
            "Engine", "NullCoreWriter", "EndStep",
            "NullCoreWriter::EndStep: No active step");
    }

    Impl->IsInStep = false;
}

void NullCoreWriter::PerformPuts()
{
    if (!Impl->IsOpen)
    {
        helper::Throw<std::runtime_error>(
            "Engine", "NullCoreWriter", "PerformPuts",
            "NullCoreWriter::PerformPuts: Engine already closed");
    }

    return;
}

void NullCoreWriter::Flush(const int)
{
    if (!Impl->IsOpen)
    {
        helper::Throw<std::runtime_error>(
            "Engine", "NullCoreWriter", "Flush",
            "NullCoreWriter::Flush: Engine already closed");
    }

    return;
}

void NullCoreWriter::DoClose(const int)
{
    if (!Impl->IsOpen)
    {
        helper::Throw<std::runtime_error>("Engine", "NullCoreWriter", "DoClose",
                                          "already closed");
    }

    Impl->IsOpen = false;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
