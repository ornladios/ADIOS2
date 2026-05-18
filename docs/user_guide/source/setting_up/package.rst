####################
Install as a Package
####################

.. |pkg-summary| image:: https://repology.org/badge/latest-versions/adios2.svg
   :target: https://repology.org/project/adios2/versions
.. |pkg-conda| image:: https://img.shields.io/conda/v/conda-forge/libadios2
   :target: https://anaconda.org/conda-forge/adios2
.. |pkg-spack| image:: https://repology.org/badge/version-for-repo/spack/adios2.svg
   :target: https://repology.org/project/adios2/versions
.. |pkg-homebrew| image:: https://repology.org/badge/version-for-repo/homebrew/adios2.svg
   :target: https://repology.org/project/adios2/versions
.. |pkg-ubuntu2404| image:: https://repology.org/badge/version-for-repo/ubuntu_24_04/adios2.svg
   :target: https://repology.org/project/adios2/versions
.. |pkg-ubuntu2604| image:: https://repology.org/badge/version-for-repo/ubuntu_26_04/adios2.svg
   :target: https://repology.org/project/adios2/versions
.. |pkg-debian-unstable| image:: https://repology.org/badge/version-for-repo/debian_unstable/adios2.svg
   :target: https://repology.org/project/adios2/versions
.. |pkg-arch| image:: https://repology.org/badge/version-for-repo/arch/adios2.svg
   :target: https://repology.org/project/adios2/versions
.. |pkg-opensuse-tw| image:: https://repology.org/badge/version-for-repo/opensuse_tumbleweed/adios2.svg
   :target: https://repology.org/project/adios2/versions
.. |pkg-nix-unstable| image:: https://repology.org/badge/version-for-repo/nix_unstable/adios2.svg
   :target: https://repology.org/project/adios2/versions
.. |pkg-vcpkg| image:: https://repology.org/badge/version-for-repo/vcpkg/adios2.svg
   :target: https://repology.org/project/adios2/versions
.. |pkg-docker| image:: https://img.shields.io/docker/v/ornladios/adios2

.. list-table::
   :header-rows: 1

   * - Platform
     - Package
   * - Summary
     - |pkg-summary|
   * - Conda
     - |pkg-conda|
   * - Spack
     - |pkg-spack|
   * - Homebrew
     - |pkg-homebrew|
   * - Ubuntu 24.04
     - |pkg-ubuntu2404|
   * - Ubuntu 26.04
     - |pkg-ubuntu2604|
   * - Debian Unstable
     - |pkg-debian-unstable|
   * - Arch
     - |pkg-arch|
   * - OpenSUSE Tumbleweed
     - |pkg-opensuse-tw|
   * - Nix unstable
     - |pkg-nix-unstable|
   * - vcpkg
     - |pkg-vcpkg|
   * - Dockerhub
     - |pkg-docker|

*****************************************
Conda
*****************************************

ADIOS2 can be obtained from anaconda cloud:  `conda-forge adios2 <https://anaconda.org/conda-forge/adios2>`_

*****************************************
PyPI
*****************************************

ADIOS2 pip package can be downloaded with `pip3 install adios2` or `python3 -m pip install adios2`. This is contains the serial build only, so MPI programs cannot use it. See `adios2 on PyPi <https://pypi.org/project/adios2/>`_

*****************************************
Spack
*****************************************

ADIOS2 is packaged in Spack. See `adios2 spack package <https://packages.spack.io/package.html?name=adios2>`_

*****************************************
Docker
*****************************************

Docker images including building and installation of dependencies and ADIOS 2 containers for Ubuntu 20 and CentOS 7 can be found in:
under the directory `scripts/docker/ <https://github.com/ornladios/ADIOS2/tree/master/scripts/docker>`_
