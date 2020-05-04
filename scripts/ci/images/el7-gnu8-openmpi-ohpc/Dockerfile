FROM ornladios/adios2:ci-el7-gnu8-ohpc-base

# Install OpenHPC packages
RUN yum -y install python-devel python-numpy-gnu8-ohpc openmpi3-gnu8-ohpc \
        phdf5-gnu8-openmpi3-ohpc python-mpi4py-gnu8-openmpi3-ohpc

# Misc cleanup of unneeded files
RUN yum clean all && \
    rm -rfv /tmp/* /var/cache/yum
