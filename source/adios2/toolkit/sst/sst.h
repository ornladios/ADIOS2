/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 *  The SST external interfaces.
 *
 *  This is more a rough sketch than a final version.  The details will
 *  change when the integration with ADIOS2 layers happen.  In the meantime,
 *  this interface (hopefully) captures enough of the functionality for
 *  control plane and data plane implementations to proceed while the
 *  integration details are hashed out.
 */
#ifndef SST_H_
#define SST_H_

#include "sst_comm_fwd.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/*!
 * SstStream is the basic type of a stream connecting an ADIOS2 reader
 * and an ADIOS2 writer.  Externally the same data type is used for both.
 */
typedef struct _SstStream *SstStream;

/*
 *  metadata and typedefs are tentative and may come from ADIOS2 constructors.
 */
typedef struct _SstMetaMetaBlock *SstMetaMetaList;
typedef struct _SstFullMetadata *SstFullMetadata;
typedef struct _SstData *SstData;
typedef struct _SstBlock *SstBlock;

typedef enum
{
    SstSuccess,
    SstEndOfStream,
    SstFatalError,
    SstTimeout
} SstStatusValue;

/* The SST version of enum class StepMode in ADIOSTypes.h */
typedef enum
{
    SstAppend, // writer modes ignored in SST
    SstUpdate, // writer modes ignored in SST
    SstNextAvailable,
    SstLatestAvailable // reader advance mode
} SstStepMode;

typedef struct _SstParams *SstParams;

typedef enum
{
    SstQueueFullBlock = 0,
    SstQueueFullDiscard = 1
} SstQueueFullPolicy;

/*
 *  Writer-side operations
 */
extern SstStream SstWriterOpen(const char *filename, SstParams Params, SMPI_Comm comm);

extern void SstStreamDestroy(SstStream Stream);

typedef void (*DataFreeFunc)(void *Data);
extern void SstProvideTimestepMM(SstStream s, SstData LocalMetadata, SstData LocalData,
                                 long Timestep, DataFreeFunc FreeData, void *FreeClientData,
                                 SstData AttributeData, DataFreeFunc FreeAttribute,
                                 void *FreeAttributeClientData, SstMetaMetaList MMBlocks);
extern void SstWriterClose(SstStream stream);
/*  SstWriterDefinitionLock is called once only, on transition from unlock to
 * locked definitions */
extern void SstWriterDefinitionLock(SstStream stream, long EffectiveTimestep);

/*
 *  Reader-side operations
 */
extern SstStream SstReaderOpen(const char *filename, SstParams Params, SMPI_Comm comm);
extern void SstReaderGetParams(SstStream stream, int *WriterIsRowMajor);
extern SstFullMetadata SstGetCurMetadata(SstStream stream);
extern SstMetaMetaList SstGetNewMetaMetaData(SstStream stream, size_t timestep);
extern SstBlock SstGetAttributeData(SstStream stream, size_t timestep);
extern void *SstReadRemoteMemory(SstStream s, int rank, size_t timestep, size_t offset,
                                 size_t length, void *buffer, void *DP_TimestepInfo);
extern SstStatusValue SstWaitForCompletion(SstStream stream, void *completion);
extern void SstReleaseStep(SstStream stream);
extern SstStatusValue SstAdvanceStep(SstStream stream, const float timeout_sec);
extern void SstReaderClose(SstStream stream);
extern size_t SstCurrentStep(SstStream s);
/*  SstReaderDefinitionLock is called once only, on transition from unlock to
 * locked definitions */
extern void SstReaderDefinitionLock(SstStream stream, size_t EffectiveTimestep);

#include "sst_data.h"

#define SST_POSTFIX ".sst"

#ifdef __cplusplus
}
#endif

#endif /* SST_H_*/
