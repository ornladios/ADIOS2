################################################################################
# READ THIS FIRST!!!!!
################################################################################
# It is extremly important that this image be squashed before pushing using
# either docker build --squash, docker-squash, or some other mechanism.  The
# last step in this image build removes the various compiler installer
# artifacts, including the license file.  Failure to squash the image will
# result in the license file being present within the layers of the final image.
#
# For the compiler to run successfully in this image, the license file needs to
# be populated before running.  To achieve this on a public CI service:
#
#   1.  Create a secret variable INTEL_LICENSE_FILE_CONTENT
#   2.  Set the value of the variable of the base64 encoded content of
#       the license file.  You can get this from the following:
#         $ base64 -w0 /path/to/license.lic
#   3.  Create a step in the build before anything else is run that sets up the
#       the license file from the secret variable:
#         mkdir -p /opt/intel/licenses
#         echo "$INTEL_LICENSE_FILE_CONTENT" | base64 -d | sudo tee /opt/intel/licenses/license.lic > /dev/null
#
# The following build arguments can be passed:
#   LICENSE_FILE - The name of the license file in the build directory
#   INTEL_VERSION - The compiler version to use for installer files in the
#     build directory.  For example, a value of 2019_update6 would copy and
#     install:
#       parallel_studio_xe_2019_update6_composer_edition_for_cpp.tgz
#       parallel_studio_xe_2019_update6_composer_edition_for_fortran.tgz
#
################################################################################

FROM ornladios/adios2:ci-el7-base

# Install Intel C++ and Fortran compilers
ARG LICENSE_FILE=CI.lic
ARG INTEL_VERSION=2020_update2
WORKDIR /tmp
COPY ${LICENSE_FILE} /opt/intel/licenses/license.lic
COPY silent-cpp.cfg /tmp
COPY silent-fortran.cfg /tmp
COPY parallel_studio_xe_${INTEL_VERSION}_composer_edition_for_cpp.tgz /tmp/
COPY parallel_studio_xe_${INTEL_VERSION}_composer_edition_for_fortran.tgz /tmp/
RUN tar -xzf parallel_studio_xe_${INTEL_VERSION}_composer_edition_for_cpp.tgz && \
    ./parallel_studio_xe_${INTEL_VERSION}_composer_edition_for_cpp/install.sh -s ./silent-cpp.cfg && \
    tar -xzf parallel_studio_xe_${INTEL_VERSION}_composer_edition_for_fortran.tgz && \
    ./parallel_studio_xe_${INTEL_VERSION}_composer_edition_for_fortran/install.sh -s ./silent-fortran.cfg

# Install OpenHPC packages
RUN yum install -y /tmp/*_for_cpp/rpm/intel-{comp,ps}xe-doc-*.rpm && \
    yum -y install https://github.com/openhpc/ohpc/releases/download/v1.3.GA/ohpc-release-1.3-1.el7.x86_64.rpm && \
    yum -y install lmod-ohpc intel-compilers-devel-ohpc \
        python3-numpy-intel-ohpc && \
    sed 's|python3\.4|python3.6|g' -i /opt/ohpc/pub/moduledeps/*/py3-numpy/*

# Install ZFP
WORKDIR /opt/zfp
RUN curl -L https://github.com/LLNL/zfp/releases/download/0.5.5/zfp-0.5.5.tar.gz | tar -xvz && \
    mkdir build && \
    cd build && \
    source /etc/profile && \
    module load intel && \
    export CC=icc CXX=icpc FC=ifort && \
    cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/zfp/0.5.5 ../zfp-0.5.5 && \
    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
    cd .. && \
    rm -rf zfp-0.5.5 build
ENV PATH=/opt/zfp/0.5.5/bin:${PATH} \
    LD_LIBRARY_PATH=/opt/zfp/0.5.5/lib64:${LD_LIBRARY_PATH} \
    CMAKE_PREFIX_PATH=/opt/zfp/0.5.5:${CMAKE_PREFIX_PATH}

# Install SZ
WORKDIR /opt/sz
RUN curl -L https://github.com/disheng222/SZ/archive/v2.1.8.3.tar.gz | tar -xvz && \
    mkdir build && \
    cd build && \
    source /etc/profile && \
    module load intel && \
    export CC=icc CXX=icpc FC=ifort && \
    cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/sz/2.1.8.3 ../SZ-2.1.8.3 && \
    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
    cd .. && \
    rm -rf SZ-2.1.8.3 build
ENV PATH=/opt/sz/2.1.8.3/bin:${PATH} \
    LD_LIBRARY_PATH=/opt/sz/2.1.8.3/lib64:${LD_LIBRARY_PATH} \
    CMAKE_PREFIX_PATH=/opt/sz/2.1.8.3:${CMAKE_PREFIX_PATH}

# Misc cleanup of unneeded files
RUN rm -rfv \
        /tmp/* \
        /opt/intel/licenses/* \
        /opt/intel/man \
        /opt/intel/documentation_* \
        /opt/intel/ide_support_* \
        /opt/intel/samples_* \
        /opt/intel/compilers_and_libraries_*.*.*/linux/mkl/benchmarks \
        /opt/intel/compilers_and_libraries_*.*.*/linux/mkl/examples \
        /var/cache/yum
