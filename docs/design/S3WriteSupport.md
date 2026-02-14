# ADIOS2 S3 Write Support Design

**Status**: Implemented (multi-object default, single-object optional)
**Date**: January-February 2026
**Authors**: Greg Eisenhauer, Claude

## Overview

ADIOS2 supports writing BP5 data files to S3-compatible object storage while keeping metadata local for fast access. Two object modes are available:

- **Multi-object mode** (default): Each data file is stored as a sequence of numbered S3 objects (`data.0.0`, `data.0.1`, ...). Objects are finalized individually via `PutObject`, providing automatic crash recovery.
- **Single-object mode**: Each data file is stored as one S3 object via multipart upload. Requires a finalize utility for crash recovery.

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

Object numbers are sequential starting at 0 and are **independent of step numbers**. Multiple objects may be created per step (via mid-step flushes or large writes).

### FinalizeSegment

The `Transport::FinalizeSegment()` virtual method signals that accumulated data should be uploaded as a numbered object. It is called:

1. **At EndStep boundaries** - after all step data is flushed
2. **During PerformDataWrite/Flush** - for mid-step memory relief
3. **At Close** - to upload any remaining buffered data

For non-S3 transports, `FinalizeSegment()` is a no-op.

### Write Flow

```
Open(data.0, Write):
  → Store base object name "data.0"
  → Initialize object counter = 0

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

Requires the `adios2_s3_finalize` utility to complete incomplete multipart uploads using S3's `ListParts` API.

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

```bash
AWS_ENDPOINT=https://s3.amazonaws.com
ADIOS2_AWS_BUCKET=mybucket
AWS_ACCESS_KEY_ID=...
AWS_SECRET_KEY=...
AWS_SESSION_TOKEN=...
```

### Transport Parameters

```cpp
"s3_object_mode"           // "multi" (default) or "single"
"direct_upload_threshold"  // Min size for zero-copy direct upload in multi mode (default 1MB)
"min_part_size"            // Minimum part size for single mode (default 5MB)
"max_part_size"            // Maximum part size for single mode (default 5GB)
"endpoint"                 // S3 endpoint URL
"bucket"                   // S3 bucket name
"verbose"                  // Verbosity level (0-5)
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

## Limitations

### No Append Operations

S3 objects are immutable after upload. Append mode is not supported.

### Accidental Deletion

If user deletes the local directory (`rm -rf sim.bp`), S3 data objects become orphaned. Use `adios2_s3_delete` instead.

### 10K Part Limit (Single Mode Only)

Single-object mode is limited to 10,000 parts per object. For long-running simulations, use multi-object mode (default) which has no such limit.

## References

- [AWS S3 Multipart Upload](https://docs.aws.amazon.com/AmazonS3/latest/userguide/mpuoverview.html)
- [AWS SDK for C++ Async Operations](https://docs.aws.amazon.com/sdk-for-cpp/v1/developer-guide/async-methods.html)
- [S3 ListParts API](https://docs.aws.amazon.com/AmazonS3/latest/API/API_ListParts.html)
