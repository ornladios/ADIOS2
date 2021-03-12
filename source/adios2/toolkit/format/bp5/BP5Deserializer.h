/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP5Deserializer.h
 *
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP5_BP5DESERIALIZER_H_
#define ADIOS2_TOOLKIT_FORMAT_BP5_BP5DESERIALIZER_H_

#include "adios2/core/Attribute.h"
#include "adios2/core/IO.h"

#include "BP5Base.h"
#include "atl.h"
#include "ffs.h"
#include "fm.h"

#ifdef _WIN32
#pragma warning(disable : 4250)
#endif

namespace adios2
{
namespace format
{

class BP5Deserializer
{

public:
    BP5Deserializer();

    ~BP5Deserializer();

    void InstallMetaMetaData(MetaMetaInfoBlock &MMList);
    void InstallMetaData(void *MetadataBlock, size_t BlockLen,
                         size_t WriterRank);

private:
    FFSContext ReaderFFSContext;
};

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_UTILITIES_FORMAT_BP5_BP5Serializer_H_ */
