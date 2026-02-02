#!/bin/bash
# Build container image for NERSC Spin deployment
# This script builds an x86_64 image even on ARM Macs
# The Dockerfile clones ADIOS2 from GitHub, so no local source is needed
# Supports both Docker and Podman

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

IMAGE_NAME="${IMAGE_NAME:-adios2-xrootd-http}"
IMAGE_TAG="${IMAGE_TAG:-latest}"

# Optional: override repo/branch via environment variables
ADIOS2_REPO="${ADIOS2_REPO:-https://github.com/ornladios/ADIOS2.git}"
ADIOS2_BRANCH="${ADIOS2_BRANCH:-master}"

# Detect container runtime (prefer podman if both available, or use CONTAINER_CMD env var)
if [ -n "$CONTAINER_CMD" ]; then
    # User explicitly specified runtime
    :
elif command -v podman &> /dev/null; then
    CONTAINER_CMD="podman"
elif command -v docker &> /dev/null; then
    CONTAINER_CMD="docker"
else
    echo "Error: Neither docker nor podman found in PATH"
    exit 1
fi

echo "Building ADIOS2 XRootD HTTP image for x86_64 (linux/amd64)"
echo "Container runtime: $CONTAINER_CMD"
echo "ADIOS2 repo: $ADIOS2_REPO"
echo "ADIOS2 branch: $ADIOS2_BRANCH"
echo "Image: $IMAGE_NAME:$IMAGE_TAG"
echo ""

# Build for linux/amd64 (x86_64) platform
# This is required for NERSC Spin which runs x86_64
if [ "$CONTAINER_CMD" = "podman" ]; then
    # Podman has native cross-platform support
    podman build \
        --platform linux/amd64 \
        --build-arg "ADIOS2_REPO=$ADIOS2_REPO" \
        --build-arg "ADIOS2_BRANCH=$ADIOS2_BRANCH" \
        --tag "$IMAGE_NAME:$IMAGE_TAG" \
        --file "$SCRIPT_DIR/Dockerfile" \
        "$SCRIPT_DIR"
else
    # Docker uses buildx for cross-platform builds
    docker buildx build \
        --platform linux/amd64 \
        --build-arg "ADIOS2_REPO=$ADIOS2_REPO" \
        --build-arg "ADIOS2_BRANCH=$ADIOS2_BRANCH" \
        --tag "$IMAGE_NAME:$IMAGE_TAG" \
        --file "$SCRIPT_DIR/Dockerfile" \
        --load \
        "$SCRIPT_DIR"
fi

echo ""
echo "Build complete!"
echo ""
echo "To test locally:"
echo "  $CONTAINER_CMD run --rm -p 8080:8080 $IMAGE_NAME:$IMAGE_TAG"
echo ""
echo "To push to a registry for Spin:"
echo "  $CONTAINER_CMD tag $IMAGE_NAME:$IMAGE_TAG registry.spin.nersc.gov/YOUR_PROJECT/$IMAGE_NAME:$IMAGE_TAG"
echo "  $CONTAINER_CMD push registry.spin.nersc.gov/YOUR_PROJECT/$IMAGE_NAME:$IMAGE_TAG"
echo ""
echo "To build from a different branch:"
echo "  ADIOS2_BRANCH=feature-branch ./build.sh"
echo ""
echo "To force a specific container runtime:"
echo "  CONTAINER_CMD=docker ./build.sh"
