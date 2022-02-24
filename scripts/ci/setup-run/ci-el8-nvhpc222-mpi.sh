module unload $(module -t --redirect avail openmpi)
source $(dirname $(readlink -f ${BASH_SOURCE}))/ci-el8-nvhpc222.sh
