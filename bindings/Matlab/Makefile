ADIOS_DIR=/ccs/proj/e2e/pnorbert/ADIOS/ADIOS2/build.titan.gnu-nompi/install
ADIOS_INC=-I${ADIOS_DIR}/include
ADIOS_LIBS=-Wl,-rpath=${ADIOS_DIR}/lib -L${ADIOS_DIR}/lib -ladios2 -ladios2_sst -ladios2_evpath  -ladios2_ffs -ladios2_enet   -ladios2_atl  -ladios2_dill -lbz2 -lrt

MEXOPTS=-DDEBUG CFLAGS="-g -std=c99 -fPIC"
default:
	@echo "ADIOS reader methods for MATLAB"
	@echo "Have 'mex' compiler in the path and run"
	@echo "> make matlab"


matlab: adiosopenc adiosclosec adiosreadc

INC:=$(shell echo ${ADIOSREAD_SEQ_INC})
LIB:=$(shell echo ${ADIOSREAD_SEQ_CLIB})

adiosopenc:  
	mex adiosopenc.c  ${MEXOPTS} -I. ${ADIOS_INC} ${ADIOS_LIBS}

adiosreadc: 
	mex adiosreadc.c  ${MEXOPTS} -I. ${ADIOS_INC} ${ADIOS_LIBS}

adiosclosec:
	mex adiosclosec.c ${MEXOPTS} -I. ${ADIOS_INC} ${ADIOS_LIBS}

#adiosdbgc:
#       mex adiosdbgc.c ${MEXOPTS} -I. ${ADIOS_INC} ${ADIOS_LIBS}


clean:
	rm -f *.mexa64 core* *.o 

install:
	@echo "Install *.m and *.mexa64 into MATLABPATH"
	@echo "Do not have 'adios' anywhere in the path where you install the files"
	@echo "otherwise 'help adios' will not work in Matlab"
