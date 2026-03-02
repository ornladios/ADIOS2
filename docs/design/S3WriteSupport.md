# ADIOS2 S3 Write Support Design

**Status**: Implemented (multi-object default, single-object optional)
**Date**: January-February 2026
**Authors**: Greg Eisenhauer, Claude

## Overview

ADIOS2 supports writing BP5 data files to S3-compatible object storage while keeping metadata local for fast access. Two object modes are available:

- **Multi-object mode** (default): Each data file is stored as a sequence of numbered S3 objects (`data.0.0`, `data.0.1`, ...). Objects are finalized individually via `PutObject`, providing automatic crash recovery.
- **Single-object mode**: Each data file is stored as one S3 object via multipart upload. No automatic crash recovery.

## Architecture

### Hybrid Storage Model

```
Local filesystem:              S3 bucket (multi-object mode):
sim.bp/                        mybucket/sim.bp/
  ├── md.idx                     ├── data.0.0
  ├── md.0                       ├── data.0.1
  ├── mmd.0                      ├── data.0.2
  └── s3.json                    ├── data.1.0
                                 └── data.1.1

                               S3 bucket (single-object mode):
                               mybucket/sim.bp/
                                 ├── data.0
                                 ├── data.1
                                 └── data.2
```

- **Local**: All metadata files (md.idx, md.0, mmd.0) - small, frequently accessed
- **S3**: Data files only - large, bulk storage
- **s3.json**: Sidecar file recording S3 location info for readers

### Why Hybrid?

1. **Fast metadata access** - bpls and file scanning don't touch S3
2. **bpls just works** - reads local metadata, fetches S3 data only for variable reads
3. **Minimal engine changes** - same file structure as local BP5

## Multi-Object Mode (Default)

### Object Naming

Each data file (e.g., `data.0`) is stored as a sequence of numbered objects:
```
data.0.0    (first segment)
data.0.1    (second segment)
data.0.2    (third segment)
...
```

Object numbers are sequential starting at 0 and are **independent of step numbers**. Multiple objects may be created per step (e.g., via large writes that exceed the direct upload threshold).

### FinalizeSegment

The `Transport::FinalizeSegment()` virtual method signals that accumulated data should be uploaded as a numbered object. It is called:

1. **After async data write completes** - in the async write thread, after all step data has been written
2. **At Close** - to upload any remaining buffered data

`Flush()` is a no-op for S3 transports. In multi-object mode, uploads are driven by `FinalizeSegment()` at step boundaries, not by `Flush()`. In single-object mode, S3's 5 MB minimum part size prevents partial flushes. All buffered data is uploaded as complete objects via `FinalizeSegment()` or `Close()`.

For non-S3 transports, `FinalizeSegment()` is a no-op.

### Write Flow

```
Open(data.0, Write):
  → Store base object name "data.0"
  → Initialize object counter = 0
  → If subfile 0: DeleteStaleObjects() (remove all data.* from previous run)

Write(buffer, size):
  → If large write (>= 1MB threshold):
      1. FlushWriteBuffer() as object N (if buffer non-empty), N++
      2. Upload buffer directly as object N (zero-copy), N++
  → Otherwise:
      → Append to write buffer

FinalizeSegment():
  → If write buffer non-empty:
      → Upload as object N via PutObject, N++

Close():
  → FlushWriteBuffer() (upload remaining data)
```

### Zero-Copy Large Writes

When a `Write()` call exceeds the direct upload threshold (default 1MB):

1. Any buffered data is flushed as one object
2. The large buffer is uploaded directly as a separate object (no memcpy)

This avoids copying large data blocks into the internal buffer.

### Crash Recovery

In multi-object mode, crash recovery is automatic:

- Each object is finalized immediately via `PutObject`
- Completed objects are visible and readable
- Only data buffered but not yet uploaded is lost on crash
- No finalize utility needed

## Single-Object Mode

Set `S3ObjectMode=single` to use one multipart upload per data file.

### S3 Multipart Upload Constraints

- Minimum part size: 5 MB (except final part)
- Maximum parts: 10,000 per object
- Maximum object size: 5 TB
- Object only visible after `CompleteMultipartUpload`

### Write Flow

```
Open(data.0, Write):
  → InitiateMultipartUpload → UploadId

Write():
  → Buffer data
  → When buffer >= 5MB: UploadPart

Close():
  → Upload remaining buffer as final part
  → CompleteMultipartUpload (object now visible)
```

### Crash Recovery (Single Mode)

Single-object mode has no crash recovery. If the writer crashes before `Close()` calls `CompleteMultipartUpload`, the in-progress upload is invisible and will eventually be cleaned up by S3's lifecycle rules. A finalize utility using S3's `ListParts` API could be implemented in the future.

## Read Path

### Virtual Concatenated File

When opening a data file for reading, the transport first tries `HeadObject` for the exact object name. If not found, it discovers multi-object layout:

1. `ListObjectsV2` with prefix `data.0.`
2. Filter for numeric suffixes only
3. Sort by object number
4. Build cumulative offset table
5. Present as single virtual file of size = sum of all sub-object sizes

### Cross-Object Reads

Read requests that span object boundaries are handled transparently:
- Binary search to find the starting sub-object
- Loop reading from consecutive sub-objects until request is fulfilled
- Each sub-read uses S3 `GetObject` with `Range` header

The virtual file abstraction is transparent to BP5Reader.

## Configuration

### Engine Parameters

```cpp
io.SetEngine("BP5");

// Route data files to S3
io.SetParameter("DataTransport", "awssdk");

// S3 endpoint (for non-AWS S3-compatible storage)
io.SetParameter("S3Endpoint", "https://s3.amazonaws.com");

// S3 bucket
io.SetParameter("S3Bucket", "mybucket");

// Object mode: "multi" (default) or "single"
io.SetParameter("S3ObjectMode", "multi");

// Zero-copy direct upload threshold for multi mode (default 1MB)
io.SetParameter("S3DirectUploadThreshold", "10MB");

// Credentials (also available via environment variables)
io.SetParameter("S3AccessKeyID", "...");
io.SetParameter("S3SecretKey", "...");
io.SetParameter("S3SessionToken", "...");
```

### Environment Variables

Credentials and connection settings are read from environment variables when not set as ADIOS2 parameters:

```bash
AWS_ACCESS_KEY_ID=...
AWS_SECRET_ACCESS_KEY=...
AWS_SESSION_TOKEN=...     # optional, for temporary credentials
AWS_ENDPOINT=...          # S3 endpoint URL
ADIOS2_AWS_BUCKET=...     # S3 bucket name
AWS_CACHE=...             # local cache directory for read-side caching
```

### Transport Parameters

```cpp
"s3_object_mode"           // "multi" (default) or "single"
"direct_upload_threshold"  // Min size for zero-copy direct upload in multi mode (default 1MB)
"min_part_size"            // Minimum part size for single mode (default 5MB)
"max_part_size"            // Maximum part size for single mode (default 5GB)
"endpoint"                 // S3 endpoint URL
"bucket"                   // S3 bucket name
"accesskeyid"              // AWS access key ID
"secretkey"                // AWS secret key
"sessiontoken"             // AWS session token (for temporary credentials)
"verbose"                  // Verbosity level (0-5)
"cache"                    // Local cache directory path for read-side caching
"recheck_metadata"         // "true" (default) or "false" - revalidate cached metadata on open
```

### s3.json Sidecar File

Written to local BP5 directory at Open time:

```json
{
  "version": 1,
  "transport": "awssdk",
  "endpoint": "https://s3.amazonaws.com",
  "bucket": "mybucket"
}
```

## Append and AppendAfterSteps

Multi-object mode supports both `Mode::Append` and the `AppendAfterSteps` parameter.

### Simple Append

Opening a data file in `Mode::Append` discovers existing segment objects, finds the highest segment number, and continues numbering from there. New data is written as new objects without touching existing ones.

### AppendAfterSteps (Truncation)

When `AppendAfterSteps` is set, BP5 computes a truncation offset for each data file. The `Truncate(offset)` implementation:

1. Lists existing segment objects sorted by number
2. Walks them accumulating sizes until the offset is reached
3. Deletes all segment objects beyond that point
4. Resets the object counter to continue from the kept segments

Truncation offsets always fall on segment boundaries since each step's data starts at the beginning of a segment object.

### Stale Object Cleanup

On `Open(Write)`, the subfile-0 transport deletes all existing `data.*` objects under the BP directory prefix. This prevents stale objects from a previous write from being concatenated with new data on read. Only subfile 0 performs cleanup to avoid redundant work across ranks.

## Async Write

When `DataTransport` is set (e.g., to `awssdk`), `AsyncWrite` is automatically enabled (`Naive` mode) unless explicitly overridden. `FinalizeSegment()` runs inside the async write thread after data is written, so S3 uploads happen concurrently with the next step's computation.

## Limitations

### Aggregation Compatibility

S3 only supports sequential writes (no seek). The `EveryoneWrites` aggregation strategy requires multiple ranks to write at different offsets within the same data file, which is incompatible with S3. Use `TwoLevelShm` (the default) which assigns one writer per subfile.

### Accidental Deletion

If user deletes the local directory (`rm -rf sim.bp`), S3 data objects become orphaned. There is currently no cleanup utility; objects must be deleted manually from S3.

### 10K Part Limit (Single Mode Only)

Single-object mode is limited to 10,000 parts per object. For long-running simulations, use multi-object mode (default) which has no such limit.

### Single-Object Mode Limitations

Single-object mode does not support `Mode::Append` or `AppendAfterSteps`. These features are only available in multi-object mode.

## References

- [AWS S3 Multipart Upload](https://docs.aws.amazon.com/AmazonS3/latest/userguide/mpuoverview.html)
- [AWS SDK for C++ Async Operations](https://docs.aws.amazon.com/sdk-for-cpp/v1/developer-guide/async-methods.html)
- [S3 ListParts API](https://docs.aws.amazon.com/AmazonS3/latest/API/API_ListParts.html)
