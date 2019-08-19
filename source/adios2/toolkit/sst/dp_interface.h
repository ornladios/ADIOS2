#ifndef _DP_INTERFACE_H
#define _DP_INTERFACE_H

#include <sst_data.h>

#include "adios2/common/ADIOSConfig.h"
#include "adios2/helper/mpiwrap.h"
#include <evpath.h>

/*!
 *
 * CP_DP_Interface is the set of data format descriptions and function
 * pointers that define a dataplane interface to control plane.
 *
 */
typedef struct _CP_DP_Interface *CP_DP_Interface;

/*!
 * CP_Services is the type of a pointer to a struct of function pointers
 * that give data plane access to control plane routines and functions.
 * Generally it is the first argument to all DP functions invoked by the
 * control plane.
 */
typedef struct _CP_Services *CP_Services;

/*!
 * DP_RS_Stream is an externally opaque pointer-sized value that represents
 * the Reader Side DP stream.  It is returned by an init function and
 * provided back to the dataplane on every subsequent reader side call.
 */
typedef void *DP_RS_Stream;

/*!
 * DP_WS_Stream is an externally opaque pointer-sized value that represents
 * the Writer Side DP stream.  Because a stream might have multiple readers,
 * this value is only provided to the per-reader writer-side initialization
 * function, which returns its own opaque stream ID value.
 */
typedef void *DP_WS_Stream;

/*!
 * DP_WSR_Stream is an externally opaque pointer-sized value that represents
 * the Writer Side *per Reader* DP stream.  This value is returned by the
 * per-reader writer-side initialization and provided back to the dataplane
 * on any later reader-specific operations.
 */
typedef void *DP_WSR_Stream;

/*!
 * CP_PeerCohort is a value provided to the data plane that acts as a
 * handle to the opposite (reader or writer) cohort.  It is used in the
 * sst_send_to_peer service and helps the dataplane leverage existing
 * control plane messaging capabilities.
 */
typedef void *CP_PeerCohort;

/*!
 * CP_DP_InitReaderFunc is the type of a dataplane reader-side stream
 * initialization function.  Its return value is DP_RS_stream, an externally
 * opaque handle which is provided to the dataplane on all subsequent
 * operations for this stream.  'stream' is an input parameter and is the
 * control plane-level reader-side stream identifier.  This may be useful
 * for callbacks, access to MPI communicator, EVPath info, etc. so can be
 * associated with the DP_RS_stream.  'ReaderContactInfoPtr' is a pointer to a
 * void*.  That void* should be filled in by the init function with a
 * pointer to reader-specific contact information for this process.  The
 * `readerContactFormats` FMStructDescList should describe the datastructure
 * pointed to by the void*.  The control plane will gather this information
 * for all reader ranks, transmit it to the writer cohort and provide it as
 * an array of pointers in the `providedReaderInfo` argument to
 * CP_DP_InitWriterPerReaderFunc.  Params is the set of Engine-level parameters
 * set at the ADIOS level.
 */
typedef DP_RS_Stream (*CP_DP_InitReaderFunc)(CP_Services Svcs, void *CP_Stream,
                                             void **ReaderContactInfoPtr,
                                             struct _SstParams *Params);

/*!
 * CP_DP_DestroyReaderFunc is the type of a dataplane reader-side
 * stream destruction function.  Its should shutdown and deallocate
 * dataplane resources associated with a reader-side stream.
 */
typedef void (*CP_DP_DestroyReaderFunc)(CP_Services Svcs, DP_RS_Stream Reader);

/*!
 * CP_DP_InitWriterFunc is the type of a dataplane writer-side stream
 * initialization function.  Its return value is DP_WS_stream, an externally
 * opaque handle which is provided to the dataplane on all subsequent
 * stream-wide operations for this stream.  'stream' is an input parameter and
 * is the control plane-level writer-side stream identifier.  This may be useful
 * for callbacks, access to MPI communicator, EVPath info, etc. so can be
 * associated with the DP_RS_stream.  Params is the set of Engine-level
 * parameters set at the ADIOS level.
 */
typedef DP_WS_Stream (*CP_DP_InitWriterFunc)(CP_Services Svcs, void *CP_Stream,
                                             struct _SstParams *Params);

/*!
 * CP_DP_DestroyWriterFunc is the type of a dataplane writer-side
 * stream destruction function.  Its should shutdown and deallocate
 * dataplane resources associated with a writer-side stream.
 */
typedef void (*CP_DP_DestroyWriterFunc)(CP_Services Svcs, DP_RS_Stream Writer);

/*!
 * CP_DP_InitWriterPerReaderFunc is the type of a dataplane writer-side
 * per-reader stream initialization function.  It is called when a new
 * reader joins an writer-side stream.  Its return value is DP_WSR_stream,
 * an externally opaque handle which is provided to the dataplane on all
 * operations on this stream that are specific to this reader.  operations
 * for this stream.  'stream' is an input parameter and is the DP_WS_stream
 * value that was returned when this stream was initialized via the
 * CP_DP_InitWriterFunc.  `readerCohortSize` is the size of the reader's MPI
 * cohort.  `providedReaderInfo` is a pointer to an array of void* pointers
 * with array size `readerCohortSize`.  The Nth element of the array is a
 * pointer to the value returned in initReaderInfo by reader rank N (with
 * type described by ReaderContactFormats).  'initWriterInfo' is a pointer
 * to a void*.  That void* should be filled in by the init function with a
 * pointer to writer-specific contact information for this process.  The
 * `writerContactFormats` FMStructDescList should describe the datastructure
 * pointed to by the void*.  The control plane will gather this information
 * for all writer ranks, transmit it to the reader cohort and provide it as
 * an array of pointers in the `providedWriterInfo` argument to
 * ProvideWriterDataToReader().  The `peerCohort` argument is a handle to
 * the reader-side peer cohort for use in peer-to-peer messaging.
 */
typedef DP_WSR_Stream (*CP_DP_InitWriterPerReaderFunc)(
    CP_Services Svcs, DP_WS_Stream Stream, int ReaderCohortSize,
    CP_PeerCohort PeerCohort, void **ProvidedReaderInfo,
    void **WriterContactInfoPtr);

/*!
 * CP_DP_DestroyWriterPerReaderFunc is the type of a dataplane writer-side
 * stream destruction function.  Its should shutdown and deallocate
 * dataplane resources associated with a writer-side stream.
 */
typedef void (*CP_DP_DestroyWriterPerReaderFunc)(CP_Services Svcs,
                                                 DP_RS_Stream Writer);

/*
 * CP_DP_ProvideWriterDataToReaderFunc is the type of a dataplane reader-side
 * function that provides information about the newly-connected writer-side
 * stream.  The `stream` parameter was that which was returned by a call to
 * the CP_DP_InitReaderFunc.  `writerCohortSize` is the size of the
 * writer's MPI cohort.  `providedWriterInfo` is a pointer to an array of
 * void* pointers with array size `writerCohortSize`.  The Nth element of
 * the array is a pointer to the value returned in WriterContactInfoPtr by
 * writer
 * rank N (with type described by WriterContactFormats).  `PeerCohort`
 * argument is a handle to writer-side peer cohort for use in peer-to-peer
 * messaging.
 */
typedef void (*CP_DP_ProvideWriterDataToReaderFunc)(CP_Services Svcs,
                                                    DP_RS_Stream Stream,
                                                    int WriterCohortSize,
                                                    CP_PeerCohort PeerCohort,
                                                    void **ProvidedWriterInfo);

/*
 *  DP_CompletionHandle an externally opaque pointer-sized value that is
 *  returned by the asynchronous DpReadRemoteMemory() call and which can be
 *  used to wait for the compelteion of the read.
 */
typedef void *DP_CompletionHandle;

/*!
 * CP_DP_ReadRemoteMemoryFunc is the type of a dataplane function that reads
 * into a local buffer the data contained in the data block associated with
 * a specific writer `rank` and a specific `timestep`.  The data should be
 * placed in the area pointed to by `buffer`.  The returned data should
 * start at offset `offset` from the beginning of the writers data block and
 * continue for `length` bytes.  The value provided for DP_TimestepInfo will
 * be value which was returned as the void* pointed to by TimestepInfoPtr on
 * the writer side in ProvideTimestepFunc.
 */
typedef DP_CompletionHandle (*CP_DP_ReadRemoteMemoryFunc)(
    CP_Services Svcs, DP_RS_Stream RS_Stream, int Rank, long Timestep,
    size_t Offset, size_t Length, void *Buffer, void *DP_TimestepInfo);

/*!
 * CP_DP_WaitForCompletionFunc is the type of a dataplane function that
 * suspends the execution of the current thread until the asynchronous
 * CP_DP_ReadRemoteMemory call that returned its `handle` parameter.
 * the return value is 0 in the event that the wait failed, 1 on success.
 */
typedef int (*CP_DP_WaitForCompletionFunc)(CP_Services Svcs,
                                           DP_CompletionHandle Handle);

/*!
 * CP_DP_NotifyConnFailureFunc is the type of a dataplane function which the
 * control plane uses to notify the data plane that a CP-level connection
 * has failed (and therefore the remote host is likely dead).  This is an
 * informational function by the CP, but as a side effect it should cause
 * any pending Wait operations (for ReadRemoteMemory) to complete and return
 * an error.
 */
typedef void (*CP_DP_NotifyConnFailureFunc)(CP_Services Svcs,
                                            DP_RS_Stream RS_Stream,
                                            int FailedPeerRank);

/*!
 * CP_DP_ProvideTimestepFunc is the type of a dataplane function that
 * delivers a block of data associated with timestep `timestep` to the
 * dataplane, where it should be available for remote read requests until it
 * is released with CP_DP_ReleaseTimestep.  While not necessarily useful for
 * the data plane, we also deliver the local (non-consolidated) metadata and
 * a pointer to a void*, TimestepInfoPtr.  That void* should be filled in by
 * the ProvideTimestep function with a pointer to any DP info that the data
 * plane wishes to be available on the reader side for this timestep.  The
 * `TimestepInfoFormats` FMStructDescList should describe the datastructure
 * pointed to by the void*.  The control plane will gather this information
 * for all writer ranks, transmit it to the reader cohort along with the
 * aggregated metadata.
 */
typedef void (*CP_DP_ProvideTimestepFunc)(CP_Services Svcs, DP_WS_Stream Stream,
                                          struct _SstData *Data,
                                          struct _SstData *LocalMetadata,
                                          long Timestep,
                                          void **TimestepInfoPtr);

/*!
 * CP_DP_PerReaderTimestepRegFuncc is the type of a dataplane function that
 * notifies DataPlane that a particular timestep (which has been previosly
 * registered via the CP_DP_ProvideTimestepFunc will be provided to a
 * specific reader.
 */
typedef void (*CP_DP_PerReaderTimestepRegFunc)(CP_Services Svcs,
                                               DP_WSR_Stream Stream,
                                               long Timestep,
                                               int WriterDefinitionsLocked);

/*!
 * CP_DP_ReadPatternLockedFunc is the type of a dataplane function
 * that notifies writer-side DataPlane that the reader has specified
 * that his read pattern will not change.  This is only called if the
 * writer has also specified that his write geometry will not change.
 */
typedef void (*CP_DP_ReadPatternLockedFunc)(CP_Services Svcs,
                                            DP_WSR_Stream Stream,
                                            long EffectiveTimestep);

/*!
 * CP_DP_ReleaseTimestepFunc is the type of a dataplane function that
 * informs the dataplane that the data associated with timestep `timestep`
 * will no longer be the subject of remote read requests, so its resources
 * may be released.
 */
typedef void (*CP_DP_ReleaseTimestepFunc)(CP_Services Svcs, DP_WS_Stream Stream,
                                          long Timestep);

/*!
 * CP_DP_PerReaderReleaseTimestepFunc is the type of a dataplane function
 * that informs the dataplane that a particular reader is finished with a
 * particular timestep and the dataplane will no will no longer be receive
 * read requests from that particular reader.  DP can count on: 1) receiving
 * one of these calls to match eash PerReaderTimestepRegFunc call, and 2)
 * receiving these calls prior to the call to the overall
 * ReleaseTimestepFunc.
 */
typedef void (*CP_DP_PerReaderReleaseTimestepFunc)(CP_Services Svcs,
                                                   DP_WSR_Stream Stream,
                                                   long Timestep);

/*!
 * CP_DP_GetPriorityFunc is the type of a dataplane initialization
 * function that returns the relative priority of this particular
 * dataplane WRT other dataplanes.  Its return value is an integer
 * where higher means higher priority (more desirable) and -1 means
 * that the dataplane cannot be used.  The function is really intended
 * to support dataplanes for which we cannot determine at compile-time
 * whether or not the run-time conditions will support their use
 * (libfabric is the primum exemplum).  The Svcs and CP_Stream
 * parameters exist only to support verbosity in the DP-level
 * function.
 */
typedef int (*CP_DP_GetPriorityFunc)(CP_Services Svcs, void *CP_Stream,
                                     struct _SstParams *Params);

/*!
 * CP_DP_UnGetPriorityFunc is the type of a dataplane initialization
 * function that undoes/deallocates anything that might have been done
 * or allocated by the GetPriority function in the event that the
 * dataplane in question is not used.
 */
typedef void (*CP_DP_UnGetPriorityFunc)(CP_Services Svcs, void *CP_Stream);

struct _CP_DP_Interface
{
    FMStructDescList ReaderContactFormats;
    FMStructDescList WriterContactFormats;
    FMStructDescList TimestepInfoFormats;

    CP_DP_InitReaderFunc initReader;
    CP_DP_InitWriterFunc initWriter;
    CP_DP_InitWriterPerReaderFunc initWriterPerReader;
    CP_DP_ProvideWriterDataToReaderFunc provideWriterDataToReader;

    CP_DP_ReadRemoteMemoryFunc readRemoteMemory;
    CP_DP_WaitForCompletionFunc waitForCompletion;
    CP_DP_NotifyConnFailureFunc notifyConnFailure;

    CP_DP_ProvideTimestepFunc provideTimestep;
    CP_DP_PerReaderTimestepRegFunc readerRegisterTimestep;
    CP_DP_ReleaseTimestepFunc releaseTimestep;
    CP_DP_PerReaderReleaseTimestepFunc readerReleaseTimestep;
    CP_DP_ReadPatternLockedFunc readPatternLocked;

    CP_DP_DestroyReaderFunc destroyReader;
    CP_DP_DestroyWriterFunc destroyWriter;
    CP_DP_DestroyWriterPerReaderFunc destroyWriterPerReader;

    CP_DP_GetPriorityFunc getPriority;
    CP_DP_UnGetPriorityFunc unGetPriority;
};

typedef void (*CP_VerboseFunc)(void *CP_Stream, char *Format, ...);
typedef CManager (*CP_GetCManagerFunc)(void *CP_stream);
typedef MPI_Comm (*CP_GetMPICommFunc)(void *CP_Stream);
typedef int (*CP_SendToPeerFunc)(void *CP_Stream, CP_PeerCohort PeerCohort,
                                 int Rank, CMFormat Format, void *Data);
struct _CP_Services
{
    CP_VerboseFunc verbose;
    CP_GetCManagerFunc getCManager;
    CP_SendToPeerFunc sendToPeer;
    CP_GetMPICommFunc getMPIComm;
};

CP_DP_Interface SelectDP(CP_Services Svcs, void *CP_Stream,
                         struct _SstParams *Params);

#endif
