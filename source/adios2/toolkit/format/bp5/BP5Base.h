/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP5Base.h
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP5_BP5BASE_H_
#define ADIOS2_TOOLKIT_FORMAT_BP5_BP5BASE_H_

#include "adios2/core/Attribute.h"
#include "adios2/core/IO.h"
#include "adios2/toolkit/format/buffer/BufferV.h"
#include "adios2/toolkit/format/buffer/heap/BufferSTL.h"
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

struct MetaMetaInfoBlock
{
    char *MetaMetaInfo;
    size_t MetaMetaInfoLen;
    char *MetaMetaID;
    size_t MetaMetaIDLen;
};

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_UTILITIES_FORMAT_B5_BP5Base_H_ */
