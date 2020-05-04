FROM ornladios/adios2:ci-el7-gnu8-ohpc-base

# Install OpenHPC packages
RUN yum -y install python34-devel python34-numpy-gnu8-ohpc hdf5-gnu8-ohpc

# Misc cleanup of unneeded files
RUN yum clean all && \
    rm -rfv /tmp/* /var/cache/yum
