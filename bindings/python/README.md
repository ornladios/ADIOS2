Python bindings for ADIOS 2.0

Dependencies:
	
	Python 2.7.x
	Numpy and MPI4PY
	ADIOS shared library (libadios.so) built with : make SHARED=yes (see ADIOS README.md)  
	Add boost python and libadios.so libraries path in LD_LIBRARY_PATH or DYLD_LIBRARY_PATH


For Boost Python use: 
	make -jn , where n is the number of cores
	e.g.
	make -j4 
	make -j8

	Additional Requirements:
	Boost.Python version 1.63 (latest)
	

For PyBind11 use:
	make -jn HAVE_PYBIND11=yes, where n is the number of cores
	e.g.
	make -j4 HAVE_PYBIND11=yes
	make -j8 HAVE_PYBIND11=yes
	
	Requirements:
	PyBind11 version 2.0.1 or master from github (header only library, no need to compile)
