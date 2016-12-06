# Adaptable Input / Output System (ADIOS) v2.0

This is v2.0 of the ADIOS I/O system, developed as part of the DoE. Exascale
Computing Program.

## License
ADIOS >= 2.0 is licensed under the Apache License v2.0.  See the accompanying
Copyright.txt for more details.

## Directory layout

/cmake    : Project specific CMake modules
/examples : ADIOS Examples
/include  : Public header files 
/scripts  : Project maintenance and development scripts
source    : Main ADIOS source
-- /foo   : Source and private header files for the "foo" component
testing   : Tests

## Developers

###Getting started

Uppon cloning this repo, you will need to run the scripts/developer/setup.sh
script.  This will perform the following:

  * Validate that clang-format is available
  * Setup formatting commit hooks

