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
    // if (m_SourceReader)
    //#m_SourceReader->Close();

    if (m_Query != nullptr)
        delete m_Query;
}
/*
template <class T>
void Worker<T>::BuildIdxFile(adios2::IndexType&type, adios2::Params& input)
{
  if (type != adios2::IndexType::MIN_MAX)
    throw std::invalid_argument("Unable to build index with given type.");

  {
    // build min max
    m_IndexTool = new BlockIndex();

    bool doOverWrite = ToBoolValue(input,
adios2::BlockIndexBuilder.m_ParamOverWrite); size_t blockSize =
ToUIntValue(input, adios2::BlockIndexBuilder.m_ParamBlockSize, 2000000);

    adios2::IO idxWriteIO =
m_adios2.DeclareIO(std::string("BLOCKINDEX-Write-")+m_DataReader->Name());
    adios2::BlockIndexBuilder builder(m_IdxFileName, m_Comm, overwrite);
    builder.GenerateIndexFrom(*m_DataIO, idxWriteIO, *m_DataReader, blockSize);
  }
}
*/

QueryVar *Worker::GetBasicVarQuery(adios2::core::IO &currentIO,
                                   const std::string &variableName)
{
    const std::string varType = currentIO.InquireVariableType(variableName);
    if (varType.size() == 0)
    {
        std::cerr << "No such variable: " << variableName << std::endl;
        return nullptr;
    }
#define declare_type(T)                                                        \
    if (varType == helper::GetType<T>())                                       \
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
    // std::cout << "  will evaluate shifting  in output region later  .."
    //           << std::endl;
    touchedBlocks.clear();

    if (!m_Query->UseOutputRegion(outputRegion))
        throw std::invalid_argument("Unable to use the output region.");

    if (m_Query && m_SourceReader)
        m_Query->BlockIndexEvaluate(m_SourceReader->m_IO, *m_SourceReader,
                                    touchedBlocks);
}
} // namespace query
} // namespace adios2
