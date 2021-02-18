ARG base=ubuntu20.04
FROM ornladios/adios2:dependencies-${base}

USER root

# Install adios
# Note: There's a little extra bit of trickery here to retrieve active pull
# requests not yet merged.  This is only used for CI purposes and can be
# dropped from your own images
ARG adios_ver=master
WORKDIR /opt/adios
RUN git clone https://github.com/ornladios/adios2.git source && \
    cd source && \
    git fetch origin +refs/pull/*/head:refs/remotes/origin/pr* && \
    git checkout ${adios_ver} && \
    cd .. && \
    mkdir build && \
    cd build && \
    . /etc/profile && \
    if [ -f /etc/redhat-release ]; then   module load mpi; fi && \
    cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_TESTING=OFF \
      -DADIOS2_BUILD_EXAMPLES=OFF \
      ../source && \
    adios_ver_dir=$(sed -n 's|^set(PACKAGE_VERSION "\([^"]*\)")|\1|p' adios2-config-version.cmake|head -1) && \
    cmake -DCMAKE_INSTALL_PREFIX=/opt/adios/${adios_ver_dir} . && \
    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
    cd .. && \
    rm -rf source build && \
    echo "export PATH=\"/opt/adios/${adios_ver_dir}/bin:${PATH}\"" \
        >> /etc/profile.d/adios2.sh && \
    echo "export LD_LIBRARY_PATH=\"/opt/adios/${adios_ver_dir}/lib:${LD_LIBRARY_PATH}\"" \
        >> /etc/profile.d/adios2.sh && \
    echo "export CMAKE_PREFIX_PATH=\"/opt/adios/${adios_ver_dir}:${CMAKE_PREFIX_PATH}\"" \
        >> /etc/profile.d/adios2.sh && \
    echo "export PYTHONPATH=\"/opt/adios/${adios_ver_dir}/lib/python3/dist-packages:${PYTHONPATH}\"" \
        >> /etc/profile.d/adios2.sh

WORKDIR /home/adios
USER adios
