FROM ornladios/adios2:ci-el7-intel-ohpc-base

# Install OpenHPC packages
RUN yum -y install openmpi3-intel-ohpc phdf5-intel-openmpi3-ohpc \
        python3-mpi4py-intel-openmpi3-ohpc && \
    sed 's|python3\.4|python3.6|g' -i /opt/ohpc/pub/moduledeps/*/py3-mpi4py/*

# Misc cleanup of unneeded files
RUN yum clean all && \
    rm -rfv /tmp/* /var/cache/yum
