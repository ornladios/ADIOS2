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
    BP5Base();

    struct MetaMetaInfoBlock
    {
        char *MetaMetaInfo;
        size_t MetaMetaInfoLen;
        char *MetaMetaID;
        size_t MetaMetaIDLen;
    };

#define BASE_FIELDS                                                            \
    size_t Dims;       /* How many dimensions does this array have */          \
    size_t BlockCount; /* How many blocks are written	*/                       \
    size_t DBCount;    /* Dimens * BlockCount	*/                               \
    size_t *Shape;     /* Global dimensionality  [Dims]	NULL for local */      \
    size_t *Count;     /* Per-block Counts	  [DBCount] */                      \
    size_t *Offsets;   /* Per-block Offsets	  [DBCount]	NULL for local         \
                        */                                                     \
    size_t *DataBlockLocation; /* Per-block Offset in PG [BlockCount] */

    typedef struct _MetaArrayRec
    {
        BASE_FIELDS
    } MetaArrayRec;

    typedef struct _MetaArrayRecOperator
    {
        BASE_FIELDS
        size_t *DataBlockSize; // Per-block Lengths [BlockCount]
    } MetaArrayRecOperator;

    typedef struct _MetaArrayRecMM
    {
        BASE_FIELDS
        char *MinMax; // char[TYPESIZE][BlockCount]  varies by type
    } MetaArrayRecMM;

    typedef struct _MetaArrayRecOperatorMM
    {
        BASE_FIELDS
        size_t *DataBlockSize; // Per-block Lengths [BlockCount]
        char *MinMax;          // char[TYPESIZE][BlockCount]  varies by type
    } MetaArrayRecOperatorMM;

#undef BASE_FIELDS

    struct BP5MetadataInfoStruct
    {
        size_t BitFieldCount;
        size_t *BitField;
        size_t DataBlockSize;
    };

    void BP5BitfieldSet(struct BP5MetadataInfoStruct *MBase, int Bit) const;
    int BP5BitfieldTest(struct BP5MetadataInfoStruct *MBase, int Bit) const;
    FMField *MetaArrayRecListPtr;
    FMField *MetaArrayRecOperatorListPtr;
    FMField *MetaArrayRecMM1ListPtr;
    FMField *MetaArrayRecOperatorMM1ListPtr;
    FMField *MetaArrayRecMM2ListPtr;
    FMField *MetaArrayRecOperatorMM2ListPtr;
    FMField *MetaArrayRecMM4ListPtr;
    FMField *MetaArrayRecOperatorMM4ListPtr;
    FMField *MetaArrayRecMM8ListPtr;
    FMField *MetaArrayRecOperatorMM8ListPtr;
    FMField *MetaArrayRecMM16ListPtr;
    FMField *MetaArrayRecOperatorMM16ListPtr;
};
} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_UTILITIES_FORMAT_B5_BP5Base_H_ */
