#include "Worker.h"
//#include "XmlWorker.cpp"

namespace adios2
{
namespace query
{

Worker::Worker(const std::string &queryFile, adios2::core::Engine *adiosEngine)
: m_QueryFile(queryFile), m_SourceReader(adiosEngine)
{
}

Worker::~Worker()
{
    if (m_Query != nullptr)
        delete m_Query;
}

QueryVar *Worker::GetBasicVarQuery(adios2::core::IO &currentIO,
                                   const std::string &variableName)
{
    const DataType varType = currentIO.InquireVariableType(variableName);
    if (varType == DataType::None)
    {
        std::cerr << "No such variable: " << variableName << std::endl;
        return nullptr;
    }
#define declare_type(T)                                                        \
    if (varType == helper::GetDataType<T>())                                   \
    {                                                                          \
        core::Variable<T> *var = currentIO.InquireVariable<T>(variableName);   \
        if (var)                                                               \
        {                                                                      \
            QueryVar *q = new QueryVar(variableName);                          \
            adios2::Dims zero(var->Shape().size(), 0);                         \
            adios2::Dims shape = var->Shape();                                 \
            q->SetSelection(zero, shape);                                      \
            return q;                                                          \
        }                                                                      \
    }
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type
    return nullptr;
}

void Worker::GetResultCoverage(const adios2::Box<adios2::Dims> &outputRegion,
                               std::vector<Box<Dims>> &touchedBlocks)
{
    touchedBlocks.clear();

    if (!m_Query->UseOutputRegion(outputRegion))
    {
        throw std::invalid_argument("Unable to use the output region.");
    }

    if (m_Query && m_SourceReader)
    {
        m_Query->BlockIndexEvaluate(m_SourceReader->m_IO, *m_SourceReader,
                                    touchedBlocks);
    }
}
} // namespace query
} // namespace adios2
