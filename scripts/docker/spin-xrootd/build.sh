#!/bin/bash
# Build Docker image for NERSC Spin deployment
# This script builds an x86_64 image even on ARM Macs

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ADIOS2_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

IMAGE_NAME="${IMAGE_NAME:-adios2-xrootd-http}"
IMAGE_TAG="${IMAGE_TAG:-latest}"

echo "Building ADIOS2 XRootD HTTP image for x86_64 (linux/amd64)"
echo "ADIOS2 source: $ADIOS2_ROOT"
echo "Image: $IMAGE_NAME:$IMAGE_TAG"
echo ""

# Create a temporary directory for the build context
# This avoids issues with .claude directory permissions
BUILD_CONTEXT=$(mktemp -d)
trap 'rm -rf "$BUILD_CONTEXT"' EXIT

echo "Creating clean build context in $BUILD_CONTEXT..."

# Use git archive to get a clean copy of tracked files, then add untracked new files
cd "$ADIOS2_ROOT"
git archive --format=tar HEAD | tar -x -C "$BUILD_CONTEXT"

# Copy any new untracked source files that are needed for the build
# (like our new XrootdHttpRemote files)
for f in source/adios2/toolkit/remote/XrootdHttpRemote.h \
         source/adios2/toolkit/remote/XrootdHttpRemote.cpp \
         scripts/docker/spin-xrootd/Dockerfile \
         scripts/docker/spin-xrootd/xrootd-http.cfg \
         scripts/docker/spin-xrootd/docker-entrypoint.sh; do
    if [ -f "$ADIOS2_ROOT/$f" ]; then
        mkdir -p "$BUILD_CONTEXT/$(dirname "$f")"
        cp "$ADIOS2_ROOT/$f" "$BUILD_CONTEXT/$f"
    fi
done

# Also copy modified tracked files (git archive gets the committed version)
for f in source/adios2/CMakeLists.txt \
         source/adios2/engine/bp5/BP5Reader.cpp; do
    if [ -f "$ADIOS2_ROOT/$f" ]; then
        cp "$ADIOS2_ROOT/$f" "$BUILD_CONTEXT/$f"
    fi
done

echo "Build context ready."
echo ""

# Build for linux/amd64 (x86_64) platform
# This is required for NERSC Spin which runs x86_64
docker buildx build \
    --platform linux/amd64 \
    --tag "$IMAGE_NAME:$IMAGE_TAG" \
    --file "$BUILD_CONTEXT/scripts/docker/spin-xrootd/Dockerfile" \
    --load \
    "$BUILD_CONTEXT"

echo ""
echo "Build complete!"
echo ""
echo "To test locally:"
echo "  docker run --rm -p 8080:8080 $IMAGE_NAME:$IMAGE_TAG"
echo ""
echo "To push to a registry for Spin:"
echo "  docker tag $IMAGE_NAME:$IMAGE_TAG registry.spin.nersc.gov/YOUR_PROJECT/$IMAGE_NAME:$IMAGE_TAG"
echo "  docker push registry.spin.nersc.gov/YOUR_PROJECT/$IMAGE_NAME:$IMAGE_TAG"
