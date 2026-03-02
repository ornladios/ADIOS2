# ADIOS2 XRootD HTTP Server for NERSC Spin

Docker image that serves ADIOS2 BP5 datasets over HTTPS using the XRootD
framework. Designed for deployment on NERSC Spin (Kubernetes) behind an
ingress controller that terminates TLS.

## Architecture

Two XRootD plugins work together inside the container:

- **SSI plugin** (`libadios2_xrootd.so`) — handles the ADIOS2 data protocol
  (open, get variable metadata, read data). Manages a pool of open ADIOS2
  engines with resource-aware eviction.
- **HTTP handler** (`libadios2_xrootd_http.so`) — bridges HTTP POST requests
  to the SSI plugin, and serves admin endpoints.

Clients use the ADIOS2 remote reader, which sends requests over HTTPS to
port 8080. Spin ingress terminates TLS and forwards plain HTTP to the
container.

## Building

```bash
# From this directory, or use the helper script:
./build.sh

# Build from a specific branch:
ADIOS2_BRANCH=my-branch ./build.sh

# Build from a fork:
ADIOS2_REPO=https://github.com/user/ADIOS2.git ADIOS2_BRANCH=branch ./build.sh
```

The image is built for `linux/amd64` (required by Spin).

## Running Locally

```bash
docker run --rm -p 8080:8080 -v /path/to/data:/data adios2-xrootd-http
```

## Resource Management

The file pool caches open ADIOS2 engines to avoid repeated open/close
overhead. It tracks two resource dimensions:

- **File descriptors** — each non-tar BP5 file uses one FD per data subfile.
  Tar-packed files share a single FD via the SharedTarFDCache.
- **Metadata memory** — md.0, mmd.0, and md.idx are held in memory per
  cached file.

When usage approaches configured limits, idle entries are evicted
(heaviest first). Entries idle for more than 4 hours are evicted
regardless of resource pressure.

### Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `ADIOS_POOL_FD_LIMIT` | 90% of `ulimit -n` | Max estimated FDs for cached files |
| `ADIOS_POOL_METADATA_LIMIT` | 2147483648 (2 GB) | Max metadata bytes in cache |

## Admin Interface

HTTP endpoints for monitoring and managing the running server:

```
GET /_adios/stats   — pool statistics (JSON)
GET /_adios/files   — list of cached files (JSON)
GET /_adios/flush   — flush all idle cache entries
GET /_adios/limits  — view current resource limits (JSON)
GET /_adios/limits?fd=N&md=N — set limits (0 resets to default)
```

From inside the container:
```bash
curl localhost:8080/_adios/stats
```

## Configuration

`xrootd-http.cfg` configures:
- SSI plugin library path
- HTTP port (8080)
- HTTP external handler with `ssilib=` parameter pointing to the SSI plugin
- Self-signed TLS certificates (required by XRootD even behind TLS-terminating ingress)

## Deployment on Spin

1. Push image to a registry accessible from Spin
2. Create a deployment with the image, mounting data at `/data`
3. Configure ingress to route HTTPS traffic to port 8080
4. The `hosts.yaml` client config should point to the ingress hostname
