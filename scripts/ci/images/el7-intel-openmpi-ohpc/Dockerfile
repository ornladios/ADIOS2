FROM ornladios/adios2:ci-el7-intel-ohpc-base

# Install OpenHPC packages
RUN yum -y install python34-devel python34-numpy-intel-ohpc \
        openmpi3-intel-ohpc phdf5-intel-openmpi3-ohpc \
        python34-mpi4py-intel-openmpi3-ohpc

# Misc cleanup of unneeded files
RUN yum clean all && \
    rm -rfv /tmp/* /var/cache/yum
