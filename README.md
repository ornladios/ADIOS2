# ADIOSPP

	Next generation ADIOS2.0 in C++11 for exascale computations. 
	Read ./doc/CodingGuidelines first if you are a developer.
	Doxygen documentation can be generated running doxygen under ./doc, a ./doc/html directory will be created

	Requirements: 
	1) C++11 compiler (e.g. gnu gcc 4.8.x and above) in PATH, default is g++
	2) MPI compiler (e.g. openmpi, mpich2 ) in PATH, default is mpic++
  
	MULTICORE BUILD MODES: ( use -jn where n: number of cores)

	Basic modes;
		build ./lib/libadios.a and ./lib/libadios_nompi.a
		make -jn 
		e.g.:	make -j4
		
		build ./lib/libadios.so and ./lib/libadios_nompi.so
		make -jn 
		e.g.:	make -j4    
	     
		build ./lib/libadios.a (.so) with truly MPI code (from mpi.h) using mpic++
		make mpi -jn
		e.g:     make mpi -j4  
		
		build ./lib/libadios_nompi.a with serial (dummy MPI) code only calling mpidummy.h using g++ (C++11)
		make nompi -jn 
		e.g:     make nompi -j4
		
	Additional modes:
		
		For shared library mode add the SHARED=yes option to any build:
		build ./lib/libadios.so and ./lib/libadios_nompi.so
		make -jn SHARED=yes
		e.g.:	make -j4 SHARED=yes
			
		build with DataMan library (https://github.com/JasonRuonanWang/DataMan) for WAN transport 
		Make sure DataMan path is correct in Makefile.libs add HAVE_DATAMAN=yes flag
		e.g.:	make -j4 HAVE_DATAMAN=yes


	For examples, start with examples/hello/bpWriter/, build as above after ADIOS basic modes library is built
		cd examples/hello/bpWriter/
		make -j4
		mpirun -n 4 ./helloBPWriter.exe   -> generates myDoubles.bp directory
		./helloBPWriter_nompi.exe       -> generates myDoubles_nompi.bp directory