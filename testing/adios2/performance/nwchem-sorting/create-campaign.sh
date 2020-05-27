#! /bin/bash

directory=$PROJWORK/csc143/xin/adios_iotest_nwchem_copro
touch $directory
rm -rf $directory
cheetah create-campaign -e cheetah-campaign-adios-iotest.py -m rhea -o $directory -a $PWD

