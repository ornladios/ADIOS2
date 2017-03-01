# Makefile
# Created on: Feb 13, 2017
#     Author: pnorbert

#
#   SYSTEM SPECIFIC SETTINGS
#
#COMPILERS
CC=g++ 
MPICC=mpic++

#FLAGS
CFLAGS=-Wall -Wpedantic -Woverloaded-virtual -std=c++11 -O0 -g
LDFLAGS=-lpthread

#ADIOS1 LOCATION
ADIOS1_DIR=/opt/adios/1.11

#ADIOS2 LOCATION
ADIOS2_DIR=../..

# SEQUENTIAL HDF5 LOCATION
HDF5_DIR=/opt/hdf5-1.8.17
HDF5_INCLUDE=-I$(HDF5_DIR)/include 
HDF5_LIB=$(HDF5_DIR)/lib/libhdf5.la

#
#   SHOULD NOT NEED TO CHANGE ANYTHING BELOW
#

ADIOS1_INCLUDE=-I$(ADIOS1_DIR)/include
ADIOS1_LIB=`${ADIOS1_DIR}/bin/adios_config -l`


ADIOS2_INCLUDE= -DHAVE_MPI -I$(ADIOS2_DIR)/include
ADIOS2_LIB=$(ADIOS2_DIR)/lib/libadios.a

default: 
	@echo "Make targets: ascii hdf5_a hdf5_b phdf5 adios1 adios2"
	@echo "  ascii:    build example with text output(IO_ascii.cpp)"
	@echo "  hdf5_a:   build example with HDF5, separate timesteps (IO_hdf5_a.cpp)"
	@echo "  hdf5_b:   build example with HDF5, combined timesteps (IO_hdf5_b.cpp)"
	@echo "  phdf5:    build example with Parallel HDF5  (IO_phdf5.cpp)"
	@echo "  adios1:   build example with ADIOS 1.x  (IO_adios1.cpp)"
	@echo "  adios2:   build example with ADIOS2  (IO_adios2.cpp)"


all: default
help: default


ascii: main.cpp HeatTransfer.cpp Settings.cpp IO_ascii.cpp 
	    $(MPICC) $(CFLAGS) -o heatTransfer_ascii $^ $(LDFLAGS)   

hdf5_a: main.cpp HeatTransfer.cpp Settings.cpp IO_hdf5_a.cpp 
	    libtool --mode=link --tag=CC $(MPICC) $(CFLAGS) $(HDF5_INCLUDE) -o heatTransfer_hdf5_a $^ $(HDF5_LIB) $(LDFLAGS)
	    
adios1: main.cpp HeatTransfer.cpp Settings.cpp IO_adios1.cpp 
	    $(MPICC) $(CFLAGS) $(ADIOS1_INCLUDE) -o heatTransfer_adios1 $^ $(ADIOS1_LIB) $(LDFLAGS)  

adios2: main.cpp HeatTransfer.cpp Settings.cpp IO_adios2.cpp 
	    $(MPICC) $(CFLAGS) $(ADIOS2_INCLUDE) -o heatTransfer_adios2 $^ $(ADIOS2_LIB) $(LDFLAGS)  
	
clean:
	rm -f heatTransfer_ascii heatTransfer_hdf5_a heatTransfer_adios1 heatTransfer_adios2
     
