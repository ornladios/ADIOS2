[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![Documentation](https://readthedocs.org/projects/adios2/badge/?version=latest)](https://adios2.readthedocs.io/en/latest/?badge=latest)
[![GitHub release](https://img.shields.io/github/release/ornladios/adios2/all.svg)]()
[![latest packaged version(s)](https://repology.org/badge/latest-versions/adios2.svg)](https://repology.org/project/adios2/versions)
[![OpenSSF Scorecard](https://api.scorecard.dev/projects/github.com/ornladios/ADIOS2/badge)](https://scorecard.dev/viewer/?uri=github.com/ornladios/ADIOS2)
[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/11410/badge)](https://www.bestpractices.dev/projects/11410)

# ADIOS2 : The Adaptable Input Output System version 2

ADIOS2 is an adaptable, scalable, and unified framework for scientific data I/O.
It transports self-describing variables and attributes across files, networks, and memory using a single API, and is used in production on the world's largest supercomputers.

### Features

- **Unified I/O API**: same interface for files, wide-area networks, and in-memory staging; switch transport without changing application code
- **BP format**: self-describing binary-packed format (`.bp`) with rapid metadata extraction and built-in compression support
- **MPI-native**: scales from laptop to petabyte workloads on the largest HPC systems; serial use also supported
- **Streaming and step-based**: asynchronous, grouped variable transport with an explicit steps abstraction for in-situ and in-transit workflows
- **Multiple language bindings**: C++17, Python, C99, Fortran 90, and Matlab
- **Compression operators**: ZFP, SZ, MGARD, Blosc2, PNG, and others
- **Encryption operators**: symmetric (AES-256-GCM) and asymmetric encryption available for data at rest
- **In-situ/in-transit engines**: SST and DataMan engines for code coupling over RDMA, MPI, and TCP without touching files
- **DAOS engine**: writes and reads datasets through the DAOS data plane (native object API) directly, with no dfuse or POSIX interception required
- **Plugin support**: extend ADIOS2 with custom engines and operators via shared-library plugins at runtime
- **Security-focused**: committed to OpenSSF best practices; see the [OpenSSF Scorecard](https://scorecard.dev/viewer/?uri=github.com/ornladios/ADIOS2) and [Best Practices badge](https://www.bestpractices.dev/projects/11410)
- **Widely available**: packaged for major Linux distributions, Conda, Spack, Homebrew, and vcpkg; validated on U.S. DoE supercomputers including Frontier, Summit, and Perlmutter
- **HDF5 VOL**: existing HDF5 applications can use ADIOS2 as a backend transparently via the HDF5 Virtual Object Layer

## Example

```python
import adios2
import numpy as np

data = np.array([1.0, 2.0, 3.0, 4.0, 5.0])

# Write to a BP file
with adios2.Stream("data.bp", "w") as f:
    f.write("data", data)

# Read it back
with adios2.Stream("data.bp", "r") as f:
    for _ in f.steps():
        result = f.read("data")

print(result)  # [1. 2. 3. 4. 5.]
```

## Install ADIOS2

**Python (PyPI):**
```bash
pip install adios2
```

**From source:**
See the [install documentation](https://adios2.readthedocs.io/en/latest/setting_up/setting_up.html#) and [scripts/runconf/runconf.sh](https://github.com/ornladios/ADIOS2/blob/master/scripts/runconf/runconf.sh) for a CMake configuration example.
Once installed, refer to [Linking ADIOS2](https://adios2.readthedocs.io/en/latest/setting_up/setting_up.html#linking-adios-2).

## Packages

| Platform            | Package                                                                                                                                                    |
|---------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Summary             | [![latest packaged version(s)](https://repology.org/badge/latest-versions/adios2.svg)](https://repology.org/project/adios2/versions)                       |
| Conda               | [![Conda Version](https://img.shields.io/conda/v/conda-forge/libadios2)](https://anaconda.org/conda-forge/adios2)                                          |
| Spack               | [![Spack package](https://repology.org/badge/version-for-repo/spack/adios2.svg)](https://repology.org/project/adios2/versions)                             |
| Homebrew            | [![Homebrew package](https://repology.org/badge/version-for-repo/homebrew/adios2.svg)](https://repology.org/project/adios2/versions)                       |
| Ubuntu 24.04        | [![Ubuntu 24.04 package](https://repology.org/badge/version-for-repo/ubuntu_24_04/adios2.svg)](https://repology.org/project/adios2/versions)               |
| Ubuntu 26.04        | [![Ubuntu 26.04 package](https://repology.org/badge/version-for-repo/ubuntu_26_04/adios2.svg)](https://repology.org/project/adios2/versions)               |
| Debian Unstable     | [![Debian Unstable package](https://repology.org/badge/version-for-repo/debian_unstable/adios2.svg)](https://repology.org/project/adios2/versions)         |
| Arch                | [![Arch Linux package](https://repology.org/badge/version-for-repo/arch/adios2.svg)](https://repology.org/project/adios2/versions)                         |
| OpenSUSE TumbleWeed | [![openSUSE Tumbleweed package](https://repology.org/badge/version-for-repo/opensuse_tumbleweed/adios2.svg)](https://repology.org/project/adios2/versions) |
| Nix unstable        | [![nixpkgs unstable package](https://repology.org/badge/version-for-repo/nix_unstable/adios2.svg)](https://repology.org/project/adios2/versions)           |
| vcpkg               | [![Vcpkg package](https://repology.org/badge/version-for-repo/vcpkg/adios2.svg)](https://repology.org/project/adios2/versions)                             |
| Dockerhub           | ![Docker Image Version](https://img.shields.io/docker/v/ornladios/adios2)                                                                                  |

## Resources

* Documentation: [adios2.readthedocs.io](https://adios2.readthedocs.io)
* Latest release: [v2.12.1](https://github.com/ornladios/ADIOS2/releases/tag/v2.12.1)
* All releases: [github.com/ornladios/ADIOS2/releases](https://github.com/ornladios/ADIOS2/releases)
* Citing: if you find ADIOS2 useful, please cite our [SoftwareX paper](https://doi.org/10.1016/j.softx.2020.100561)

## Community

ADIOS2 is an open source project: questions, discussion, and contributions are welcome.

- Mailing list: adios-ecp@kitware.com
- Github Discussions: https://github.com/ornladios/ADIOS2/discussions
- Bug reports: [open an issue](https://github.com/ornladios/ADIOS2/issues)
- Contributing: see the [Contributor's Guide](Contributing.md)

## Institutions

ADIOS2 is developed as a multi-institutional collaboration between:

- [Oak Ridge National Laboratory](https://www.ornl.gov)
- [Kitware Inc.](https://www.kitware.com)
- [Lawrence Berkeley National Laboratory](http://www.lbl.gov)
- [Georgia Institute of Technology](http://www.gatech.edu)
- [Rutgers University](http://www.rutgers.edu)

## License
ADIOS2 is licensed under the Apache License v2.0.
See the accompanying [Copyright.txt](Copyright.txt) for more details.
