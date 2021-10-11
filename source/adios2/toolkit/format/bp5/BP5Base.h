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

class BP5Base
{
public:
    struct MetaMetaInfoBlock
    {
        char *MetaMetaInfo;
        size_t MetaMetaInfoLen;
        char *MetaMetaID;
        size_t MetaMetaIDLen;
    };

    typedef struct _MetaArrayRec
    {
        size_t Dims;          // How many dimensions does this array have
        size_t BlockCount;    // How many blocks are written
        size_t DBCount;       // Dimens * BlockCount
        size_t *Shape;        // Global dimensionality  [Dims]	NULL for local
        size_t *Count;        // Per-block Counts	  [DBCount]
        size_t *Offsets;      // Per-block Offsets	  [DBCount]	NULL for local
        size_t *DataLocation; // Per-block Offsets [BlockCount]
    } MetaArrayRec;

    struct BP5MetadataInfoStruct
    {
        size_t BitFieldCount;
        size_t *BitField;
        size_t DataBlockSize;
    };

    void BP5BitfieldSet(struct BP5MetadataInfoStruct *MBase, int Bit);
    int BP5BitfieldTest(struct BP5MetadataInfoStruct *MBase, int Bit);
};
} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_UTILITIES_FORMAT_B5_BP5Base_H_ */
