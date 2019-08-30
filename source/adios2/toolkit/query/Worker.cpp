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
    if (m_SourceReader)
        m_SourceReader->Close();
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

void Worker::GetResultCoverage(const adios2::Box<adios2::Dims> &outputRegion,
                               std::vector<Box<Dims>> &touchedBlocks)
{
    // std::cout << "  will evaluate shifting  in output region later  .."
    //           << std::endl;
    touchedBlocks.clear();

    if (m_Query && m_SourceReader)
        m_Query->BlockIndexEvaluate(m_SourceReader->m_IO, *m_SourceReader,
                                    touchedBlocks);

    // Query<T> simpleQ = parse();

    /*
    if (m_Idx == nullptr) {
      throw std::invalid_argument("Unable to use index with given type.");
    }

    {
      // use min max

    }
    */
}
} // namespace query
} // namespace adios2
