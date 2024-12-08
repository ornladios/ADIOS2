ARG INSTALL_PREFIX=/opt/tsan
ARG TOOLCHAIN_FILE=toolchain-tsan.cmake
ARG CFLAGS="-fsanitize=thread"
ARG CXXFLAGS="-fsanitize=thread"
ARG LLVM_USE_SANITIZER=Thread

FROM ghcr.io/ornladios/adios2:ci-opensuse-tw-full-stack-onbuild AS tmp-stage
FROM ghcr.io/ornladios/adios2:ci-opensuse-tw-sanitizer-base
COPY --from=tmp-stage /opt/tsan/ /opt/tsan/
