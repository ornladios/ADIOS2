FROM ghcr.io/ornladios/adios2:ci-opensuse-tw-sanitizer-base

# Install core dev packages
RUN zypper install -y --no-recommends libasan8 hdf5-devel && \
    zypper clean --all

# Install ZFP
WORKDIR /opt/zfp
RUN curl -L https://github.com/LLNL/zfp/releases/download/0.5.5/zfp-0.5.5.tar.gz | tar -xvz && \
    cmake -GNinja -S zfp-0.5.5 -B build \
      -DBUILD_SHARED_LIBS=ON \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/opt/zfp/0.5.5 \
      && \
    cmake --build build && \
    cmake --install build && \
    rm -rf zfp-0.5.5 build
ENV PATH=/opt/zfp/0.5.5/bin:${PATH} \
    LD_LIBRARY_PATH=/opt/zfp/0.5.5/lib64:${LD_LIBRARY_PATH} \
    CMAKE_PREFIX_PATH=/opt/zfp/0.5.5:${CMAKE_PREFIX_PATH}
